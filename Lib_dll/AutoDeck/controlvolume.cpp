// controlvolume.cpp
#include "pch.h" // Pastikan header ini ada dan sesuai dengan proyek Visual Studio Anda
#include <windows.h>
#include <mmdeviceapi.h>    // Untuk IMMDeviceEnumerator, IMMDevice
#include <audiopolicy.h>    // Untuk IAudioSessionManager2, IAudioSessionEnumerator, IAudioSessionControl, IAudioSessionControl2
#include <endpointvolume.h> // Untuk IAudioEndpointVolume, ISimpleAudioVolume
#include <functiondiscoverykeys_devpkey.h> // Untuk PKEY_Device_FriendlyName (jika diperlukan di masa depan)
#include <comdef.h>         // Untuk _com_error (jika digunakan untuk error handling COM yang lebih detail)
#include <psapi.h>          // Untuk GetModuleBaseName
#include <string>       // Untuk std::wstring
#include <algorithm>    // Untuk std::transform
#include <cwctype>      // Untuk ::towlower

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Psapi.lib") // Diperlukan untuk GetModuleBaseName

// Fungsi helper untuk logging, bisa Anda sesuaikan
void LogDebug(const wchar_t* format, ...) {
    wchar_t buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), format, args);
    va_end(args);
    OutputDebugStringW(buffer);
}



extern "C" __declspec(dllexport) void __stdcall ListAudioSessions()
{
    // Gunakan COINIT_APARTMENTTHREADED karena banyak API UI/Audio lebih menyukainya
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] ListAudioSessions: Failed to initialize COM. HRESULT: 0x%08X\n", hr);
        return;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioSessionManager2* pSessionManager = nullptr;
    IAudioSessionEnumerator* pSessionEnumerator = nullptr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] ListAudioSessions: Failed to create device enumerator. HRESULT: 0x%08X\n", hr);
        CoUninitialize();
        return;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] ListAudioSessions: Failed to get default audio endpoint. HRESULT: 0x%08X\n", hr);
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] ListAudioSessions: Failed to activate session manager. HRESULT: 0x%08X\n", hr);
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] ListAudioSessions: Failed to get session enumerator. HRESULT: 0x%08X\n", hr);
        if (pSessionManager) pSessionManager->Release();
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    int sessionCount = 0;
    pSessionEnumerator->GetCount(&sessionCount);
    LogDebug(L"[DEBUG] ListAudioSessions: Session count: %d\n", sessionCount);

    for (int i = 0; i < sessionCount; ++i) {
        IAudioSessionControl* pSessionControl = nullptr;
        IAudioSessionControl2* pSessionControl2 = nullptr;

        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (SUCCEEDED(hr)) {
            hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
            if (SUCCEEDED(hr)) {
                DWORD pid = 0;
                hr = pSessionControl2->GetProcessId(&pid);
                if (SUCCEEDED(hr) && pid != 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    if (hProcess) {
                        wchar_t exeName[MAX_PATH] = L"<unknown>";
                        if (GetModuleBaseName(hProcess, NULL, exeName, MAX_PATH) > 0) {
                            // Berhasil mendapatkan nama exe
                        }
                        CloseHandle(hProcess);
                        LogDebug(L"[DEBUG] ListAudioSessions: Session %d - PID: %lu - EXE: %s\n", i, pid, exeName);
                    }
                    else {
                        LogDebug(L"[DEBUG] ListAudioSessions: Session %d - PID: %lu - Could not open process.\n", i, pid);
                    }
                }
                pSessionControl2->Release();
            }
            pSessionControl->Release();
        }
    }

    if (pSessionEnumerator) pSessionEnumerator->Release();
    if (pSessionManager) pSessionManager->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
}

