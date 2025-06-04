// MQTT.cpp
#include "pch.h" // Pastikan pch.h meng-include semua header yang dibutuhkan seperti iostream, string, vector, memory, chrono, sstream, dan mqtt/async_client.h
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream> // Untuk std::ostringstream

#include "mqtt/async_client.h"

#ifdef _WIN32
#include <windows.h> // Untuk OutputDebugStringA dan HMODULE
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C"
#define OutputDebugStringA(str) std::cout << str // Fallback untuk non-Windows
typedef void* HMODULE; // Placeholder HMODULE untuk non-Windows
#endif

// Variabel global untuk state MQTT
std::unique_ptr<mqtt::async_client> client_ptr;
bool g_isConnectedFlag = false;
long long g_functionCallCounter = 0; // Counter untuk melacak urutan panggilan

// --- Fungsi Helper ---
std::string GetNextCallId() {
    return "[" + std::to_string(++g_functionCallCounter) + "]";
}

void DbgPrint(const std::string& func_name, const std::string& call_id, const std::string& msg) {
    std::ostringstream oss;
    oss << call_id << " [MQTT_DLL::" << func_name << "] " << msg << "\n";
    //OutputDebugStringA(oss.str().c_str());
}

void LogClientPtrState(const std::string& func_name, const std::string& call_id, const std::string& context_msg) {
    std::ostringstream oss_state;
    oss_state << "CLIENT_STATE_CHECK (" << context_msg << "): client_ptr is " << (client_ptr ? "VALID" : "NULL");
    if (client_ptr) {
        bool is_conn = false;
        try { // Tambahkan try-catch di sini karena is_connected() bisa throw jika state internal buruk
            is_conn = client_ptr->is_connected();
        }
        catch (const std::exception& e) {
            DbgPrint(func_name, call_id, "LogClientPtrState: Exception checking client_ptr->is_connected(): " + std::string(e.what()));
        }
        oss_state << ", client_ptr->is_connected(): " << (is_conn ? "true" : "false");
    }
    oss_state << ", g_isConnectedFlag: " << (g_isConnectedFlag ? "true" : "false");
    DbgPrint(func_name, call_id, oss_state.str());
}

// --- MQTT Callback Listener ---
class mqtt_callback_listener : public virtual mqtt::callback,
    public virtual mqtt::iaction_listener
{
    std::string listener_id_;
public:
    mqtt_callback_listener(std::string id) : listener_id_(std::move(id)) {}
    mqtt_callback_listener() : listener_id_("global_listener") {}


    void connection_lost(const std::string& cause) override {
        std::string call_id = GetNextCallId();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "EVENT: connection_lost. Cause: " + (cause.empty() ? "Unknown" : cause) + ". Setting g_isConnectedFlag = false.");
        g_isConnectedFlag = false;
        LogClientPtrState("mqtt_callback_listener(" + listener_id_ + ")::connection_lost", call_id, "After connection_lost");
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_msg;
        oss_msg << "EVENT: message_arrived. Topic: " << msg->get_topic()
            << ", Payload: " << msg->to_string();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_msg.str());
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_delivery;
        if (token) {
            oss_delivery << "EVENT: delivery_complete for token [" << token->get_message_id()
                << "], RC: " << token->get_return_code();
        }
        else {
            oss_delivery << "EVENT: delivery_complete (null token)";
        }
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_delivery.str());
    }

    void on_failure(const mqtt::token& tok) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_failure;
        oss_failure << "EVENT: on_failure for token [" << tok.get_message_id() << "], type: " << tok.get_type()
            << ", RC: " << tok.get_return_code();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_failure.str());

        if (tok.get_type() == mqtt::token::Type::CONNECT) {
            DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "CONNECT Action FAILED (on_failure). Setting g_isConnectedFlag = false.");
            g_isConnectedFlag = false;
        }
        LogClientPtrState("mqtt_callback_listener(" + listener_id_ + ")::on_failure", call_id, "After on_failure for token " + std::to_string(tok.get_message_id()));
    }

    void on_success(const mqtt::token& tok) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_success;
        oss_success << "EVENT: on_success for token [" << tok.get_message_id() << "], type: " << tok.get_type()
            << ", RC: " << tok.get_return_code();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_success.str());

        if (tok.get_type() == mqtt::token::Type::CONNECT) {
            if (tok.get_return_code() == mqtt::SUCCESS || tok.get_return_code() == 0) {
                DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "CONNECT Action SUCCESS (on_success with RC=0). Setting g_isConnectedFlag = true.");
                g_isConnectedFlag = true;
            }
            else {
                DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "CONNECT Action 'SUCCESS' (on_success) BUT RC is NOT 0 (" + std::to_string(tok.get_return_code()) + "). Flag NOT set to true.");
            }
        }
        else if (tok.get_type() == mqtt::token::Type::DISCONNECT) {
            DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "DISCONNECT Action SUCCESS (on_success). Setting g_isConnectedFlag = false.");
            g_isConnectedFlag = false;
        }
        LogClientPtrState("mqtt_callback_listener(" + listener_id_ + ")::on_success", call_id, "After on_success for token " + std::to_string(tok.get_message_id()));
    }
};

mqtt_callback_listener global_mqtt_cb_listener("global_listener_instance"); // Beri nama instance global

// --- DLL Exported Functions ---

