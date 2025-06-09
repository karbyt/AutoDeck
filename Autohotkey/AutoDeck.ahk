#Requires AutoHotkey v2.0
Persistent
SetWorkingDir A_ScriptDir "\library" ; IMPORTANT: Otherwise windows can't find the DLLs and it's dependencies
SendMode "Input"
A_BatchLines := -1

#Include library\ClassSerial.ahk
#Include library\JsRT.ahk
#Include library\Jxon.ahk
#Include Config.ahk



; ==================== GLOBAL PATH =============================
global NIRCMD_PATH := A_ScriptDir "\library\nircmd.exe"
global AUTODECK_DLL_PATH := A_ScriptDir "\library\AutoDeck.dll"
global CONFIG_FILE := A_ScriptDir "\Config.ahk"
global MQTT_PATH := A_ScriptDir "\library\MQTT.dll"
global MQTT_DLL_NAME := "MQTT.dll" ; Nama DLL saja
; =============================================================




; ================= GLOBAL ICONS PATH =========================
; --- PNG ICONS ---
global successIcon := A_ScriptDir "\media\WebSuccess.png"
global failedIcon := A_ScriptDir "\media\WebFailed.png"
global capsIconOn := A_ScriptDir "\media\Lock.png"
global capsIconOff := A_ScriptDir "\media\Unlock.png"
global cableIcon := A_ScriptDir "\media\Cable.png"
global micOffIcon := A_ScriptDir "\media\MicOff.png"
global micOnIcon := A_ScriptDir "\media\MicOn.png"
global speakerOffIcon := A_ScriptDir "\media\SpeakerOff.png"
global speakerOnIcon := A_ScriptDir "\media\SpeakerOn.png"
global speakerIcon := A_ScriptDir "\media\Speaker.png"
global autodeckpngIcon := A_ScriptDir "\media\AutoDeck.png"
global autodeckpngIconSuspended := A_ScriptDir "\media\AutoDeckSuspended.png"
global autodeckpngIconPaused := A_ScriptDir "\media\AutoDeckPaused.png"
global layerIcon := A_ScriptDir "\media\Layer.png"
global mqttIcon := A_ScriptDir "\media\MQTT.png"
global spellcheckIcon := A_ScriptDir "\media\SpellCheck.png"


; --- ICO ICON ---
global aboutIcon := A_ScriptDir "\media\About.ico"
global autodeckicoIcon := A_ScriptDir "\media\AutoDeck.ico"
global bugIcon := A_ScriptDir "\media\Bug.ico"
global configFolderIcon := A_ScriptDir "\media\ConfigFolder.ico"
global editConfigIcon := A_ScriptDir "\media\EditConfig.ico"
global exitIcon := A_ScriptDir "\media\Exit.ico"
global pauseIcon := A_ScriptDir "\media\PauseScript.ico"
global reloadIcon := A_ScriptDir "\media\Reload.ico"
global suspendIcon := A_ScriptDir "\media\Suspend.ico"
global windowSpyIcon := A_ScriptDir "\media\WindowSpy.ico"
; =============================================================





; ===================== DEPENDENCIES CHECKS =========================
; --- FILE CHECK FUNCTION ---
_DependencyCheck(filePath, fileName) {
    if !FileExist(filePath) {
        MsgBox "Depencency file is not found: " fileName "`nPath: " filePath "`nMake sure this file is in the correct directory.", "Dependency Error"
        ExitApp 
    }
    return true
}

; --- ICON CHECKS ---
_DependencyCheck(successIcon, "websuccess.png")
_DependencyCheck(failedIcon, "webfailed.png")
_DependencyCheck(capsIconOn, "lock.png")
_DependencyCheck(capsIconOff, "unlock.png")
_DependencyCheck(pauseIcon, "PauseScript.ico")
_DependencyCheck(cableIcon, "cable.png")
_DependencyCheck(micOffIcon, "MicOff.png")
_DependencyCheck(micOnIcon, "MicOn.png")
_DependencyCheck(speakerOffIcon, "SpeakerOff.png")
_DependencyCheck(speakerOnIcon, "SpeakerOn.png")
_DependencyCheck(speakerIcon, "Speaker.png") 
_DependencyCheck(autodeckpngIcon, "AutoDeck.png")
_DependencyCheck(autodeckpngIconSuspended, "AutoDeckSuspended.png")
_DependencyCheck(autodeckpngIconPaused, "AutoDeckPaused.png")
_DependencyCheck(layerIcon, "Layer.png")
_DependencyCheck(mqttIcon, "MQTT.png")
_DependencyCheck(spellcheckIcon, "SpellCheck.png")
_DependencyCheck(autodeckicoIcon, "AutoDeck.ico")
_DependencyCheck(aboutIcon, "About.ico")
_DependencyCheck(bugIcon, "Bug.ico")
_DependencyCheck(configFolderIcon, "ConfigFolder.ico")
_DependencyCheck(editConfigIcon, "EditConfig.ico")
_DependencyCheck(exitIcon, "Exit.ico")
_DependencyCheck(reloadIcon, "Reload.ico")
_DependencyCheck(suspendIcon, "Suspend.ico")
_DependencyCheck(windowSpyIcon, "WindowSpy.ico")