extern "C" __declspec(dllexport) void __stdcall SetAppVolume(const wchar_t* targetExe, int volumePercent)
{
    if (!targetExe) return;

    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    float volumeNormalized = static_cast<float>(volumePercent) / 100.0f;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool comInitialized = SUCCEEDED(hr);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioSessionManager2* pSessionManager = nullptr;
    IAudioSessionEnumerator* pSessionEnumerator = nullptr;

    do {
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) break;

        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
        if (FAILED(hr)) break;

        hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);
        if (FAILED(hr)) break;

        hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
        if (FAILED(hr)) break;

        int sessionCount = 0;
        pSessionEnumerator->GetCount(&sessionCount);

        for (int i = 0; i < sessionCount; ++i) {
            IAudioSessionControl* pSessionControl = nullptr;
            IAudioSessionControl2* pSessionControl2 = nullptr;
            ISimpleAudioVolume* pVolume = nullptr;

            hr = pSessionEnumerator->GetSession(i, &pSessionControl);
            if (FAILED(hr)) continue;

            hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
            if (FAILED(hr)) {
                pSessionControl->Release();
                continue;
            }

            DWORD pid = 0;
            pSessionControl2->GetProcessId(&pid);

            if (pid != 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                if (hProcess) {
                    wchar_t exeName[MAX_PATH] = L"";
                    if (GetModuleBaseName(hProcess, NULL, exeName, MAX_PATH) > 0) {
                        if (_wcsicmp(exeName, targetExe) == 0) {
                            hr = pSessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVolume);
                            if (SUCCEEDED(hr)) {
                                pVolume->SetMasterVolume(volumeNormalized, NULL);
                                pVolume->Release();
                                CloseHandle(hProcess);
                                pSessionControl2->Release();
                                pSessionControl->Release();
                                break; // exit on first match
                            }
                        }
                    }
                    CloseHandle(hProcess);
                }
            }

            pSessionControl2->Release();
            pSessionControl->Release();
        }

    } while (false);

    if (pSessionEnumerator) pSessionEnumerator->Release();
    if (pSessionManager) pSessionManager->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();

    if (comInitialized) CoUninitialize();
}


extern "C" __declspec(dllexport) int __stdcall GetMasterVolume(const wchar_t* deviceType)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterVolume: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        return -1; // Indikasi error
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioEndpointVolume* pEndpointVolume = nullptr;
    int volumePercent = -1; // Default ke error

    EDataFlow dataFlow = eRender; // Default ke playback
    if (deviceType != nullptr && deviceType[0] != L'\0') {
        if (_wcsicmp(deviceType, L"playback") == 0) {
            dataFlow = eRender;
            LogDebug(L"[DEBUG] GetMasterVolume: Device type set to Playback (eRender).\n");
        }
        else if (_wcsicmp(deviceType, L"recording") == 0 || _wcsicmp(deviceType, L"capture") == 0) {
            dataFlow = eCapture;
            LogDebug(L"[DEBUG] GetMasterVolume: Device type set to Recording (eCapture).\n");
        }
        else {
            LogDebug(L"[WARNING] GetMasterVolume: Invalid deviceType '%s'. Defaulting to Playback.\n", deviceType);
            // Tetap eRender
        }
    }
    else {
        LogDebug(L"[DEBUG] GetMasterVolume: No device type specified. Defaulting to Playback (eRender).\n");
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterVolume: Failed to create device enumerator. HRESULT: 0x%08X\n", hr);
        CoUninitialize();
        return -1;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterVolume: Failed to get default audio endpoint for dataFlow %d. HRESULT: 0x%08X\n", dataFlow, hr);
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterVolume: Failed to activate IAudioEndpointVolume. HRESULT: 0x%08X\n", hr);
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    float currentVolumeNormalized = 0.0f;
    hr = pEndpointVolume->GetMasterVolumeLevelScalar(&currentVolumeNormalized);
    if (SUCCEEDED(hr)) {
        // Konversi ke 0-100 dan bulatkan
        volumePercent = static_cast<int>(currentVolumeNormalized * 100.0f + 0.5f);
        LogDebug(L"[DEBUG] GetMasterVolume: Retrieved master volume: %.2f (Normalized), %d%% (Percent)\n", currentVolumeNormalized, volumePercent);
    }
    else {
        LogDebug(L"[ERROR] GetMasterVolume: Failed to get master volume level scalar. HRESULT: 0x%08X\n", hr);
        volumePercent = -1; // Set lagi ke error jika GetMasterVolumeLevelScalar gagal
    }

    if (pEndpointVolume) pEndpointVolume->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();

    return volumePercent;
}