DLL_EXPORT int mqtt_connect(const char* address_c, const char* client_id_c, const char* username_c, const char* password_c) {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_connect";
    DbgPrint(func_name, call_id, "ENTERED. Address: " + std::string(address_c ? address_c : "NULL") + ", ClientID: " + std::string(client_id_c ? client_id_c : "NULL"));
    LogClientPtrState(func_name, call_id, "Start of function");

    g_isConnectedFlag = false; // Selalu reset flag di awal upaya koneksi baru
    DbgPrint(func_name, call_id, "Set g_isConnectedFlag = false");

    try {
        std::string address(address_c ? address_c : "");
        if (address.empty()) {
            DbgPrint(func_name, call_id, "ERROR: Broker address is NULL or empty.");
            return 101; // Kode error custom untuk alamat broker kosong
        }
        std::string client_id_str = (client_id_c && *client_id_c) ? client_id_c : ("cpp_client_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));


        if (client_ptr) { // Tidak perlu client_ptr->is_connected() karena jika ptr ada tapi tidak connect, kita tetap mau buat koneksi baru
            DbgPrint(func_name, call_id, "Previous client_ptr exists. Attempting to disconnect and reset it.");
            LogClientPtrState(func_name, call_id, "Before disconnecting previous client");
            try {
                if (client_ptr->is_connected()) {
                    DbgPrint(func_name, call_id, "Previous client was connected. Calling disconnect().");
                    client_ptr->disconnect()->wait_for(std::chrono::seconds(5)); // Gunakan listener default atau null
                    DbgPrint(func_name, call_id, "Previous client disconnect() completed.");
                }
                else {
                    DbgPrint(func_name, call_id, "Previous client existed but was not connected.");
                }
            }
            catch (const mqtt::exception& disc_ex) {
                DbgPrint(func_name, call_id, "Exception during previous client disconnect: " + std::string(disc_ex.what()) + ", RC: " + std::to_string(disc_ex.get_reason_code()));
            }
            client_ptr.reset();
            DbgPrint(func_name, call_id, "Previous client_ptr has been reset.");
            LogClientPtrState(func_name, call_id, "After resetting previous client");
        }
        else {
            DbgPrint(func_name, call_id, "No previous client_ptr instance. Good to create a new one.");
        }
        // Pastikan g_isConnectedFlag false setelah reset client lama atau jika tidak ada client lama
        g_isConnectedFlag = false;

        DbgPrint(func_name, call_id, "Creating new mqtt::async_client instance for address: " + address + ", clientID: " + client_id_str);
        client_ptr = std::make_unique<mqtt::async_client>(address, client_id_str);
        LogClientPtrState(func_name, call_id, "After std::make_unique<mqtt::async_client>");
        if (!client_ptr) { // Seharusnya tidak terjadi dengan make_unique kecuali OOM
            DbgPrint(func_name, call_id, "FATAL: std::make_unique<mqtt::async_client> returned nullptr!");
            return 102; // Error pembuatan client
        }

        DbgPrint(func_name, call_id, "Setting client callbacks using global_mqtt_cb_listener.");
        client_ptr->set_callback(global_mqtt_cb_listener); // Ini untuk events seperti connection_lost, message_arrived

        mqtt::connect_options conn_opts;
        DbgPrint(func_name, call_id, "Configuring connect_options.");
        if (username_c && *username_c) {
            conn_opts.set_user_name(username_c);
            DbgPrint(func_name, call_id, "Username set.");
        }
        if (password_c && *password_c) {
            conn_opts.set_password(password_c);
            DbgPrint(func_name, call_id, "Password set.");
        }
        conn_opts.set_automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30));
        conn_opts.set_clean_session(true);
        // conn_opts.set_keep_alive_interval(20); // Opsional, bisa membantu

        DbgPrint(func_name, call_id, "Calling client_ptr->connect() with specific action listener (global_mqtt_cb_listener).");
        // Kita menggunakan global_mqtt_cb_listener juga sebagai action listener untuk connect
        mqtt::token_ptr conn_tok_ptr = client_ptr->connect(conn_opts, nullptr, global_mqtt_cb_listener);

        if (!conn_tok_ptr) {
            DbgPrint(func_name, call_id, "ERROR: client_ptr->connect() returned a null token pointer.");
            if (client_ptr) client_ptr.reset(); // Bersihkan jika gagal di sini
            LogClientPtrState(func_name, call_id, "After connect returned null token");
            return 103; // Error token null
        }
        DbgPrint(func_name, call_id, "Connect token created. Message ID (if any): " + std::to_string(conn_tok_ptr->get_message_id()));

        DbgPrint(func_name, call_id, "Waiting for connect token completion (max 10s)...");
        if (!conn_tok_ptr->wait_for(std::chrono::seconds(10))) {
            DbgPrint(func_name, call_id, "Connect token wait_for TIMED OUT.");
            // Meskipun timeout, callback on_success mungkin sudah dipanggil jika koneksi terjadi cepat.
            // Atau on_failure jika gagal sebelum timeout.
            LogClientPtrState(func_name, call_id, "After connect token wait_for TIMEOUT");
            if (g_isConnectedFlag && client_ptr && client_ptr->is_connected()) {
                DbgPrint(func_name, call_id, "TIMEOUT but g_isConnectedFlag is true and client seems connected. Returning SUCCESS (0).");
                return 0;
            }
            DbgPrint(func_name, call_id, "TIMEOUT and not connected. Resetting client and returning TIMEOUT_ERROR (2).");
            if (client_ptr) client_ptr.reset();
            g_isConnectedFlag = false;
            LogClientPtrState(func_name, call_id, "After TIMEOUT and client reset");
            return 2; // Kode error timeout
        }

        DbgPrint(func_name, call_id, "Connect token wait_for COMPLETED.");
        LogClientPtrState(func_name, call_id, "After connect token wait_for COMPLETED");

        int token_rc = conn_tok_ptr->get_return_code();
        DbgPrint(func_name, call_id, "Connect token RC: " + std::to_string(token_rc));

        if (token_rc != mqtt::SUCCESS) {
            DbgPrint(func_name, call_id, "Connect token completed with FAILURE RC: " + std::to_string(token_rc) + ". Error string from token: " + conn_tok_ptr->get_error_message());
            g_isConnectedFlag = false; // Pastikan flag false
            if (client_ptr) client_ptr.reset();
            LogClientPtrState(func_name, call_id, "After connect token FAILURE RC and client reset");
            return token_rc; // Kembalikan kode error dari token
        }

        // Pada titik ini, token RC adalah SUCCESS. Kita sangat bergantung pada g_isConnectedFlag yang di-set oleh callback.
        // Tambahan jeda singkat MUNGKIN membantu jika ada race condition antara wait_for selesai dan callback dieksekusi sepenuhnya di thread lain,
        // meskipun idealnya Paho harus menanganinya.
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // LogClientPtrState(func_name, call_id, "After potential small sleep");


        if (g_isConnectedFlag && client_ptr && client_ptr->is_connected()) {
            DbgPrint(func_name, call_id, "SUCCESS: Token RC=0, g_isConnectedFlag=true, and client_ptr->is_connected()=true.");
            LogClientPtrState(func_name, call_id, "Exiting with SUCCESS (0)");
            return 0;
        }
        else {
            // Ini adalah kondisi yang tidak diharapkan jika token RC=0
            std::ostringstream oss_err;
            oss_err << "UNEXPECTED STATE: Token RC=0, but ";
            if (!g_isConnectedFlag) oss_err << "g_isConnectedFlag is FALSE. ";
            if (!client_ptr) oss_err << "client_ptr is NULL. ";
            else if (!client_ptr->is_connected()) oss_err << "client_ptr->is_connected() is FALSE. ";
            DbgPrint(func_name, call_id, oss_err.str());

            // Coba verifikasi lagi status koneksi dari client_ptr, karena g_isConnectedFlag mungkin belum terupdate oleh callback
            bool current_client_connected_status = false;
            if (client_ptr) {
                try {
                    current_client_connected_status = client_ptr->is_connected();
                }
                catch (const std::exception& e_chk) {
                    DbgPrint(func_name, call_id, "Exception checking is_connected in unexpected state: " + std::string(e_chk.what()));
                }
            }

            if (current_client_connected_status) {
                DbgPrint(func_name, call_id, "Fallback: client_ptr->is_connected() is true. Setting g_isConnectedFlag=true and returning SUCCESS (0).");
                g_isConnectedFlag = true; // Sinkronkan flag
                LogClientPtrState(func_name, call_id, "Exiting with SUCCESS (0) after fallback");
                return 0;
            }

            DbgPrint(func_name, call_id, "Failed to confirm connection despite token RC=0. Resetting client and returning UNEXPECTED_STATE_ERROR (4).");
            if (client_ptr) client_ptr.reset();
            g_isConnectedFlag = false;
            LogClientPtrState(func_name, call_id, "Exiting with UNEXPECTED_STATE_ERROR (4)");
            return 4;
        }

    }
    catch (const mqtt::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (mqtt::exception): " + std::string(e.what()) + ". RC: " + std::to_string(e.get_reason_code()) + ". Error Str: " + e.get_error_str());
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After mqtt::exception and client reset");
        return e.get_reason_code() != 0 ? e.get_reason_code() : 1; // Kembalikan RC dari exception jika ada, atau 1 (generic error)
    }
    catch (const std::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (std::exception): " + std::string(e.what()));
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After std::exception and client reset");
        return 3; // Generic error
    }
    catch (...) {
        DbgPrint(func_name, call_id, "EXCEPTION (unknown type)");
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After unknown exception and client reset");
        return 5; // Generic error for unknown exception
    }
}