; --- LIBRARY CHECKS ---
_DependencyCheck(AUTODECK_DLL_PATH, "AutoDeck.dll")
_DependencyCheck(CONFIG_FILE, "Config.ahk")
_DependencyCheck(MQTT_PATH, "MQTT.dll")

; --- End File Check ---
;=================================================================





; ======================= RUN ON STARTUP =========================
RegKey := "HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
ValueName := "AutoDeck"
ExePath := A_ScriptFullPath

try {
    existingValue := RegRead(RegKey, ValueName)
} catch {
    existingValue := ""
}

if (existingValue != ExePath) {
    RegWrite(ExePath, "REG_SZ", RegKey, ValueName)
}
; ================================================================




; ================ AUTO RELOAD WHEN CONFIG CHANGES ================
;                      IT DOESN'T WORK. FUCK IT

lastModified := FileGetTime(CONFIG_FILE, "M")

SetTimer(CheckConfigUpdate, 1000)

CheckConfigUpdate(*) {
    static lastModified := FileGetTime(CONFIG_FILE, "M")
    currentModified := FileGetTime(CONFIG_FILE, "M")
    if currentModified != lastModified {
        sendSerial(CONFIG_JSON)
        ;Sleep 1000
        ;Reload
    }
}
; =================================================================



; ===================== WINDOW UTILITY ============================
PinWindow(targetWindow := "A") {
    tWnd := WinActive(targetWindow)
    title := WinGetTitle("ahk_id " tWnd)
    newTitle := InStr(title, " - AlwaysOnTop") ? RegExReplace(title, " - AlwaysOnTop$") : title " - AlwaysOnTop"
    WinSetTitle(newTitle, "ahk_id " tWnd)
    WinSetAlwaysOnTop(-1, "ahk_id " tWnd)
}
; ==================================================================




; ===================== NEW TAB EXPLORER ==========================
; Note: This function is quite complex and specific. It may require adjustments if Explorer behavior changes.
ExplorerNewTab(path) {
    ExplorerHwnd := WinExist("ahk_class CabinetWClass")

    if !ExplorerHwnd {
        ExplorerNewWindow(path)
        return
    }

    Windows := ComObject("Shell.Application").Windows
    Count := Windows.Count

    PostMessage(0x0111, 0xA21B, 0, , "ahk_class CabinetWClass") ; Command for "New Tab"

    timeout := A_TickCount + 5000
    while (Windows.Count = Count) {
        Sleep(10)
        if (A_TickCount > timeout) {
            ExplorerNewWindow(path)
            return
        }
    }

    Item := Windows.Item(Count)
    try {
        Item.Navigate2(path)
    } catch {
        ExplorerNewWindow(path)
    }

    if WinExist("ahk_class CabinetWClass") and !WinActive("ahk_class CabinetWClass") {
        WinRestore("ahk_class CabinetWClass")
    }
}

ExplorerNewWindow(path) {
    Run("explorer.exe " path)
    WinWaitActive("ahk_class CabinetWClass")
    Send("{Space}")
}
; ==================================================================





; =================== TOAST NOTIFICATION CLASS =====================
class ToastNotification {
    static activeToasts := []
    static toastVisualWidth := 430
    static toastVisualHeight := 155
    static toastSpacing := 10

    __New(title, message, iconPath, color, duration) {
        this.duration := duration
        this.initialTransparency := 200
        this.width := ToastNotification.toastVisualWidth
        this.height := ToastNotification.toastVisualHeight

        this.gui := Gui("+ToolWindow -Caption +AlwaysOnTop +Disabled")
        this.gui.Add("Picture", "x10 y8 w70 h70", iconPath)
        this.gui.SetFont("s12 w700 q5", "Segoe UI")
        this.gui.Add("Text", "cFFFFFF x84 y8", title)
        this.gui.SetFont("s12 w400 q5", "Segoe UI")
        this.gui.Add("Text", "cFFFFFF x84 y30 w" (this.width - 84 - 10), message)
        this.gui.BackColor := color

        local currentOffsetY := 0
        for toastInstance in ToastNotification.activeToasts {
            if IsObject(toastInstance) && toastInstance.HasProp("gui") && IsObject(toastInstance.gui) && toastInstance.gui.HasProp("Hwnd") && WinExist("ahk_id " toastInstance.gui.Hwnd) {
                currentOffsetY += toastInstance.height + ToastNotification.toastSpacing
            }
        }

        local X := A_ScreenWidth - this.width - 36
        local Y := A_ScreenHeight - this.height - 120 - currentOffsetY

        this.gui.Show("NoActivate w" this.width " h" this.height " x" X " y" Y)
        ToastNotification.activeToasts.Push(this)

        WinSetTransparent(this.initialTransparency, this.gui.Hwnd)
        WinSetRegion("0-0 w" this.width " h" this.height " r30-30", this.gui.Hwnd)
        WinSetExStyle("+0x20", this.gui.Hwnd) ; Click-through

        SetTimer(this._StartFadeOut.Bind(this), -this.duration)
    }