extern "C" __declspec(dllexport) void __stdcall SetMasterVolume(int volumePercent, const wchar_t* deviceType)
{
    // Validasi dan konversi volume
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    float volumeNormalized = static_cast<float>(volumePercent) / 100.0f;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterVolume: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        return;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioEndpointVolume* pEndpointVolume = nullptr;

    EDataFlow dataFlow = eRender; // Default ke playback
    if (deviceType != nullptr && deviceType[0] != L'\0') { // Cek apakah string tidak null dan tidak kosong
        if (_wcsicmp(deviceType, L"playback") == 0) {
            dataFlow = eRender;
            LogDebug(L"[DEBUG] SetMasterVolume: Device type set to Playback (eRender).\n");
        }
        else if (_wcsicmp(deviceType, L"recording") == 0 || _wcsicmp(deviceType, L"capture") == 0) {
            dataFlow = eCapture;
            LogDebug(L"[DEBUG] SetMasterVolume: Device type set to Recording (eCapture).\n");
        }
        else {
            LogDebug(L"[WARNING] SetMasterVolume: Invalid deviceType '%s'. Defaulting to Playback.\n", deviceType);
            // Tetap eRender
        }
    }
    else {
        LogDebug(L"[DEBUG] SetMasterVolume: No device type specified or empty. Defaulting to Playback (eRender).\n");
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterVolume: Failed to create device enumerator. HRESULT: 0x%08X\n", hr);
        CoUninitialize();
        return;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterVolume: Failed to get default audio endpoint for dataFlow %d. HRESULT: 0x%08X\n", dataFlow, hr);
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterVolume: Failed to activate IAudioEndpointVolume. HRESULT: 0x%08X\n", hr);
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pEndpointVolume->SetMasterVolumeLevelScalar(volumeNormalized, NULL);
    if (SUCCEEDED(hr)) {
        LogDebug(L"[DEBUG] SetMasterVolume: Set master volume for dataFlow %d to %d%% (%.2f Normalized).\n", dataFlow, volumePercent, volumeNormalized);
    }
    else {
        LogDebug(L"[ERROR] SetMasterVolume: Failed to set master volume level scalar. HRESULT: 0x%08X\n", hr);
    }

    if (pEndpointVolume) pEndpointVolume->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
}

extern "C" __declspec(dllexport) BOOL __stdcall GetMasterMute(const wchar_t* deviceType)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterMute: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        // Mengembalikan TRUE (muted) sebagai indikasi error bisa jadi ambigu.
        // Pertimbangkan untuk memiliki cara error handling yang lebih jelas jika diperlukan,
        // atau dokumentasikan bahwa TRUE bisa berarti error atau memang muted.
        // Untuk konsistensi dengan return value lain yang -1, mungkin lebih baik
        // jika fungsi ini bisa return -1, 0, 1, tapi AHK mengharapkan bool.
        // Untuk sekarang, kita asumsikan error berarti tidak bisa menentukan, jadi false (not muted)
        // atau biarkan default ke apa pun yang akan menjadi nilai bMuted jika gagal.
        // Mari kita default ke TRUE (muted) pada error CoInitialize, karena itu state "aman".
        return TRUE;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioEndpointVolume* pEndpointVolume = nullptr;
    BOOL bMuted = TRUE; // Default ke muted, terutama jika ada error di bawah

    EDataFlow dataFlow = eRender; // Default ke playback
    if (deviceType != nullptr && deviceType[0] != L'\0') {
        if (_wcsicmp(deviceType, L"playback") == 0) {
            dataFlow = eRender;
            LogDebug(L"[DEBUG] GetMasterMute: Device type set to Playback (eRender).\n");
        }
        else if (_wcsicmp(deviceType, L"recording") == 0 || _wcsicmp(deviceType, L"capture") == 0) {
            dataFlow = eCapture;
            LogDebug(L"[DEBUG] GetMasterMute: Device type set to Recording (eCapture).\n");
        }
        else {
            LogDebug(L"[WARNING] GetMasterMute: Invalid deviceType '%s'. Defaulting to Playback.\n", deviceType);
            // Tetap eRender
        }
    }
    else {
        LogDebug(L"[DEBUG] GetMasterMute: No device type specified or empty. Defaulting to Playback (eRender).\n");
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterMute: Failed to create device enumerator. HRESULT: 0x%08X\n", hr);
        CoUninitialize();
        return TRUE; // Default error state
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterMute: Failed to get default audio endpoint for dataFlow %d. HRESULT: 0x%08X\n", dataFlow, hr);
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return TRUE; // Default error state
    }

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetMasterMute: Failed to activate IAudioEndpointVolume. HRESULT: 0x%08X\n", hr);
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return TRUE; // Default error state
    }

    hr = pEndpointVolume->GetMute(&bMuted);
    if (SUCCEEDED(hr)) {
        LogDebug(L"[DEBUG] GetMasterMute: Retrieved mute state for dataFlow %d: %s.\n", dataFlow, bMuted ? L"Muted (TRUE)" : L"Not Muted (FALSE)");
    }
    else {
        LogDebug(L"[ERROR] GetMasterMute: Failed to get mute state. HRESULT: 0x%08X\n", hr);
        bMuted = TRUE; // Jika gagal mendapatkan state, asumsikan muted sebagai state error.
    }

    if (pEndpointVolume) pEndpointVolume->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();

    return bMuted; // Mengembalikan TRUE jika muted, FALSE jika tidak muted atau error (tergantung logika di atas)
}