DLL_EXPORT int mqtt_publish(const char* topic_c, const char* payload_c, int qos, int retained_int) {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_publish";
    DbgPrint(func_name, call_id, "ENTERED. Topic: " + std::string(topic_c ? topic_c : "NULL") + ", QoS: " + std::to_string(qos) + ", Retained: " + std::to_string(retained_int));
    LogClientPtrState(func_name, call_id, "Start of function");

    if (!g_isConnectedFlag || !client_ptr || !client_ptr->is_connected()) {
        DbgPrint(func_name, call_id, "ERROR: Client not connected or client_ptr invalid. Cannot publish.");
        LogClientPtrState(func_name, call_id, "Client not connected state check"); // Log state saat kondisi ini terpenuhi
        return 2; // Not connected
    }

    if (!topic_c || !*topic_c) {
        DbgPrint(func_name, call_id, "ERROR: Topic is NULL or empty.");
        return 6; // Invalid argument
    }
    if (!payload_c) { // Payload boleh kosong, tapi tidak null pointer
        DbgPrint(func_name, call_id, "ERROR: Payload is NULL.");
        return 7; // Invalid argument
    }


    try {
        std::string topic(topic_c);
        // payload_c bisa jadi string kosong, strlen akan 0.
        // Paho publish menerima (const void* payload, size_t payloadlen)
        // jadi string payload_str(payload_c, strlen(payload_c)) mungkin tidak perlu jika payload_c sudah valid.
        // Cukup gunakan payload_c dan strlen(payload_c) secara langsung.
        size_t payload_len = strlen(payload_c);
        bool retained = (retained_int != 0);

        DbgPrint(func_name, call_id, "Calling client_ptr->publish(). Payload length: " + std::to_string(payload_len));
        // Menggunakan global_mqtt_cb_listener sebagai action listener untuk publish
        mqtt::delivery_token_ptr pub_tok_ptr = client_ptr->publish(topic, payload_c, payload_len, qos, retained, nullptr, global_mqtt_cb_listener);

        if (!pub_tok_ptr) {
            DbgPrint(func_name, call_id, "ERROR: client_ptr->publish() returned a null token pointer.");
            LogClientPtrState(func_name, call_id, "After publish returned null token");
            return 5; // Publish error
        }
        DbgPrint(func_name, call_id, "Publish token created. Message ID: " + std::to_string(pub_tok_ptr->get_message_id()));

        // Untuk QoS 0, token mungkin sudah complete dan RC=0 tanpa perlu wait_for.
        // Untuk QoS 1 & 2, wait_for menunggu ack dari broker.
        if (qos > 0) {
            DbgPrint(func_name, call_id, "Waiting for publish token (QoS > 0) completion (max 5s)...");
            if (!pub_tok_ptr->wait_for(std::chrono::seconds(5))) {
                DbgPrint(func_name, call_id, "Publish token wait_for TIMED OUT.");
                LogClientPtrState(func_name, call_id, "After publish token wait_for TIMEOUT");
                // Jika timeout, delivery_complete mungkin tidak terpanggil, atau terpanggil nanti.
                // RC dari token mungkin belum final.
                // Kita bisa anggap gagal jika timeout untuk QoS > 0.
                return 3; // Timeout
            }
            DbgPrint(func_name, call_id, "Publish token wait_for COMPLETED.");
        }
        else {
            DbgPrint(func_name, call_id, "Publish QoS 0, not waiting for token explicitly. Relying on Paho internal handling.");
            // Untuk QoS 0, Paho mungkin menyelesaikan token secara sinkron atau sangat cepat asinkron.
            // Kita bisa langsung cek RC-nya.
        }

        LogClientPtrState(func_name, call_id, "After publish token wait_for/QoS0 check");
        int token_rc = pub_tok_ptr->get_return_code();
        DbgPrint(func_name, call_id, "Publish token RC: " + std::to_string(token_rc));

        if (token_rc != mqtt::SUCCESS) {
            DbgPrint(func_name, call_id, "Publish token completed with FAILURE RC: " + std::to_string(token_rc) + ". Error string from token: " + pub_tok_ptr->get_error_message());
            return token_rc;
        }

        DbgPrint(func_name, call_id, "SUCCESS: Publish seems successful (token RC=0).");
        LogClientPtrState(func_name, call_id, "Exiting with SUCCESS (0)");
        return 0;

    }
    catch (const mqtt::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (mqtt::exception): " + std::string(e.what()) + ". RC: " + std::to_string(e.get_reason_code()) + ". Error Str: " + e.get_error_str());
        LogClientPtrState(func_name, call_id, "After mqtt::exception");
        // Tidak reset client_ptr di sini, biarkan koneksi tetap ada jika masih bisa
        return e.get_reason_code() != 0 ? e.get_reason_code() : 1;
    }
    catch (const std::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (std::exception): " + std::string(e.what()));
        LogClientPtrState(func_name, call_id, "After std::exception");
        return 4; // Generic error
    }
    catch (...) {
        DbgPrint(func_name, call_id, "EXCEPTION (unknown type)");
        LogClientPtrState(func_name, call_id, "After unknown exception");
        return 8; // Generic error for unknown exception
    }
}

DLL_EXPORT int mqtt_disconnect() {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_disconnect";
    DbgPrint(func_name, call_id, "ENTERED.");
    LogClientPtrState(func_name, call_id, "Start of function");

    if (!client_ptr) {
        DbgPrint(func_name, call_id, "Client was not initialized (client_ptr is NULL). Nothing to disconnect. Setting g_isConnectedFlag = false.");
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "Exiting (client was NULL) with SUCCESS (0)");
        return 0;
    }

    try {
        // Cek apakah sudah terkoneksi sebelum mencoba disconnect
        bool was_connected = false;
        try { was_connected = client_ptr->is_connected(); }
        catch (...) {}

        if (was_connected) {
            DbgPrint(func_name, call_id, "Client is connected. Calling client_ptr->disconnect().");
            // Menggunakan global_mqtt_cb_listener sebagai action listener untuk disconnect
            mqtt::token_ptr disc_tok_ptr = client_ptr->disconnect(nullptr, global_mqtt_cb_listener);
            if (!disc_tok_ptr) {
                DbgPrint(func_name, call_id, "ERROR: client_ptr->disconnect() returned a null token pointer.");
                // Lanjutkan dengan reset client
            }
            else {
                DbgPrint(func_name, call_id, "Disconnect token created. Message ID: " + std::to_string(disc_tok_ptr->get_message_id()));
                DbgPrint(func_name, call_id, "Waiting for disconnect token completion (max 5s)...");
                if (!disc_tok_ptr->wait_for(std::chrono::seconds(5))) {
                    DbgPrint(func_name, call_id, "Disconnect token wait_for TIMED OUT.");
                    // Jika timeout, kita tetap reset client
                }
                else {
                    DbgPrint(func_name, call_id, "Disconnect token wait_for COMPLETED. RC: " + std::to_string(disc_tok_ptr->get_return_code()));
                }
            }
        }
        else {
            DbgPrint(func_name, call_id, "Client_ptr exists but is_connected() is false. No need to call disconnect API, just resetting local state.");
        }

        DbgPrint(func_name, call_id, "Resetting client_ptr and g_isConnectedFlag.");
        client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After client reset, exiting with SUCCESS (0)");
        return 0;

    }
    catch (const mqtt::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (mqtt::exception): " + std::string(e.what()) + ". RC: " + std::to_string(e.get_reason_code()) + ". Error Str: " + e.get_error_str());
        // Tetap reset client jika ada exception saat disconnect
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After mqtt::exception and client reset");
        return e.get_reason_code() != 0 ? e.get_reason_code() : 1;
    }
    catch (const std::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (std::exception): " + std::string(e.what()));
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After std::exception and client reset");
        return 3;
    }
    catch (...) {
        DbgPrint(func_name, call_id, "EXCEPTION (unknown type)");
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After unknown exception and client reset");
        return 5;
    }
}