    _StartFadeOut() {
        if !IsObject(this.gui) || !this.gui.HasProp("Hwnd") || !WinExist("ahk_id " this.gui.Hwnd) {
            this._Cleanup()
            return
        }
        try {
            loop 50 {
                local currentTrans := this.initialTransparency - (A_Index * 4)
                if (currentTrans < 0) currentTrans := 0
                if !IsObject(this.gui) || !this.gui.HasProp("Hwnd") || !WinExist("ahk_id " this.gui.Hwnd) { 
                    break
                }
                WinSetTransparent(currentTrans, this.gui.Hwnd)
                if (currentTrans = 0) {
                    break
                }
                Sleep(5) ; Penyesuaian sleep untuk fade yang lebih smooth
            }
        } catch Error as e {
            OutputDebug("Error during fade: " e.Message " for HWND: " (IsObject(this.gui) && this.gui.HasProp("Hwnd") ? this.gui.Hwnd : "N/A"))
        } finally {
            this._Cleanup()
        }
    }

    _Cleanup() {
        try {
            if IsObject(this.gui) && this.gui.HasProp("Hwnd") && WinExist("ahk_id " this.gui.Hwnd) {
                this.gui.Destroy()
            }
        } catch Error {
            OutputDebug("Error destroying GUI in _Cleanup")
        }
        for index, toastInstance in ToastNotification.activeToasts {
            if (toastInstance == this) {
                ToastNotification.activeToasts.RemoveAt(index)
                break
            }
        }
        ; ToastNotification.ReStackRemainingToasts() ; Implement if needed
    }
}

ShowToast(title, message, iconPath, color, duration) {
    ToastNotification(title, message, iconPath, color, duration)
}
;==========================================================================





; =========================== WINDOWS TOAST ===============================
toast_app_exe := A_ScriptFullPath
startMenuShortcut := A_StartMenuCommon "\AutoDeck.lnk"

if !FileExist(startMenuShortcut) {
    FileCreateShortcut(toast_app_exe, startMenuShortcut,,, "AutoDeck Notification", toast_app_exe)
}

