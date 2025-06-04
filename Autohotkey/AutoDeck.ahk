#SingleInstance Force
SetWorkingDir A_ScriptDir "\library" ; IMPORTANT: Otherwise windows can't find the DLLs and it's dependencies
SendMode "Input"
A_BatchLines := -1

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



; ================ AUTO RELOAD WHEN CONFIG CHANGES ================
lastModified := FileGetTime(CONFIG_FILE, "M")

SetTimer(CheckConfigUpdate, 1000)

CheckConfigUpdate(*) {
    static lastModified := FileGetTime(CONFIG_FILE, "M")
    currentModified := FileGetTime(CONFIG_FILE, "M")
    if currentModified != lastModified {
        Reload
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
    static js := (JsRT.Edge)()
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



;=============================== MINI TOAST ===============================
global activeToastGui := 0  ; Menyimpan objek GUI, bukan hanya hwnd

CapsToast(iconPath, tittle, color, duration) {
    global activeToastGui
    if (activeToastGui != 0) {
        try {
            activeToastGui.Destroy()  
            activeToastGui := 0
        } catch {
            ; if error, do nothing
        }
    }
    ; Create a new GUI for the toast
    capsToast := Gui("+AlwaysOnTop -Caption +ToolWindow +Disabled", "capsToast")
    capsToast.Add("Picture", "x10 y8 w25 h25", iconPath)
    capsToast.SetFont("s12 w700 q5", "Segoe UI")
    capsToast.Add("Text", "cFFFFFF x40 y10", tittle)
    capsToast.BackColor := color

    Width := 120
    Height := 45
    MonitorWidth := A_ScreenWidth
    MonitorHeight := A_ScreenHeight
    X := (MonitorWidth - Width) // 2  
    Y := MonitorHeight - Height - 130  
    activeToastGui := capsToast
    
    capsToast.Show("NoActivate w" Width " h" Height " x" X " y" Y)

    WinSetTransparent 250, "ahk_id " capsToast.Hwnd
    WinSetRegion "0-0 w200 h75 r30-30", "ahk_id " capsToast.Hwnd
    WinSetExStyle("+0x20", capsToast)  ; Klik-through
    DestroyThisToast := _DestroySpecificToast.Bind(capsToast)
    SetTimer DestroyThisToast, -duration
}

_DestroySpecificToast(guiObj) {
    global activeCapsLockToastGui
    try {
        guiObj.Destroy()
        if (activeCapsLockToastGui == guiObj) {
            activeCapsLockToastGui := 0
        }
    } catch {

    }
}
; ==========================================================================



; ========================== CALL CAPSLOCK TOAST ==========================
~CapsLock:: {
    global capsIconOn, capsIconOff
    ToggleState := GetKeyState("CapsLock", "T")
    if (ToggleState)
        CapsToast(capsIconOn, "Caps ON", "0e0030", 2000)
    else
        CapsToast(capsIconOff, "Caps OFF", "0e0030", 2000) ; Warna bisa dibedakan jika mau
}
; =========================================================================



; ======================== HTTP REQUEST FUNCTION ==========================
SendHttpRequest(url, successCallback := "", errorCallback := "") {
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



; ========================== M Q T T WRAPPER FUNCTIONS ============================
; --- Cek apakah MQTT digunakan ---
UsingMQTT := (IsSet(MQTT_BROKER) && MQTT_BROKER != "")

; --- Variabel global ---
hDll := 0
IsConnected := false

; --- Inisialisasi jika MQTT aktif ---
if UsingMQTT {
    hDll := DllCall("LoadLibrary", "Str", MQTT_PATH, "Ptr")
    if (hDll) {
        connectResult := DllCall(MQTT_DLL_NAME "\mqtt_connect", "AStr", MQTT_BROKER, "AStr", MQTT_CLIENT_ID, "AStr", MQTT_USER, "AStr", MQTT_PASS, "CDecl Int")
        IsConnected := (connectResult = 0)

        if (!IsConnected) {
            MsgBox "Gagal terhubung ke MQTT broker:`n" . MQTT_BROKER . "`nKode error: " . connectResult, "MQTT Connection Failed"
        }
    } else {
        MsgBox "Gagal memuat DLL: " . MQTT_PATH, "DLL Load Failed"
        ExitApp
    }
}

; --- Fungsi Publish MQTT ---
publishMQTT(topic, msg, qos := 0, retained := 0) {
    global IsConnected
    if (!IsConnected)
        return false
    result := DllCall("MQTT.dll\mqtt_publish", "AStr", topic, "AStr", msg, "Int", qos, "Int", retained, "CDecl Int")
    return (result = 0)
}

; --- Bersihkan saat keluar ---
OnExit(*) {
    global IsConnected, hDll
    if (IsConnected)
        DllCall("MQTT.dll\mqtt_disconnect", "CDecl Int")
    if (hDll)
        DllCall("FreeLibrary", "Ptr", hDll)
}


; =================================================================================



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
    CapsToast(bMute ? speakerOffIcon : speakerOnIcon, "Mic " (bMute ? "Muted" : "Unmuted"), "0e0030", 2000) ; Ganti ikon sesuai kebutuhan
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
    CapsToast(isMuted ? speakerOffIcon : speakerOnIcon, (isMuted ? "Muted" : "Unmuted"), "0e0030", 2000) 
}

GetMasterMute(deviceType := "playback") {
    global AUTODECK_DLL_PATH
    return DllCall(AUTODECK_DLL_PATH "\GetMasterMute", "WStr", deviceType, "Int")
}
; =================================================================================








; ======================== READ AND PARSE SERIAL MESSAGE ==========================

#Include library\ClassSerial.ahk 

global cs ; Class Serial Object

GetGlobalVar(name) {
    return %name%
    }

MapValue(val, in_min, in_max, out_min, out_max) {
    return Round((val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
}

SetupSerialCommunication() {
    global cs
    cs := Serial(Config_ComPort, Config_BaudRate)
    
    if (!cs.Event_Parse_Start(MyFunc, "`n", 10)) {
        MsgBox "Can't connect`nI have no idea why", "Error", "iconX"
        ExitApp
    }
    ShowWinToast("Serial Connected", "Device on " Config_ComPort " `nis connected.", cableIcon)
}

MyFunc(MsgFromSerial) {
    CleanMsg := Trim(MsgFromSerial, "rn")
    ; ToolTip "Raw Msg: " CleanMsg
    
    ; --- PARSING BUTTON --- format: K{id}:{P/R}
    if RegExMatch(CleanMsg, "^K(\d+):([PR])$", &m) {
        keyID := m[1]
        state := m[2]
        fnName := "K" keyID
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
    
    ; --- PARSING SLIDER ANALOG --- format: S{id}:{value}
    if RegExMatch(CleanMsg, "^S(\d+):(\d+)$", &s) {
        sliderID := s[1]
        rawValue := s[2]
    
        maxADC := GetGlobalVar("Slider_MaxValue")
        mapped := MapValue(rawValue, 0, maxADC, 0, 100)
    
        fnName := "S" sliderID
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
    if RegExMatch(CleanMsg, "^ACK:\s*(.*)", &m) {
        received := m[1]  ; Ini bagian setelah "ACK: "
        MsgBox "ESP32 echoing: " received
    }


}
;====================================================================


SendConfigToESP32() {
    global cs

    if (!IsObject(cs) || !cs._Is_Actually_Connected) {
        MsgBox "Serial port tidak terkoneksi!", "Error Pengiriman Konfigurasi"
        return
    }

    ; Contoh input JSON rapi (multiline)
    prettyJson := '
(
{
    "version": 1,
    "pins": {
        "button1": 12,
        "button2": 14,
        "led_status": 2,
        "slider1_adc": 34
    },
    "settings": {
        "baud_rate": 115200,
        "device_name": "MyMacropadV2"
    }
}
)'
    MsgBox (prettyJson)


    ; Convert JSON rapi ke Map
    jsonStr := Trim(prettyJson) ; Hilangkan newline/spasi di awal/akhir
    MsgBox (jsonStr)
    map := Jxon_Load(&jsonStr)

    ; Dump jadi satu baris tanpa indentasi atau newline
    jsonToSend := Jxon_Dump(map, false)
    MsgBox "JSON yang akan dikirim: " jsonToSend, "Info Konfigurasi"
    ; Kirim string JSON ke ESP32, tambahkan newline sebagai terminator
    ; Jika WriteString Anda mengharapkan parameter encoding, sesuaikan.
    ; Di AHK v2, string secara default adalah UTF-8, jadi jika WriteString
    ; hanya mengirim byte dari string, itu seharusnya sudah benar.
    try {
        bytesSent := cs.WriteString(jsonToSend . "`n") ; Tambahkan newline
        if (bytesSent > 0) {
            MsgBox "Konfigurasi dikirim: " . bytesSent . " bytes.`n" . jsonToSend, "Info Konfigurasi"
        } else {
            MsgBox "Gagal mengirim konfigurasi. Tidak ada byte terkirim atau port disconnect.", "Error Pengiriman Konfigurasi"
        }
    } catch Error as e {
        MsgBox "Error saat mengirim konfigurasi: " . e.Message, "Exception Pengiriman"
    }
}

































; ===================== BEGIN THE COMMUNICATION =====================
SetupSerialCommunication()
;====================================================================





; ==================== DEBUGGING AND TESTING AREA ====================

; F1::ShowToast("Test Toast", "Ini adalah pesan tes.", successIcon, "107c10", 3000)
; F2::Nircmd_MonitorOff()
F9::{
    SetMasterVolume(20)

}

F8::{
    publishMQTT("ngisormejo", "T")
}

F10::{
    SendConfigToESP32()
}
;=========================================================