DLL_EXPORT bool mqtt_is_connected() {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_is_connected";
    DbgPrint(func_name, call_id, "ENTERED.");
    LogClientPtrState(func_name, call_id, "Start of function");

    // Keputusan utama berdasarkan g_isConnectedFlag, tapi verifikasi dengan client_ptr jika ada.
    bool final_status = false;
    if (g_isConnectedFlag) {
        if (client_ptr) {
            try {
                if (client_ptr->is_connected()) {
                    final_status = true;
                    DbgPrint(func_name, call_id, "g_isConnectedFlag is true, client_ptr exists and client_ptr->is_connected() is true. Returning true.");
                }
                else {
                    DbgPrint(func_name, call_id, "WARNING: g_isConnectedFlag is true, but client_ptr->is_connected() is false. State mismatch! Returning false and correcting g_isConnectedFlag.");
                    g_isConnectedFlag = false; // Koreksi flag
                    final_status = false;
                }
            }
            catch (const std::exception& e) {
                DbgPrint(func_name, call_id, "Exception checking client_ptr->is_connected(): " + std::string(e.what()) + ". Assuming not connected. Returning false.");
                g_isConnectedFlag = false; // Koreksi flag jika ada error akses
                final_status = false;
            }
        }
        else {
            DbgPrint(func_name, call_id, "WARNING: g_isConnectedFlag is true, but client_ptr is NULL. State mismatch! Returning false and correcting g_isConnectedFlag.");
            g_isConnectedFlag = false; // Koreksi flag
            final_status = false;
        }
    }
    else {
        // g_isConnectedFlag is false
        DbgPrint(func_name, call_id, "g_isConnectedFlag is false. Verifying with client_ptr (if exists).");
        if (client_ptr) {
            try {
                if (client_ptr->is_connected()) {
                    DbgPrint(func_name, call_id, "WARNING: g_isConnectedFlag is false, but client_ptr->is_connected() is TRUE. State mismatch! Correcting g_isConnectedFlag and returning true.");
                    g_isConnectedFlag = true; // Koreksi flag
                    final_status = true;
                }
                else {
                    DbgPrint(func_name, call_id, "g_isConnectedFlag is false, and client_ptr->is_connected() is false. Consistent. Returning false.");
                    final_status = false;
                }
            }
            catch (const std::exception& e) {
                DbgPrint(func_name, call_id, "Exception checking client_ptr->is_connected() while g_isConnectedFlag false: " + std::string(e.what()) + ". Assuming not connected. Returning false.");
                final_status = false;
            }
        }
        else {
            DbgPrint(func_name, call_id, "g_isConnectedFlag is false, and client_ptr is NULL. Consistent. Returning false.");
            final_status = false;
        }
    }

    LogClientPtrState(func_name, call_id, "End of function, returning: " + std::string(final_status ? "true" : "false"));
    return final_status;
}


// Fungsi tes counter dan HMODULE (jika masih diperlukan untuk debug)
#ifdef _WIN32
HMODULE g_dllModuleHandle = NULL;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    std::string call_id = GetNextCallId();
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_dllModuleHandle = hModule;
        DbgPrint("DllMain", call_id, "DLL_PROCESS_ATTACH. HMODULE: " + (g_dllModuleHandle ? std::to_string(reinterpret_cast<uintptr_t>(g_dllModuleHandle)) : "NULL"));
        break;
    case DLL_THREAD_ATTACH:
        DbgPrint("DllMain", call_id, "DLL_THREAD_ATTACH.");
        break;
    case DLL_THREAD_DETACH:
        DbgPrint("DllMain", call_id, "DLL_THREAD_DETACH.");
        break;
    case DLL_PROCESS_DETACH:
        DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH.");
        if (client_ptr) {
            DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH: client_ptr exists. Attempting to disconnect and reset.");
            // Jangan panggil mqtt_disconnect() dari DllMain karena bisa deadlock atau masalah lain.
            // Cukup reset pointer jika perlu.
            // Operasi disconnect yang melibatkan network sebaiknya tidak di DllMain.
            try {
                if (client_ptr->is_connected()) {
                    DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH: Client was connected. Forcing disconnect (no wait).");
                    // client_ptr->disconnect(); // Panggil versi non-blocking jika ada, atau cukup reset.
                }
            }
            catch (...) {} // Abaikan exception di DllMain
            client_ptr.reset();
            g_isConnectedFlag = false;
            DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH: client_ptr reset.");
        }
        g_dllModuleHandle = NULL;
        break;
    }
    return TRUE;
}

DLL_EXPORT const char* mqtt_get_module_handle_test() {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_get_module_handle_test";
    static char buffer[20]; // Cukup untuk menyimpan alamat hex
    if (g_dllModuleHandle) {
        sprintf_s(buffer, sizeof(buffer), "%p", g_dllModuleHandle);
        DbgPrint(func_name, call_id, "Current HMODULE: " + std::string(buffer));
    }
    else {
        strcpy_s(buffer, sizeof(buffer), "NULL");
        DbgPrint(func_name, call_id, "Current HMODULE is NULL");
    }
    return buffer;
}

#endif // _WIN32

// Variabel global untuk tes counter sederhana (jika masih dipakai)
int g_testCounter = 0;

DLL_EXPORT int mqtt_get_counter() {
    std::string call_id = GetNextCallId();
    DbgPrint("mqtt_get_counter", call_id, "Current g_testCounter is: " + std::to_string(g_testCounter));
    return g_testCounter;
}

DLL_EXPORT void mqtt_increment_counter() {
    std::string call_id = GetNextCallId();
    g_testCounter++;
    DbgPrint("mqtt_increment_counter", call_id, "g_testCounter is now: " + std::to_string(g_testCounter));
}// MQTT.cpp
#include "pch.h" // Pastikan pch.h meng-include semua header yang dibutuhkan seperti iostream, string, vector, memory, chrono, sstream, dan mqtt/async_client.h
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream> // Untuk std::ostringstream

#include "mqtt/async_client.h"