extern "C" __declspec(dllexport) void __stdcall SetMasterMute(BOOL bMute, const wchar_t* deviceType)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterMute: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        return;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioEndpointVolume* pEndpointVolume = nullptr;

    EDataFlow dataFlow = eRender; // Default ke playback
    if (deviceType != nullptr && deviceType[0] != L'\0') {
        if (_wcsicmp(deviceType, L"playback") == 0) {
            dataFlow = eRender;
        }
        else if (_wcsicmp(deviceType, L"recording") == 0 || _wcsicmp(deviceType, L"capture") == 0) {
            dataFlow = eCapture;
        }
        else {
            LogDebug(L"[WARNING] SetMasterMute: Invalid deviceType '%s'. Defaulting to Playback.\n", deviceType);
        }
    }
    else {
        LogDebug(L"[DEBUG] SetMasterMute: No device type. Defaulting to Playback for dataFlow %d, mute state: %s.\n", dataFlow, bMute ? L"TRUE" : L"FALSE");
    }


    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterMute: Failed to create device enumerator. HRESULT: 0x%08X\n", hr);
        CoUninitialize();
        return;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDevice);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterMute: Failed to get default audio endpoint. HRESULT: 0x%08X\n", hr);
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pEndpointVolume);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetMasterMute: Failed to activate IAudioEndpointVolume. HRESULT: 0x%08X\n", hr);
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
        return;
    }

    hr = pEndpointVolume->SetMute(bMute, NULL);
    if (SUCCEEDED(hr)) {
        LogDebug(L"[DEBUG] SetMasterMute: Set mute state for dataFlow %d to %s.\n", dataFlow, bMute ? L"Muted (TRUE)" : L"Not Muted (FALSE)");
    }
    else {
        LogDebug(L"[ERROR] SetMasterMute: Failed to set mute state. HRESULT: 0x%08X\n", hr);
    }

    if (pEndpointVolume) pEndpointVolume->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
}

// Helper function to find a specific app session and get an interface
template<typename T>
HRESULT GetAppSessionInterface(const wchar_t* targetExe, T** ppInterface)
{
    if (targetExe == nullptr || ppInterface == nullptr) return E_POINTER;
    *ppInterface = nullptr;

    // CoInitializeEx sudah dipanggil oleh fungsi pemanggil
    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioSessionManager2* pSessionManager = nullptr;
    IAudioSessionEnumerator* pSessionEnumerator = nullptr;
    HRESULT hr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) return hr;

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        pEnumerator->Release();
        return hr;
    }

    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSessionManager);
    if (FAILED(hr)) {
        pDevice->Release();
        pEnumerator->Release();
        return hr;
    }

    hr = pSessionManager->GetSessionEnumerator(&pSessionEnumerator);
    if (FAILED(hr)) {
        pSessionManager->Release();
        pDevice->Release();
        pEnumerator->Release();
        return hr;
    }

    int sessionCount = 0;
    pSessionEnumerator->GetCount(&sessionCount);
    bool found = false;

    for (int i = 0; i < sessionCount; ++i) {
        IAudioSessionControl* pSessionControl = nullptr;
        IAudioSessionControl2* pSessionControl2 = nullptr;

        hr = pSessionEnumerator->GetSession(i, &pSessionControl);
        if (SUCCEEDED(hr)) {
            hr = pSessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
            if (SUCCEEDED(hr)) {
                DWORD pid = 0;
                pSessionControl2->GetProcessId(&pid);
                if (pid != 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
                    if (hProcess) {
                        wchar_t exeName[MAX_PATH] = L"<unknown>";
                        if (GetModuleBaseName(hProcess, NULL, exeName, MAX_PATH) > 0) {
                            if (_wcsicmp(exeName, targetExe) == 0) {
                                hr = pSessionControl2->QueryInterface(__uuidof(T), (void**)ppInterface);
                                if (SUCCEEDED(hr)) {
                                    LogDebug(L"[DEBUG] GetAppSessionInterface: Found %s (PID: %lu) and got interface.\n", targetExe, pid);
                                    found = true;
                                }
                                else {
                                    LogDebug(L"[ERROR] GetAppSessionInterface: Found %s (PID: %lu) but failed to QueryInterface for T. HRESULT: 0x%08X\n", targetExe, pid, hr);
                                }
                                // Jika ingin hanya yang pertama, break di sini
                                // Jika ingin yang "paling aktif", perlu logika IAudioMeterInformation
                            }
                        }
                        CloseHandle(hProcess);
                    }
                }
                pSessionControl2->Release();
            }
            pSessionControl->Release();
        }
        if (found) break; // Hanya ambil interface dari sesi pertama yang cocok
    }

    if (pSessionEnumerator) pSessionEnumerator->Release();
    if (pSessionManager) pSessionManager->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();

    return found ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_FOUND); // Atau E_FAIL jika tidak ditemukan
}