ShowWinToast(title, message, imgpath) {
     js := (JsRT.Edge)()
    static toast_appid := A_ScriptFullPath

    js.ProjectWinRTNamespace("Windows.UI.Notifications")
    js.Exec('
    (
        function toast(template, image, text, app) {
            var N = Windows.UI.Notifications;
            var toastXml = N.ToastNotificationManager
                .getTemplateContent(N.ToastTemplateType[template]);
            var i = 0;
            for (let el of toastXml.getElementsByTagName("text")) {
                if (typeof text == "string") {
                    el.innerText = text;
                    break;
                }
                el.innerText = text(++i);
            }
            toastXml.getElementsByTagName("image")[0]
                .setAttribute("src", image);
            var toastNotifier = N.ToastNotificationManager
                .createToastNotifier(app || "AutoHotkey");
            var notification = new N.ToastNotification(toastXml);
            toastNotifier.show(notification);
        }
        function clearAllToasts(app) {
            Windows.UI.Notifications.ToastNotificationManager.history.clear(app);
        }
    )')

    js.clearAllToasts(toast_appid)
    js.toast("toastImageAndText02", imgpath, [title, message], toast_appid)
}
;==========================================================================




; =============================== MINI TOAST ===============================
global activeToasts := []   
global TOAST_GUI_HEIGHT := 45    
global TOAST_VERTICAL_SPACING := 35 
global TOAST_BASE_Y_OFFSET := 130  

showMiniToast(iconPath, title, color, duration) {
    global activeToasts, TOAST_GUI_HEIGHT
    toastGui := Gui("+AlwaysOnTop -Caption +ToolWindow +Disabled", "miniToast_" . A_TickCount)
    toastGui.Add("Picture", "x10 y8 w25 h25", iconPath)
    toastGui.SetFont("s12 w700 q5", "Consolas") 
    toastGui.Add("Text", "cFFFFFF x40 y10", title)
    toastGui.BackColor := color
    calculatedWidth := Round(100 + (StrLen(title) * 15)) 
    toastInfo := {gui: toastGui, width: calculatedWidth, height_gui: TOAST_GUI_HEIGHT}
    activeToasts.Push(toastInfo) 
    _repositionAllActiveToasts()
    destroyFunc := _destroySpecificToastAndReposition.Bind(toastGui)
    SetTimer(destroyFunc, -duration)
}

_destroySpecificToastAndReposition(guiObjToDestroy) {
    global activeToasts
    local toastRemoved := false
    try {
        if (IsObject(guiObjToDestroy) && guiObjToDestroy.Hwnd) { 
            guiObjToDestroy.Destroy()
        }
    } catch Error {
    }
    Loop activeToasts.Length {
        idx := activeToasts.Length - A_Index + 1
        if (activeToasts[idx].gui == guiObjToDestroy) {
            activeToasts.RemoveAt(idx)
            toastRemoved := true
            break 
        }
    }
    if (toastRemoved || !IsObject(guiObjToDestroy) || !guiObjToDestroy.Hwnd) { 
        _repositionAllActiveToasts() 
    }
}

_repositionAllActiveToasts() {
    global activeToasts, TOAST_GUI_HEIGHT, TOAST_VERTICAL_SPACING, TOAST_BASE_Y_OFFSET
    MonitorWidth := A_ScreenWidth
    MonitorHeight := A_ScreenHeight
    if (!activeToasts.Length) {
        return 
    }
    y_position_for_bottom_most_toast := MonitorHeight - TOAST_BASE_Y_OFFSET - TOAST_GUI_HEIGHT
    For index, toastEntry in activeToasts {
        toastGuiObj := toastEntry.gui
        currentToastWidth := toastEntry.width
        currentToastGuiHeight := toastEntry.height_gui 
        if (!IsObject(toastGuiObj) || !toastGuiObj.Hwnd) {
            Continue 
        }
        currentX := (MonitorWidth - currentToastWidth) // 2  
        display_Y := y_position_for_bottom_most_toast - ( (index - 1) * (currentToastGuiHeight + TOAST_VERTICAL_SPACING) )
        toastGuiObj.Show("NoActivate x" currentX " y" display_Y " w" currentToastWidth " h" currentToastGuiHeight)
        WinSetTransparent(255, "ahk_id " toastGuiObj.Hwnd)
        WinSetRegion("0-0 w" currentToastWidth " h" (currentToastGuiHeight + 30) " r30-30", "ahk_id " toastGuiObj.Hwnd)
        WinSetExStyle("+0x20", toastGuiObj)  ; Klik-through
    }
}
; ================================================================================================


; ========================== CALL CAPSLOCK TOAST ==========================
~CapsLock:: {
    global capsIconOn, capsIconOff
    ToggleState := GetKeyState("CapsLock", "T")
    if (ToggleState)
        showMiniToast(capsIconOn, "Caps ON", "0e0030", 2000)
    else
        showMiniToast(capsIconOff, "Caps OFF", "0e0030", 2000) ; Warna bisa dibedakan jika mau
}
; =========================================================================




; ======================== HTTP REQUEST FUNCTION ==========================
sendHttpRequest(url, successCallback := "", errorCallback := "") {
    global successIcon, failedIcon ; Pastikan ikon tersedia
    try {
        http := ComObject("WinHttp.WinHttpRequest.5.1")
        http.SetTimeouts(5000, 5000, 5000, 5000) ; Timeout 5 sec

        http.Open("GET", url, true) ; true for Async
        http.Send()
        http.WaitForResponse() ; Tunggu hingga selesai

        if (http.Status = 200) {
            response := http.responseText
            ;ShowToast("HTTP Success", "Response: " SubStr(response, 1, 50) "...", successIcon, "107c10", 3000)
            ;ShowWinToast("HTTP Success", "Response: " SubStr(response, 1, 50) "...", successIcon)
            if IsObject(successCallback)
                successCallback.Call(response)
        } else {
            response := "Status: " http.Status " " http.StatusText
            ;ShowToast("HTTP Failed", response, failedIcon, "e81123", 3000)
            ;ShowWinToast("HTTP Failed", response, failedIcon)
            if IsObject(errorCallback)
                errorCallback.Call(response)
        }
    } catch as e {
        response := "Error: " e.Message
        ;ShowToast("HTTP Error", response, failedIcon, "e81123", 3000)
        ShowWinToast("HTTP Error", response, failedIcon)
        if IsObject(errorCallback)
            errorCallback.Call(response)
    }
}
; ==========================================================================




; ============================ TRAY ICON MENU ==============================
A_TrayMenu.Delete() 
TraySetIcon(autodeckpngIcon)
A_iconTip := "AutoDeck v1.0"

A_TrayMenu.Add("Edit Config", (*) => Run("notepad.exe " CONFIG_FILE))
A_TrayMenu.Default := "Edit Config"
A_TrayMenu.SetIcon("Edit Config", editConfigIcon)
A_TrayMenu.Add("Reload Script", (*) => Reload())
A_TrayMenu.SetIcon("Reload Script", reloadIcon)
A_TrayMenu.Add("Open Config Folder", (*) => Run(ExplorerNewTab(A_ScriptDir)))
A_TrayMenu.SetIcon("Open Config Folder", configFolderIcon)

A_TrayMenu.Add() ; separator

A_TrayMenu.Add("Window Spy", (*) => Run(A_ScriptDir "\library\WindowSpy.ahk"))
A_TrayMenu.SetIcon("Window Spy", windowSpyIcon)
A_TrayMenu.Add("Suspend Hotkeys", (*) => Suspend())
A_TrayMenu.SetIcon("Suspend Hotkeys", suspendIcon)
A_TrayMenu.Add("Pause Script", (*) => Pause(-1))  
A_TrayMenu.SetIcon("Pause Script", pauseIcon)

A_TrayMenu.Add()

A_TrayMenu.Add("Report Bug", (*) => Run("https://github.com/karbyt/autodeck/issues"))
A_TrayMenu.SetIcon("Report Bug", bugIcon)
A_TrayMenu.Add("About", (*) => Run("https://github.com/karbyt/autodeck"))
A_TrayMenu.SetIcon("About", aboutIcon)
A_TrayMenu.Add("Exit", (*) => ExitApp())
A_TrayMenu.SetIcon("Exit", exitIcon)
; =========================================================================








; ; === Konfigurasi ===
; MQTT_PATH      := A_ScriptDir "\library\MQTT.dll"
; MQTT_BROKER    := "mqtt://192.168.1.100:1883"
; MQTT_CLIENT_ID := "AHK_Client_001"
; MQTT_USER      := "sandemo"
; MQTT_PASS      := "sandemo787898"

; Config_ComPort  := "COM6"  ; Ganti sesuai port yang kamu pakai
; Config_BaudRate := 115200
; Slider_MaxValue := 4095    ; ADC maksimum untuk mapping slider

; ; ===== MQTT SUBSCRIPTION LIST (OPTIONAL) =====
; MQTT_SUBSCRIPTIONS := [
;     { Topic: "ngisormejo/command", Callback: YourCallback1 },
;     { Topic: "sensor/data", Callback: YourCallback2 }
; ]

; ; ===== MQTT FALLBACK FUNCTION (IF USING SUBSCIPTION) =====
; YourCallback1(topic, payload) {
;     Run "notepad.exe"
;     Sleep 2000
;     SendInput "Pesan Diterima:`nTopik: " . topic . "`nPayload: " . payload
;     MsgBox "Pesan Diterima:`nTopik: " . topic . "`nPayload: " . payload
; }

; YourCallback2(topic, payload) {
;     OutputDebug("Callback dari sensor/data:`n" . payload)
; }















; ========================== M Q T T WRAPPER FUNCTIONS ============================
; --- Cek apakah MQTT digunakan ---
UsingMQTT := (IsSet(MQTT_BROKER) && MQTT_BROKER != "")


; --- Default Fallback values ---
if !IsSet(MQTT_BROKER)
    MQTT_BROKER := ""
if !IsSet(MQTT_CLIENT_ID)
    MQTT_CLIENT_ID := "AutoDeck_PC"
if !IsSet(MQTT_USER)
    MQTT_USER := ""
if !IsSet(MQTT_PASS)
    MQTT_PASS := ""


; --- Variabel global ---
global hDll := 0
global IsConnected := false
global MqttSubscriptionCallbacks := Map()




; --- Inisialisasi jika MQTT aktif ---
if UsingMQTT {
    hDll := DllCall("LoadLibrary", "Str", MQTT_DLL_NAME, "Ptr")
    if (hDll) {
        connectResult := DllCall( MQTT_DLL_NAME "\mqtt_connect", "AStr", MQTT_BROKER, "AStr", MQTT_CLIENT_ID, "AStr", MQTT_USER, "AStr", MQTT_PASS, "CDecl Int")
        IsConnected := (connectResult = 0)

        if (IsConnected) {
            showMiniToast(mqttIcon, "MQTT Connected", "0e0030", 2000)
            ; showMiniToast(autodeckpngIcon, "MQTT connected", "0e0030", 2000)
            SetTimer ProcessMqttMessages, 250 ; Cek pesan setiap 250 ms
        } else {

            MsgBox "Gagal terhubung ke MQTT broker:`n" . MQTT_BROKER . "`nKode error: " . connectResult "`nMake sure you entered the right username and password", "MQTT Connection Failed"
        }
    } else {
        MsgBox "Gagal memuat DLL: " . MQTT_PATH, "DLL Load Failed"
        ExitApp
    }
}




; --- Fungsi Publish MQTT ---
publishMQTT(topic, msg, qos := 0, retained := 0) {
    global IsConnected, hDll, MQTT_PATH
    if (!IsConnected || !hDll){
        showMiniToast(reloadIcon, "Publish Failed", "0e0030", 2000)
        ;ToolTip "publish failed"
        return false
    }
    result := DllCall("MQTT.dll\mqtt_publish", "AStr", topic, "AStr", msg, "Int", qos, "Int", retained, "CDecl Int")
    return (result = 0)
}


; --- Fungsi Subscribe MQTT ---
subscribeMQTT(topicFilter, CallbackFunction, qos := 0) {
    global IsConnected, hDll, MQTT_PATH, MqttSubscriptionCallbacks
    if (!IsConnected || !hDll) {
        ; ToolTip ("Gagal subscribe, blm terhubung")
        return false

    }
    if !IsObject(CallbackFunction) {
            MsgBox "Error: CallbackFunction untuk subscribeMQTT ke '" . topicFilter . "' tidak valid.", "Subscribe Error"
        return false
    }

    try {
        result := DllCall(MQTT_PATH . "\mqtt_subscribe", "AStr", topicFilter, "Int", qos, "CDecl Int")

        if (result = 0) {
            ; Simpan callback untuk topik ini (atau filter topik).
            ; Jika topik filter mengandung wildcard, pencocokan yang lebih canggih mungkin diperlukan di ProcessMqttMessages.
            ; Untuk saat ini, kita simpan berdasarkan filter persis.
            MqttSubscriptionCallbacks[topicFilter] := CallbackFunction
            OutputDebug("AHK: Berhasil subscribe ke '" . topicFilter . "'. Callback terdaftar.")
            return true
        } else {
            MsgBox "gagal subscribe"
            return false
        }
    } catch as e {
        OutputDebug("AHK: EXCEPTION saat subscribeMQTT ke '" . topicFilter . "': " . e.Message)
        return false
    }
}


; --- Timer untuk Memproses Pesan Masuk dari Antrian DLL ---
ProcessMqttMessages() {
    global hDll, MQTT_PATH, MQTT_DLL_NAME, MqttSubscriptionCallbacks
    if (!hDll || !IsConnected) ; Jangan proses jika DLL tidak dimuat atau tidak terkoneksi
        return

    local msgCount, getMsgResult
    local topicBuf, payloadBuf
    local receivedTopic, receivedPayload

    try {
        msgCount := DllCall(MQTT_PATH . "\mqtt_get_queued_message_count", "CDecl Int")
    } catch as e_count {
        SetTimer(ProcessMqttMessages, 0) ; Matikan timer jika ada error fundamental
        MsgBox "Error kritis saat mengambil jumlah pesan MQTT. Timer dihentikan.", "MQTT Error"
        return
    }

    Loop msgCount { ; Proses semua pesan yang ada di antrian saat ini
        ; Siapkan buffer untuk setiap pesan. Ukuran ini mungkin perlu disesuaikan.
        topicBuf := Buffer(512, 0)    ; Buffer 512 byte untuk topik
        payloadBuf := Buffer(4096, 0) ; Buffer 4KB untuk payload

        try {
            getMsgResult := DllCall(MQTT_PATH . "\mqtt_get_queued_message",
                                    "Ptr", topicBuf, "Int", topicBuf.Size,
                                    "Ptr", payloadBuf, "Int", payloadBuf.Size,
                                    "CDecl Int")
        } catch as e_get {
            break ; Hentikan loop jika ada error saat mengambil pesan
        }

        if (getMsgResult == 1) { ; Pesan berhasil diambil
            receivedTopic := StrGet(topicBuf, "UTF-8")
            receivedPayload := StrGet(payloadBuf, "UTF-8")

            ; Panggil callback yang sesuai berdasarkan topik
            ; Ini adalah pencocokan sederhana. Untuk wildcard MQTT (+, #), Anda perlu logika yang lebih canggih.
            foundCallback := false
            for subscribedFilter, callbackFunc in MqttSubscriptionCallbacks {
                ; Pencocokan sederhana: apakah topik yang diterima sama dengan filter yang disubscribe?
                ; ATAU, apakah filter adalah wildcard '#' yang cocok dengan semua?
                ; ATAU, apakah filter memiliki '+' dan cocok dengan struktur topik? (ini lebih kompleks)
                
                ; Pencocokan paling dasar:
                if (receivedTopic == subscribedFilter) {
                    if (callbackFunc is Func)
                        callbackFunc.Call(receivedTopic, receivedPayload)
                    foundCallback := true
                    break  ; Asumsi satu callback per pesan
                }
                ; TODO: Implementasi pencocokan wildcard yang lebih baik jika diperlukan.
                ; Contoh sangat dasar untuk wildcard '#' di akhir filter
                else if (SubStr(subscribedFilter, -1) == "/#" && InStr(receivedTopic, SubStr(subscribedFilter, 1, StrLen(subscribedFilter) - 2)) == 1) {
                    if (callbackFunc is Func)
                        callbackFunc.Call(receivedTopic, receivedPayload)
                    foundCallback := true
                    break
                }
            }
            if (!foundCallback) {
                OutputDebug("AHK: Tidak ada callback AHK yang terdaftar untuk topik: '" . receivedTopic . "'")
            }

        } else if (getMsgResult == 0) {
            OutputDebug("AHK: mqtt_get_queued_message mengembalikan 0 (antrian kosong), padahal msgCount > 0. Aneh.")
            break ; Keluar loop
        } else if (getMsgResult == -2) {
            OutputDebug("AHK: Error dari DLL: Buffer topik terlalu kecil saat mengambil pesan.")
            ; Anda bisa coba lagi dengan buffer lebih besar, atau log error
            break
        } else if (getMsgResult == -3) {
            OutputDebug("AHK: Error dari DLL: Buffer payload terlalu kecil saat mengambil pesan.")
            break
        } else {
            OutputDebug("AHK: Error tidak dikenal (" . getMsgResult . ") saat mengambil pesan dari DLL.")
            break
        }
    }
}


if (UsingMQTT && IsSet(MQTT_SUBSCRIPTIONS)) {
    for sub in MQTT_SUBSCRIPTIONS {
        subscribeMQTT(sub.Topic, sub.Callback)
    }
}

; =====================================================================




;========================== AUTODECK.DLL WRAPPER FUNCTIONS ==========================
; SetAppVolume(const wchar_t* targetExe, int volumePercent)
; GetAppMute(const wchar_t* targetExe) -> BOOL
; SetAppMute(BOOL bMute, const wchar_t* targetExe)
; GetMasterVolume(const wchar_t* deviceType) -> int
; SetMasterVolume(int volumePercent, const wchar_t* deviceType)
; GetMasterMute(const wchar_t* deviceType) -> BOOL
; SetMasterMute(BOOL bMute, const wchar_t* deviceType)
; GetAppVolume(const wchar_t* targetExe) -> int


SetAppVolume(exeName, volumePercent) { ; volumePercent 0-100
    global AUTODECK_DLL_PATH
    volumePercent := Max(0, Min(100, volumePercent)) ; Clamp
    DllCall(AUTODECK_DLL_PATH "\SetAppVolume", "WStr", exeName, "Int", volumePercent)
}

GetAppVolume(exeName) {
    global AUTODECK_DLL_PATH
    return DllCall(AUTODECK_DLL_PATH "\GetAppVolume", "WStr", exeName, "Int")
}

SetAppMute(exeName, bMute) { ; bMute: 1 (true) untuk mute, 0 (false) untuk unmute
    global AUTODECK_DLL_PATH
    DllCall(AUTODECK_DLL_PATH "\SetAppMute", "Int", bMute, "WStr", exeName)
    ShowToast("App Mute", exeName . (bMute ? " Muted" : " Unmuted"), successIcon, bMute ? "e81123" : "107c10", 1500)
    showMiniToast(bMute ? speakerOffIcon : speakerOnIcon, "Mic " (bMute ? "Muted" : "Unmuted"), "0e0030", 2000) ; Ganti ikon sesuai kebutuhan
}

GetAppMute(exeName) {
    global AUTODECK_DLL_PATH
    return DllCall(AUTODECK_DLL_PATH "\GetAppMute", "WStr", exeName, "Int") ; BOOL is Int in AHK DllCall
}

SetMasterVolume(volumePercent, deviceType := "playback") { ; volumePercent 0-100
    global AUTODECK_DLL_PATH
    volumePercent := Max(0, Min(100, volumePercent)) ; Clamp
    DllCall(AUTODECK_DLL_PATH "\SetMasterVolume", "Int", volumePercent, "WStr", deviceType)
    ; ShowToast("Master Volume", deviceType . ": " . volumePercent . "%", successIcon, "107c10", 1000)
}

GetMasterVolume(deviceType := "playback") {
    global AUTODECK_DLL_PATH
    return DllCall(AUTODECK_DLL_PATH "\GetMasterVolume", "WStr", deviceType, "Int")
}

SetMasterMute(bMute, deviceType := "playback") { ; bMute: 1 (true) untuk mute, 0 (false) untuk unmute
    global AUTODECK_DLL_PATH
    DllCall(AUTODECK_DLL_PATH "\SetMasterMute", "Int", bMute, "WStr", deviceType)
    isMuted := GetMasterMute(deviceType) ; Verifikasi
    ; ShowToast("Master Mute", deviceType . (isMuted ? " Muted" : " Unmuted"), successIcon, isMuted ? "e81123" : "107c10", 1500)
    showMiniToast(isMuted ? speakerOffIcon : speakerOnIcon, (isMuted ? "Muted" : "Unmuted"), "0e0030", 2000) 
}

GetMasterMute(deviceType := "playback") {
    global AUTODECK_DLL_PATH
    return DllCall(AUTODECK_DLL_PATH "\GetMasterMute", "WStr", deviceType, "Int")
}
; =================================================================================







global cs  ; Class Serial Object

GetGlobalVar(name) {
    return %name%
    }

MapValue(val, in_min, in_max, out_min, out_max) {
    return Round((val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
}



; === Inisialisasi Serial ===
SetupSerialCommunication() {
    global cs, Config_ComPort, Config_BaudRate

    cs := Serial(Config_ComPort, Config_BaudRate)

    if (!cs.Event_Parse_Start(MyFunc, "`n", 10)) {
        ShowWinToast("Serial Error", "Can't open serial on " Config_ComPort "`nClose other app that use this COM Port", cableIcon)
        ;MsgBox "Tidak dapat membuka serial di " Config_ComPort
        ExitApp
    }
    showMiniToast(cableIcon, "Serial Connected on " Config_ComPort, "0e0030", 2000)
    ;ToolTip "Serial Connected"
    ;MsgBox "Serial Connected on " Config_ComPort
}

; === Parsing Serial ===
MyFunc(MsgFromSerial) {
    CleanMsg := Trim(MsgFromSerial, Chr(13) . Chr(10))
    ; ToolTip "Raw Msg: " CleanMsg
    
    ; --- PARSING BUTTON --- format: B{id}:{P/R}
    if RegExMatch(CleanMsg, "^B(\d+):([PR])$", &m) {
        buttonID := m[1]
        state := m[2]
        fnName := "B" buttonID
        fn := GetGlobalVar(fnName)
    
        if IsObject(fn) && Type(fn) = "Func" {
            try fn.Call(state)
            catch {
                MsgBox("Gagal menjalankan fungsi: " fnName)
            }
        } else {
            ToolTip "Fungsi tidak ditemukan: " fnName
            SetTimer () => ToolTip(), -1500
        }
        return
    }
    
    ; --- PARSING POTS ANALOG --- format: P{id}:{value}
    if RegExMatch(CleanMsg, "^P(\d+):(\d+)$", &s) {
        potID := s[1]
        rawValue := s[2]
    
        maxADC := GetGlobalVar("Slider_MaxValue")
        mapped := MapValue(rawValue, 0, maxADC, 0, 100)
    
        fnName := "P" potID
        fn := GetGlobalVar(fnName)
    
        if IsObject(fn) && Type(fn) = "Func" {
            try fn.Call(mapped)
            catch {
                MsgBox("Gagal menjalankan fungsi slider: " fnName)
            }
        } else {
            ToolTip "Fungsi slider tidak ditemukan: " fnName
            SetTimer () => ToolTip(), -1500
        }
    }

    ; --- PARSING ROTARY ENCODER --- format: R{id}:{CW/CCW}
    if RegExMatch(CleanMsg, "^R(\d+):(CW|CCW)$", &m) {
        rotaryID := m[1]
        direction := m[2]
        fnName := "R" rotaryID
        fn := GetGlobalVar(fnName)
    
        if IsObject(fn) && Type(fn) = "Func" {
            try fn.Call(direction)
            catch {
                MsgBox("Gagal menjalankan fungsi: " fnName)
            }
        } else {
            ToolTip "Fungsi tidak ditemukan: " fnName
            SetTimer () => ToolTip(), -1500
        }
        return
    }

    ; --- ECHO SERIAL MESSAGE ---
    ; static isAckStarted := false
    ; static ackData := ""

    ; if RegExMatch(CleanMsg, "^ACK:\s*(.*)", &m) {
    ;     isAckStarted := true
    ;     ackData := m[1] "`n"  ; Mulai kumpulkan
    ; }
    ; else if isAckStarted {
    ;     if (CleanMsg ~= "^-+$") {  ; End of line check "-----"
    ;         MsgBox "ESP32 full ACK:\n" ackData
    ;         ackData := ""
    ;         isAckStarted := false
    ;     } else {
    ;         ackData .= CleanMsg "`n"
    ;     }
    ; }
}


; ========================= SEND SERIAL ===================
sendSerial(message) {
    global cs

    if (!IsObject(cs) || !cs._Is_Actually_Connected) {
        MsgBox "Serial port tidak terkoneksi!", "Error Pengiriman"
        return
    }

    cleaned := RegExReplace(message, "\s", "")

    cleaned .= "`n"

    try {
        bytesSent := cs.WriteString(cleaned)
        if (bytesSent > 0) {
            OutputDebug "Pesan dikirim: " bytesSent "`n"
            ;MsgBox "Pesan dikirim: " . bytesSent . " bytes.`n" . cleaned, "Info Pengiriman"
        } else {
            MsgBox "Gagal mengirim pesan. Tidak ada byte terkirim atau port disconnect.", "Error Pengiriman"
        }
    } catch Error as e {
        MsgBox "Error saat mengirim pesan: " . e.Message, "Exception Pengiriman"
    }
}

; ===============================================================================



; === Serial Communication Setup ===
SetupSerialCommunication()

OnExit(Cleanup)
return

; === Cleanup saat keluar ===
Cleanup(*) {
    global cs, hDll, MQTT_PATH

    totalStart := A_TickCount

    ; === Disconnect MQTT ===
    startMQTT := A_TickCount
    mqttDC := DllCall(MQTT_PATH . "\mqtt_disconnect", "CDecl Int")
    elapsedMQTT := A_TickCount - startMQTT

    ; === Putuskan Serial ===
    startSerial := A_TickCount
    try cs := unset
    catch as ex
        MsgBox "Gagal unset cs: " ex.Message
    elapsedSerial := A_TickCount - startSerial

    ; === Unload DLL ===
    if hDll
        DllCall("FreeLibrary", "Ptr", hDll)

    ; === Ringkasan waktu ===
    totalElapsed := A_TickCount - totalStart
    ;MsgBox "MQTT Disconnect: " elapsedMQTT " ms"
        ;. "`nSerial Unset: " elapsedSerial " ms"
        ;. "`nTotal Cleanup: " totalElapsed " ms"
}










F8::ShowWinToast("test","pesansederhana",cableIcon)
; S2(val){
;     SetMasterVolume(val)
; }