#ifdef _WIN32
#include <windows.h> // Untuk OutputDebugStringA dan HMODULE
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define DLL_EXPORT extern "C"
#define OutputDebugStringA(str) std::cout << str // Fallback untuk non-Windows
typedef void* HMODULE; // Placeholder HMODULE untuk non-Windows
#endif

// Variabel global untuk state MQTT
std::unique_ptr<mqtt::async_client> client_ptr;
bool g_isConnectedFlag = false;
long long g_functionCallCounter = 0; // Counter untuk melacak urutan panggilan

// --- Fungsi Helper ---
std::string GetNextCallId() {
    return "[" + std::to_string(++g_functionCallCounter) + "]";
}

void DbgPrint(const std::string& func_name, const std::string& call_id, const std::string& msg) {
    std::ostringstream oss;
    oss << call_id << " [MQTT_DLL::" << func_name << "] " << msg << "\n";
    //OutputDebugStringA(oss.str().c_str());
}

void LogClientPtrState(const std::string& func_name, const std::string& call_id, const std::string& context_msg) {
    std::ostringstream oss_state;
    oss_state << "CLIENT_STATE_CHECK (" << context_msg << "): client_ptr is " << (client_ptr ? "VALID" : "NULL");
    if (client_ptr) {
        bool is_conn = false;
        try { // Tambahkan try-catch di sini karena is_connected() bisa throw jika state internal buruk
            is_conn = client_ptr->is_connected();
        }
        catch (const std::exception& e) {
            DbgPrint(func_name, call_id, "LogClientPtrState: Exception checking client_ptr->is_connected(): " + std::string(e.what()));
        }
        oss_state << ", client_ptr->is_connected(): " << (is_conn ? "true" : "false");
    }
    oss_state << ", g_isConnectedFlag: " << (g_isConnectedFlag ? "true" : "false");
    DbgPrint(func_name, call_id, oss_state.str());
}

// --- MQTT Callback Listener ---
class mqtt_callback_listener : public virtual mqtt::callback,
    public virtual mqtt::iaction_listener
{
    std::string listener_id_;
public:
    mqtt_callback_listener(std::string id) : listener_id_(std::move(id)) {}
    mqtt_callback_listener() : listener_id_("global_listener") {}


    void connection_lost(const std::string& cause) override {
        std::string call_id = GetNextCallId();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "EVENT: connection_lost. Cause: " + (cause.empty() ? "Unknown" : cause) + ". Setting g_isConnectedFlag = false.");
        g_isConnectedFlag = false;
        LogClientPtrState("mqtt_callback_listener(" + listener_id_ + ")::connection_lost", call_id, "After connection_lost");
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_msg;
        oss_msg << "EVENT: message_arrived. Topic: " << msg->get_topic()
            << ", Payload: " << msg->to_string();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_msg.str());
    }

    void delivery_complete(mqtt::delivery_token_ptr token) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_delivery;
        if (token) {
            oss_delivery << "EVENT: delivery_complete for token [" << token->get_message_id()
                << "], RC: " << token->get_return_code();
        }
        else {
            oss_delivery << "EVENT: delivery_complete (null token)";
        }
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_delivery.str());
    }

    void on_failure(const mqtt::token& tok) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_failure;
        oss_failure << "EVENT: on_failure for token [" << tok.get_message_id() << "], type: " << tok.get_type()
            << ", RC: " << tok.get_return_code();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_failure.str());

        if (tok.get_type() == mqtt::token::Type::CONNECT) {
            DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "CONNECT Action FAILED (on_failure). Setting g_isConnectedFlag = false.");
            g_isConnectedFlag = false;
        }
        LogClientPtrState("mqtt_callback_listener(" + listener_id_ + ")::on_failure", call_id, "After on_failure for token " + std::to_string(tok.get_message_id()));
    }

    void on_success(const mqtt::token& tok) override {
        std::string call_id = GetNextCallId();
        std::ostringstream oss_success;
        oss_success << "EVENT: on_success for token [" << tok.get_message_id() << "], type: " << tok.get_type()
            << ", RC: " << tok.get_return_code();
        DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, oss_success.str());

        if (tok.get_type() == mqtt::token::Type::CONNECT) {
            if (tok.get_return_code() == mqtt::SUCCESS || tok.get_return_code() == 0) {
                DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "CONNECT Action SUCCESS (on_success with RC=0). Setting g_isConnectedFlag = true.");
                g_isConnectedFlag = true;
            }
            else {
                DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "CONNECT Action 'SUCCESS' (on_success) BUT RC is NOT 0 (" + std::to_string(tok.get_return_code()) + "). Flag NOT set to true.");
            }
        }
        else if (tok.get_type() == mqtt::token::Type::DISCONNECT) {
            DbgPrint("mqtt_callback_listener(" + listener_id_ + ")", call_id, "DISCONNECT Action SUCCESS (on_success). Setting g_isConnectedFlag = false.");
            g_isConnectedFlag = false;
        }
        LogClientPtrState("mqtt_callback_listener(" + listener_id_ + ")::on_success", call_id, "After on_success for token " + std::to_string(tok.get_message_id()));
    }
};

mqtt_callback_listener global_mqtt_cb_listener("global_listener_instance"); // Beri nama instance global

// --- DLL Exported Functions ---