extern "C" __declspec(dllexport) int __stdcall GetAppVolume(const wchar_t* targetExe)
{
    if (targetExe == nullptr) {
        LogDebug(L"[ERROR] GetAppVolume: targetExe cannot be null.\n");
        return -1;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetAppVolume: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        return -1;
    }

    ISimpleAudioVolume* pSimpleVolume = nullptr;
    int volumePercent = -1;

    hr = GetAppSessionInterface(targetExe, &pSimpleVolume);
    if (SUCCEEDED(hr) && pSimpleVolume != nullptr) {
        float fVolume = 0.0f;
        hr = pSimpleVolume->GetMasterVolume(&fVolume);
        if (SUCCEEDED(hr)) {
            volumePercent = static_cast<int>(fVolume * 100.0f + 0.5f);
            LogDebug(L"[DEBUG] GetAppVolume: Volume for %s is %d%% (%.2f).\n", targetExe, volumePercent, fVolume);
        }
        else {
            LogDebug(L"[ERROR] GetAppVolume: Failed to get volume for %s. HRESULT: 0x%08X\n", targetExe, hr);
        }
        pSimpleVolume->Release();
    }
    else {
        LogDebug(L"[INFO] GetAppVolume: Could not find or get ISimpleAudioVolume for %s. HRESULT: 0x%08X\n", targetExe, hr);
    }

    CoUninitialize();
    return volumePercent;
}

extern "C" __declspec(dllexport) BOOL __stdcall GetAppMute(const wchar_t* targetExe)
{
    if (targetExe == nullptr) {
        LogDebug(L"[ERROR] GetAppMute: targetExe cannot be null.\n");
        return TRUE; // Default error state: muted
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] GetAppMute: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        return TRUE;
    }

    ISimpleAudioVolume* pSimpleVolume = nullptr;
    BOOL bMuted = TRUE; // Default ke muted

    hr = GetAppSessionInterface(targetExe, &pSimpleVolume);
    if (SUCCEEDED(hr) && pSimpleVolume != nullptr) {
        hr = pSimpleVolume->GetMute(&bMuted);
        if (SUCCEEDED(hr)) {
            LogDebug(L"[DEBUG] GetAppMute: Mute state for %s is %s.\n", targetExe, bMuted ? L"Muted (TRUE)" : L"Not Muted (FALSE)");
        }
        else {
            LogDebug(L"[ERROR] GetAppMute: Failed to get mute state for %s. HRESULT: 0x%08X\n", targetExe, hr);
            bMuted = TRUE; // Asumsikan muted jika gagal mendapatkan state
        }
        pSimpleVolume->Release();
    }
    else {
        LogDebug(L"[INFO] GetAppMute: Could not find or get ISimpleAudioVolume for %s. HRESULT: 0x%08X\n", targetExe, hr);
    }

    CoUninitialize();
    return bMuted;
}

extern "C" __declspec(dllexport) void __stdcall SetAppMute(BOOL bMute, const wchar_t* targetExe)
{
    if (targetExe == nullptr) {
        LogDebug(L"[ERROR] SetAppMute: targetExe cannot be null.\n");
        return;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        LogDebug(L"[ERROR] SetAppMute: CoInitializeEx failed. HRESULT: 0x%08X\n", hr);
        return;
    }

    ISimpleAudioVolume* pSimpleVolume = nullptr;

    hr = GetAppSessionInterface(targetExe, &pSimpleVolume);
    if (SUCCEEDED(hr) && pSimpleVolume != nullptr) {
        hr = pSimpleVolume->SetMute(bMute, NULL);
        if (SUCCEEDED(hr)) {
            LogDebug(L"[DEBUG] SetAppMute: Set mute state for %s to %s.\n", targetExe, bMute ? L"Muted (TRUE)" : L"Not Muted (FALSE)");
        }
        else {
            LogDebug(L"[ERROR] SetAppMute: Failed to set mute state for %s. HRESULT: 0x%08X\n", targetExe, hr);
        }
        pSimpleVolume->Release();
    }
    else {
        LogDebug(L"[INFO] SetAppMute: Could not find or get ISimpleAudioVolume for %s. HRESULT: 0x%08X\n", targetExe, hr);
    }

    CoUninitialize();
}