DLL_EXPORT int mqtt_connect(const char* address_c, const char* client_id_c, const char* username_c, const char* password_c) {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_connect";
    DbgPrint(func_name, call_id, "ENTERED. Address: " + std::string(address_c ? address_c : "NULL") + ", ClientID: " + std::string(client_id_c ? client_id_c : "NULL"));
    LogClientPtrState(func_name, call_id, "Start of function");

    g_isConnectedFlag = false; // Selalu reset flag di awal upaya koneksi baru
    DbgPrint(func_name, call_id, "Set g_isConnectedFlag = false");

    try {
        std::string address(address_c ? address_c : "");
        if (address.empty()) {
            DbgPrint(func_name, call_id, "ERROR: Broker address is NULL or empty.");
            return 101; // Kode error custom untuk alamat broker kosong
        }
        std::string client_id_str = (client_id_c && *client_id_c) ? client_id_c : ("cpp_client_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));


        if (client_ptr) { // Tidak perlu client_ptr->is_connected() karena jika ptr ada tapi tidak connect, kita tetap mau buat koneksi baru
            DbgPrint(func_name, call_id, "Previous client_ptr exists. Attempting to disconnect and reset it.");
            LogClientPtrState(func_name, call_id, "Before disconnecting previous client");
            try {
                if (client_ptr->is_connected()) {
                    DbgPrint(func_name, call_id, "Previous client was connected. Calling disconnect().");
                    client_ptr->disconnect()->wait_for(std::chrono::seconds(5)); // Gunakan listener default atau null
                    DbgPrint(func_name, call_id, "Previous client disconnect() completed.");
                }
                else {
                    DbgPrint(func_name, call_id, "Previous client existed but was not connected.");
                }
            }
            catch (const mqtt::exception& disc_ex) {
                DbgPrint(func_name, call_id, "Exception during previous client disconnect: " + std::string(disc_ex.what()) + ", RC: " + std::to_string(disc_ex.get_reason_code()));
            }
            client_ptr.reset();
            DbgPrint(func_name, call_id, "Previous client_ptr has been reset.");
            LogClientPtrState(func_name, call_id, "After resetting previous client");
        }
        else {
            DbgPrint(func_name, call_id, "No previous client_ptr instance. Good to create a new one.");
        }
        // Pastikan g_isConnectedFlag false setelah reset client lama atau jika tidak ada client lama
        g_isConnectedFlag = false;

        DbgPrint(func_name, call_id, "Creating new mqtt::async_client instance for address: " + address + ", clientID: " + client_id_str);
        client_ptr = std::make_unique<mqtt::async_client>(address, client_id_str);
        LogClientPtrState(func_name, call_id, "After std::make_unique<mqtt::async_client>");
        if (!client_ptr) { // Seharusnya tidak terjadi dengan make_unique kecuali OOM
            DbgPrint(func_name, call_id, "FATAL: std::make_unique<mqtt::async_client> returned nullptr!");
            return 102; // Error pembuatan client
        }

        DbgPrint(func_name, call_id, "Setting client callbacks using global_mqtt_cb_listener.");
        client_ptr->set_callback(global_mqtt_cb_listener); // Ini untuk events seperti connection_lost, message_arrived

        mqtt::connect_options conn_opts;
        DbgPrint(func_name, call_id, "Configuring connect_options.");
        if (username_c && *username_c) {
            conn_opts.set_user_name(username_c);
            DbgPrint(func_name, call_id, "Username set.");
        }
        if (password_c && *password_c) {
            conn_opts.set_password(password_c);
            DbgPrint(func_name, call_id, "Password set.");
        }
        conn_opts.set_automatic_reconnect(std::chrono::seconds(2), std::chrono::seconds(30));
        conn_opts.set_clean_session(true);
        // conn_opts.set_keep_alive_interval(20); // Opsional, bisa membantu

        DbgPrint(func_name, call_id, "Calling client_ptr->connect() with specific action listener (global_mqtt_cb_listener).");
        // Kita menggunakan global_mqtt_cb_listener juga sebagai action listener untuk connect
        mqtt::token_ptr conn_tok_ptr = client_ptr->connect(conn_opts, nullptr, global_mqtt_cb_listener);

        if (!conn_tok_ptr) {
            DbgPrint(func_name, call_id, "ERROR: client_ptr->connect() returned a null token pointer.");
            if (client_ptr) client_ptr.reset(); // Bersihkan jika gagal di sini
            LogClientPtrState(func_name, call_id, "After connect returned null token");
            return 103; // Error token null
        }
        DbgPrint(func_name, call_id, "Connect token created. Message ID (if any): " + std::to_string(conn_tok_ptr->get_message_id()));

        DbgPrint(func_name, call_id, "Waiting for connect token completion (max 10s)...");
        if (!conn_tok_ptr->wait_for(std::chrono::seconds(10))) {
            DbgPrint(func_name, call_id, "Connect token wait_for TIMED OUT.");
            // Meskipun timeout, callback on_success mungkin sudah dipanggil jika koneksi terjadi cepat.
            // Atau on_failure jika gagal sebelum timeout.
            LogClientPtrState(func_name, call_id, "After connect token wait_for TIMEOUT");
            if (g_isConnectedFlag && client_ptr && client_ptr->is_connected()) {
                DbgPrint(func_name, call_id, "TIMEOUT but g_isConnectedFlag is true and client seems connected. Returning SUCCESS (0).");
                return 0;
            }
            DbgPrint(func_name, call_id, "TIMEOUT and not connected. Resetting client and returning TIMEOUT_ERROR (2).");
            if (client_ptr) client_ptr.reset();
            g_isConnectedFlag = false;
            LogClientPtrState(func_name, call_id, "After TIMEOUT and client reset");
            return 2; // Kode error timeout
        }

        DbgPrint(func_name, call_id, "Connect token wait_for COMPLETED.");
        LogClientPtrState(func_name, call_id, "After connect token wait_for COMPLETED");

        int token_rc = conn_tok_ptr->get_return_code();
        DbgPrint(func_name, call_id, "Connect token RC: " + std::to_string(token_rc));

        if (token_rc != mqtt::SUCCESS) {
            DbgPrint(func_name, call_id, "Connect token completed with FAILURE RC: " + std::to_string(token_rc) + ". Error string from token: " + conn_tok_ptr->get_error_message());
            g_isConnectedFlag = false; // Pastikan flag false
            if (client_ptr) client_ptr.reset();
            LogClientPtrState(func_name, call_id, "After connect token FAILURE RC and client reset");
            return token_rc; // Kembalikan kode error dari token
        }

        // Pada titik ini, token RC adalah SUCCESS. Kita sangat bergantung pada g_isConnectedFlag yang di-set oleh callback.
        // Tambahan jeda singkat MUNGKIN membantu jika ada race condition antara wait_for selesai dan callback dieksekusi sepenuhnya di thread lain,
        // meskipun idealnya Paho harus menanganinya.
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // LogClientPtrState(func_name, call_id, "After potential small sleep");


        if (g_isConnectedFlag && client_ptr && client_ptr->is_connected()) {
            DbgPrint(func_name, call_id, "SUCCESS: Token RC=0, g_isConnectedFlag=true, and client_ptr->is_connected()=true.");
            LogClientPtrState(func_name, call_id, "Exiting with SUCCESS (0)");
            return 0;
        }
        else {
            // Ini adalah kondisi yang tidak diharapkan jika token RC=0
            std::ostringstream oss_err;
            oss_err << "UNEXPECTED STATE: Token RC=0, but ";
            if (!g_isConnectedFlag) oss_err << "g_isConnectedFlag is FALSE. ";
            if (!client_ptr) oss_err << "client_ptr is NULL. ";
            else if (!client_ptr->is_connected()) oss_err << "client_ptr->is_connected() is FALSE. ";
            DbgPrint(func_name, call_id, oss_err.str());

            // Coba verifikasi lagi status koneksi dari client_ptr, karena g_isConnectedFlag mungkin belum terupdate oleh callback
            bool current_client_connected_status = false;
            if (client_ptr) {
                try {
                    current_client_connected_status = client_ptr->is_connected();
                }
                catch (const std::exception& e_chk) {
                    DbgPrint(func_name, call_id, "Exception checking is_connected in unexpected state: " + std::string(e_chk.what()));
                }
            }

            if (current_client_connected_status) {
                DbgPrint(func_name, call_id, "Fallback: client_ptr->is_connected() is true. Setting g_isConnectedFlag=true and returning SUCCESS (0).");
                g_isConnectedFlag = true; // Sinkronkan flag
                LogClientPtrState(func_name, call_id, "Exiting with SUCCESS (0) after fallback");
                return 0;
            }

            DbgPrint(func_name, call_id, "Failed to confirm connection despite token RC=0. Resetting client and returning UNEXPECTED_STATE_ERROR (4).");
            if (client_ptr) client_ptr.reset();
            g_isConnectedFlag = false;
            LogClientPtrState(func_name, call_id, "Exiting with UNEXPECTED_STATE_ERROR (4)");
            return 4;
        }

    }
    catch (const mqtt::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (mqtt::exception): " + std::string(e.what()) + ". RC: " + std::to_string(e.get_reason_code()) + ". Error Str: " + e.get_error_str());
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After mqtt::exception and client reset");
        return e.get_reason_code() != 0 ? e.get_reason_code() : 1; // Kembalikan RC dari exception jika ada, atau 1 (generic error)
    }
    catch (const std::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (std::exception): " + std::string(e.what()));
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After std::exception and client reset");
        return 3; // Generic error
    }
    catch (...) {
        DbgPrint(func_name, call_id, "EXCEPTION (unknown type)");
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After unknown exception and client reset");
        return 5; // Generic error for unknown exception
    }
}

DLL_EXPORT int mqtt_publish(const char* topic_c, const char* payload_c, int qos, int retained_int) {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_publish";
    DbgPrint(func_name, call_id, "ENTERED. Topic: " + std::string(topic_c ? topic_c : "NULL") + ", QoS: " + std::to_string(qos) + ", Retained: " + std::to_string(retained_int));
    LogClientPtrState(func_name, call_id, "Start of function");

    if (!g_isConnectedFlag || !client_ptr || !client_ptr->is_connected()) {
        DbgPrint(func_name, call_id, "ERROR: Client not connected or client_ptr invalid. Cannot publish.");
        LogClientPtrState(func_name, call_id, "Client not connected state check"); // Log state saat kondisi ini terpenuhi
        return 2; // Not connected
    }

    if (!topic_c || !*topic_c) {
        DbgPrint(func_name, call_id, "ERROR: Topic is NULL or empty.");
        return 6; // Invalid argument
    }
    if (!payload_c) { // Payload boleh kosong, tapi tidak null pointer
        DbgPrint(func_name, call_id, "ERROR: Payload is NULL.");
        return 7; // Invalid argument
    }


    try {
        std::string topic(topic_c);
        // payload_c bisa jadi string kosong, strlen akan 0.
        // Paho publish menerima (const void* payload, size_t payloadlen)
        // jadi string payload_str(payload_c, strlen(payload_c)) mungkin tidak perlu jika payload_c sudah valid.
        // Cukup gunakan payload_c dan strlen(payload_c) secara langsung.
        size_t payload_len = strlen(payload_c);
        bool retained = (retained_int != 0);

        DbgPrint(func_name, call_id, "Calling client_ptr->publish(). Payload length: " + std::to_string(payload_len));
        // Menggunakan global_mqtt_cb_listener sebagai action listener untuk publish
        mqtt::delivery_token_ptr pub_tok_ptr = client_ptr->publish(topic, payload_c, payload_len, qos, retained, nullptr, global_mqtt_cb_listener);

        if (!pub_tok_ptr) {
            DbgPrint(func_name, call_id, "ERROR: client_ptr->publish() returned a null token pointer.");
            LogClientPtrState(func_name, call_id, "After publish returned null token");
            return 5; // Publish error
        }
        DbgPrint(func_name, call_id, "Publish token created. Message ID: " + std::to_string(pub_tok_ptr->get_message_id()));

        // Untuk QoS 0, token mungkin sudah complete dan RC=0 tanpa perlu wait_for.
        // Untuk QoS 1 & 2, wait_for menunggu ack dari broker.
        if (qos > 0) {
            DbgPrint(func_name, call_id, "Waiting for publish token (QoS > 0) completion (max 5s)...");
            if (!pub_tok_ptr->wait_for(std::chrono::seconds(5))) {
                DbgPrint(func_name, call_id, "Publish token wait_for TIMED OUT.");
                LogClientPtrState(func_name, call_id, "After publish token wait_for TIMEOUT");
                // Jika timeout, delivery_complete mungkin tidak terpanggil, atau terpanggil nanti.
                // RC dari token mungkin belum final.
                // Kita bisa anggap gagal jika timeout untuk QoS > 0.
                return 3; // Timeout
            }
            DbgPrint(func_name, call_id, "Publish token wait_for COMPLETED.");
        }
        else {
            DbgPrint(func_name, call_id, "Publish QoS 0, not waiting for token explicitly. Relying on Paho internal handling.");
            // Untuk QoS 0, Paho mungkin menyelesaikan token secara sinkron atau sangat cepat asinkron.
            // Kita bisa langsung cek RC-nya.
        }

        LogClientPtrState(func_name, call_id, "After publish token wait_for/QoS0 check");
        int token_rc = pub_tok_ptr->get_return_code();
        DbgPrint(func_name, call_id, "Publish token RC: " + std::to_string(token_rc));

        if (token_rc != mqtt::SUCCESS) {
            DbgPrint(func_name, call_id, "Publish token completed with FAILURE RC: " + std::to_string(token_rc) + ". Error string from token: " + pub_tok_ptr->get_error_message());
            return token_rc;
        }

        DbgPrint(func_name, call_id, "SUCCESS: Publish seems successful (token RC=0).");
        LogClientPtrState(func_name, call_id, "Exiting with SUCCESS (0)");
        return 0;

    }
    catch (const mqtt::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (mqtt::exception): " + std::string(e.what()) + ". RC: " + std::to_string(e.get_reason_code()) + ". Error Str: " + e.get_error_str());
        LogClientPtrState(func_name, call_id, "After mqtt::exception");
        // Tidak reset client_ptr di sini, biarkan koneksi tetap ada jika masih bisa
        return e.get_reason_code() != 0 ? e.get_reason_code() : 1;
    }
    catch (const std::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (std::exception): " + std::string(e.what()));
        LogClientPtrState(func_name, call_id, "After std::exception");
        return 4; // Generic error
    }
    catch (...) {
        DbgPrint(func_name, call_id, "EXCEPTION (unknown type)");
        LogClientPtrState(func_name, call_id, "After unknown exception");
        return 8; // Generic error for unknown exception
    }
}

DLL_EXPORT int mqtt_disconnect() {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_disconnect";
    DbgPrint(func_name, call_id, "ENTERED.");
    LogClientPtrState(func_name, call_id, "Start of function");

    if (!client_ptr) {
        DbgPrint(func_name, call_id, "Client was not initialized (client_ptr is NULL). Nothing to disconnect. Setting g_isConnectedFlag = false.");
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "Exiting (client was NULL) with SUCCESS (0)");
        return 0;
    }

    try {
        // Cek apakah sudah terkoneksi sebelum mencoba disconnect
        bool was_connected = false;
        try { was_connected = client_ptr->is_connected(); }
        catch (...) {}

        if (was_connected) {
            DbgPrint(func_name, call_id, "Client is connected. Calling client_ptr->disconnect().");
            // Menggunakan global_mqtt_cb_listener sebagai action listener untuk disconnect
            mqtt::token_ptr disc_tok_ptr = client_ptr->disconnect(nullptr, global_mqtt_cb_listener);
            if (!disc_tok_ptr) {
                DbgPrint(func_name, call_id, "ERROR: client_ptr->disconnect() returned a null token pointer.");
                // Lanjutkan dengan reset client
            }
            else {
                DbgPrint(func_name, call_id, "Disconnect token created. Message ID: " + std::to_string(disc_tok_ptr->get_message_id()));
                DbgPrint(func_name, call_id, "Waiting for disconnect token completion (max 5s)...");
                if (!disc_tok_ptr->wait_for(std::chrono::seconds(5))) {
                    DbgPrint(func_name, call_id, "Disconnect token wait_for TIMED OUT.");
                    // Jika timeout, kita tetap reset client
                }
                else {
                    DbgPrint(func_name, call_id, "Disconnect token wait_for COMPLETED. RC: " + std::to_string(disc_tok_ptr->get_return_code()));
                }
            }
        }
        else {
            DbgPrint(func_name, call_id, "Client_ptr exists but is_connected() is false. No need to call disconnect API, just resetting local state.");
        }

        DbgPrint(func_name, call_id, "Resetting client_ptr and g_isConnectedFlag.");
        client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After client reset, exiting with SUCCESS (0)");
        return 0;

    }
    catch (const mqtt::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (mqtt::exception): " + std::string(e.what()) + ". RC: " + std::to_string(e.get_reason_code()) + ". Error Str: " + e.get_error_str());
        // Tetap reset client jika ada exception saat disconnect
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After mqtt::exception and client reset");
        return e.get_reason_code() != 0 ? e.get_reason_code() : 1;
    }
    catch (const std::exception& e) {
        DbgPrint(func_name, call_id, "EXCEPTION (std::exception): " + std::string(e.what()));
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After std::exception and client reset");
        return 3;
    }
    catch (...) {
        DbgPrint(func_name, call_id, "EXCEPTION (unknown type)");
        if (client_ptr) client_ptr.reset();
        g_isConnectedFlag = false;
        LogClientPtrState(func_name, call_id, "After unknown exception and client reset");
        return 5;
    }
}

DLL_EXPORT bool mqtt_is_connected() {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_is_connected";
    DbgPrint(func_name, call_id, "ENTERED.");
    LogClientPtrState(func_name, call_id, "Start of function");

    // Keputusan utama berdasarkan g_isConnectedFlag, tapi verifikasi dengan client_ptr jika ada.
    bool final_status = false;
    if (g_isConnectedFlag) {
        if (client_ptr) {
            try {
                if (client_ptr->is_connected()) {
                    final_status = true;
                    DbgPrint(func_name, call_id, "g_isConnectedFlag is true, client_ptr exists and client_ptr->is_connected() is true. Returning true.");
                }
                else {
                    DbgPrint(func_name, call_id, "WARNING: g_isConnectedFlag is true, but client_ptr->is_connected() is false. State mismatch! Returning false and correcting g_isConnectedFlag.");
                    g_isConnectedFlag = false; // Koreksi flag
                    final_status = false;
                }
            }
            catch (const std::exception& e) {
                DbgPrint(func_name, call_id, "Exception checking client_ptr->is_connected(): " + std::string(e.what()) + ". Assuming not connected. Returning false.");
                g_isConnectedFlag = false; // Koreksi flag jika ada error akses
                final_status = false;
            }
        }
        else {
            DbgPrint(func_name, call_id, "WARNING: g_isConnectedFlag is true, but client_ptr is NULL. State mismatch! Returning false and correcting g_isConnectedFlag.");
            g_isConnectedFlag = false; // Koreksi flag
            final_status = false;
        }
    }
    else {
        // g_isConnectedFlag is false
        DbgPrint(func_name, call_id, "g_isConnectedFlag is false. Verifying with client_ptr (if exists).");
        if (client_ptr) {
            try {
                if (client_ptr->is_connected()) {
                    DbgPrint(func_name, call_id, "WARNING: g_isConnectedFlag is false, but client_ptr->is_connected() is TRUE. State mismatch! Correcting g_isConnectedFlag and returning true.");
                    g_isConnectedFlag = true; // Koreksi flag
                    final_status = true;
                }
                else {
                    DbgPrint(func_name, call_id, "g_isConnectedFlag is false, and client_ptr->is_connected() is false. Consistent. Returning false.");
                    final_status = false;
                }
            }
            catch (const std::exception& e) {
                DbgPrint(func_name, call_id, "Exception checking client_ptr->is_connected() while g_isConnectedFlag false: " + std::string(e.what()) + ". Assuming not connected. Returning false.");
                final_status = false;
            }
        }
        else {
            DbgPrint(func_name, call_id, "g_isConnectedFlag is false, and client_ptr is NULL. Consistent. Returning false.");
            final_status = false;
        }
    }

    LogClientPtrState(func_name, call_id, "End of function, returning: " + std::string(final_status ? "true" : "false"));
    return final_status;
}


// Fungsi tes counter dan HMODULE (jika masih diperlukan untuk debug)
#ifdef _WIN32
HMODULE g_dllModuleHandle = NULL;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    std::string call_id = GetNextCallId();
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_dllModuleHandle = hModule;
        DbgPrint("DllMain", call_id, "DLL_PROCESS_ATTACH. HMODULE: " + (g_dllModuleHandle ? std::to_string(reinterpret_cast<uintptr_t>(g_dllModuleHandle)) : "NULL"));
        break;
    case DLL_THREAD_ATTACH:
        DbgPrint("DllMain", call_id, "DLL_THREAD_ATTACH.");
        break;
    case DLL_THREAD_DETACH:
        DbgPrint("DllMain", call_id, "DLL_THREAD_DETACH.");
        break;
    case DLL_PROCESS_DETACH:
        DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH.");
        if (client_ptr) {
            DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH: client_ptr exists. Attempting to disconnect and reset.");
            // Jangan panggil mqtt_disconnect() dari DllMain karena bisa deadlock atau masalah lain.
            // Cukup reset pointer jika perlu.
            // Operasi disconnect yang melibatkan network sebaiknya tidak di DllMain.
            try {
                if (client_ptr->is_connected()) {
                    DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH: Client was connected. Forcing disconnect (no wait).");
                    // client_ptr->disconnect(); // Panggil versi non-blocking jika ada, atau cukup reset.
                }
            }
            catch (...) {} // Abaikan exception di DllMain
            client_ptr.reset();
            g_isConnectedFlag = false;
            DbgPrint("DllMain", call_id, "DLL_PROCESS_DETACH: client_ptr reset.");
        }
        g_dllModuleHandle = NULL;
        break;
    }
    return TRUE;
}

DLL_EXPORT const char* mqtt_get_module_handle_test() {
    std::string call_id = GetNextCallId();
    const std::string func_name = "mqtt_get_module_handle_test";
    static char buffer[20]; // Cukup untuk menyimpan alamat hex
    if (g_dllModuleHandle) {
        sprintf_s(buffer, sizeof(buffer), "%p", g_dllModuleHandle);
        DbgPrint(func_name, call_id, "Current HMODULE: " + std::string(buffer));
    }
    else {
        strcpy_s(buffer, sizeof(buffer), "NULL");
        DbgPrint(func_name, call_id, "Current HMODULE is NULL");
    }
    return buffer;
}

#endif // _WIN32

// Variabel global untuk tes counter sederhana (jika masih dipakai)
int g_testCounter = 0;

DLL_EXPORT int mqtt_get_counter() {
    std::string call_id = GetNextCallId();
    DbgPrint("mqtt_get_counter", call_id, "Current g_testCounter is: " + std::to_string(g_testCounter));
    return g_testCounter;
}

DLL_EXPORT void mqtt_increment_counter() {
    std::string call_id = GetNextCallId();
    g_testCounter++;
    DbgPrint("mqtt_increment_counter", call_id, "g_testCounter is now: " + std::to_string(g_testCounter));
}