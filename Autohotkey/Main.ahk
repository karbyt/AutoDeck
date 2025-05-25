#SingleInstance Force


;========= PIN WINDOW FUNCTION ===============
pinWindow(targetWindow := "A") {
    tWnd := WinActive(targetWindow)
    title := WinGetTitle("ahk_id " tWnd)
    newTitle := InStr(title, " - AlwaysOnTop") ? RegExReplace(title, " - AlwaysOnTop$") : title " - AlwaysOnTop"
    WinSetTitle(newTitle, "ahk_id " tWnd)
    WinSetAlwaysOnTop(-1, "ahk_id " tWnd)
}
;==========================================


;============================= NEW TAB FUNCTION =======================================
Explorer_NewTab(path) {
    ExplorerHwnd := WinExist("ahk_class CabinetWClass")

    if !ExplorerHwnd {
        Explorer_NewWindow(path)
        return
    }

    Windows := ComObject("Shell.Application").Windows
    Count := Windows.Count  ; Hitung jumlah jendela Explorer yang terbuka

    ; Kirim perintah untuk membuka tab baru ke jendela Explorer, bahkan jika dalam keadaan minimized
    PostMessage(0x0111, 0xA21B, 0, , "ahk_class CabinetWClass")

    timeout := A_TickCount + 5000
    ; Tunggu tab baru terbuka
    while (Windows.Count = Count) {
        Sleep(10)
        if (A_TickCount > timeout) {
            Explorer_NewWindow(path)
            return
        }
    }

    Item := Windows.Item(Count)
    try {
        Item.Navigate2(path)  ; Buka folder di tab baru
    } catch {
        Explorer_NewWindow(path)
    }

    ; Jika Explorer dalam keadaan minimized, restore jendela
    if WinExist("ahk_class CabinetWClass") and !WinActive("ahk_class CabinetWClass") {
        WinRestore("ahk_class CabinetWClass")
    }
}

Explorer_NewWindow(path) {
    Run("explorer.exe " path)
    WinWaitActive("ahk_class CabinetWClass")
    Send("{Space}")  ; Pilih item pertama
}
;=======================================================================================

;=========================== T O A S T =====================================
successIcon := A_ScriptDir "\media\websuccess.png"
failedIcon := A_ScriptDir "\media\webfailed.png"
capsIconOn := A_ScriptDir "\media\lock.png"
capsIconOff := A_ScriptDir "\media\unlock.png"
folderIcon := A_ScriptDir "\media\folder.png"

Toast(tittle, message, iconPath, color, duration) {
    toast := Gui("+ToolWindow -Caption +AlwaysOnTop +Disabled", "toast")

    toast.Add("Picture", "x10 y8 w70 h70", iconPath)

    toast.SetFont("s12 w700 q5", "Segoe UI")
    toast.Add("Text", "cFFFFFF x84 y8", tittle)

    toast.SetFont("s12 w400 q5", "Segoe UI")
    toast.Add("Text", "cFFFFFF x84 y30 w150", message)

    toast.BackColor := color

    Width := 400
    Height := 125
    MonitorWidth := A_ScreenWidth
    MonitorHeight := A_ScreenHeight
    X := MonitorWidth - Width - 36
    Y := MonitorHeight - Height - 120

    toast.Show("NoActivate w" Width " h" Height " x" X " y" Y)

    transparancy := 200
    WinSetTransparent transparancy, "toast" ; Transparent
    WinSetRegion "0-0 w430 h155 r30-30", "toast" ; Rounded corner
    WinSetExStyle("+0x20", toast)  ; Klik-through

    Sleep(duration)
    ; Fade out loop
    loop 50 {
        if A_Index = 50 {
            toast.Destroy()
            break
        }
        TransFade := transparancy - (A_Index * 4)
        WinSetTransparent(TransFade, toast)
        Sleep(1)
    }
}
;==========================================================================


;=========================== CAPS LOCK TOAST ==========================
global activeToastGui := 0  ; Menyimpan objek GUI, bukan hanya hwnd

CapsToast(iconPath, tittle, color, duration) {
    ; Gunakan keyword global secara eksplisit di awal fungsi
    global activeToastGui

    ; Hancurkan toast yang aktif jika ada
    if (activeToastGui != 0) {
        try {
            activeToastGui.Destroy()  ; Menghancurkan objek GUI langsung
            activeToastGui := 0
        } catch {
            ; Jika gagal, lanjutkan
        }
    }

    ; Buat GUI baru
    capsToast := Gui("+AlwaysOnTop -Caption +ToolWindow +Disabled", "capsToast")
    capsToast.Add("Picture", "x10 y8 w25 h25", iconPath)
    capsToast.SetFont("s12 w700 q5", "Segoe UI")
    capsToast.Add("Text", "cFFFFFF x40 y10", tittle)

    capsToast.BackColor := color

    Width := 120
    Height := 45
    MonitorWidth := A_ScreenWidth
    MonitorHeight := A_ScreenHeight

    X := (MonitorWidth - Width) // 2  ; Pusat horizontal
    Y := MonitorHeight - Height - 130  ; Bawah dengan jarak 50px dari tepi

    ; Simpan objek GUI saat ini
    activeToastGui := capsToast

    ; Tampilkan GUI
    capsToast.Show("NoActivate w" Width " h" Height " x" X " y" Y)

    ; Atur transparansi dan region
    WinSetTransparent 250, "ahk_id " capsToast.Hwnd
    WinSetRegion "0-0 w200 h75 r30-30", "ahk_id " capsToast.Hwnd
    WinSetExStyle("+0x20", capsToast)  ; Klik-through

    ; Buat fungsi callback dengan binding ke objek GUI saat ini
    DestroyThisToast := DestroyToast.Bind(capsToast)

    ; Panggil fungsi ini setelah durasi
    SetTimer DestroyThisToast, -duration
}

DestroyToast(guiObj) {
    ; Fungsi untuk menghancurkan objek GUI tertentu
    try {
        guiObj.Destroy()
        ; Jika ini adalah GUI aktif, reset variabel global
        global activeToastGui
        if (activeToastGui == guiObj)
            activeToastGui := 0
    } catch {
        ; Jika gagal, lanjutkan
    }
}
;==========================================================================



;========================= TOGGLE CAPS LOCK TOAST ==========================
~CapsLock::
{
    ; Deklarasi global di dalam hotkey
    global capsIconOn, capsIconOff

    ToggleState := GetKeyState("CapsLock", "T")

    if (ToggleState)
        CapsToast(capsIconOn, "Caps ON", "0078d4", 2000)  ; Biru untuk ON
    else
        CapsToast(capsIconOff, "Caps OFF", "0078d4", 2000) ; Merah untuk OFF
}
;=========================================================================


;========================= HTTP REQUEST FUNCTION ==========================
SendHttpRequest(url) {
    try {
        http := ComObject("WinHttp.WinHttpRequest.5.1")
        http.SetTimeouts(3000, 3000, 3000, 3000)  ; Timeout 3 detik untuk semua tahap

        http.Open("GET", url, false)
        http.Send()

        if (http.Status = 200) {
            response := http.responseText
            SetTimer(() => Toast("Success!", "Response: " response, successIcon, "107c10", 3000), -10)
        } else {
            response := http.Status
            SetTimer(() => Toast("Failed!", response, failedIcon, "e81123", 3000), -10)
        }
    } catch as e {
        response := e.Message
        SetTimer(() => Toast("Failed!", response, failedIcon, "e81123", 3000), -10)
    }
}
;==========================================================================





#Include Config.ahk
#Include ClassSerial.ahk

SendMode "Input"
A_BatchLines := -1

global cs

; Fungsi pembantu
GetGlobalVar(name) {
    return %name%
}

SetupSerial() {
    global cs
    cs := Serial(Config_ComPort, Config_BaudRate)

    if (!cs.Event_Parse_Start(MyFunc, "`n", 10)) {
        MsgBox "Gagal memulai komunikasi serial. Skrip akan keluar.", "Error", "IconError"
        ExitApp
    }
    ToolTip "Serial dimulai pada " Config_ComPort ", Baud " Config_BaudRate
    SetTimer () => ToolTip(), -2000
}

; ================= READ SERIAL =================
MyFunc(MsgFromSerial) {
    CleanMsg := Trim(MsgFromSerial, "`r`n")
    ; ToolTip "Raw Msg: " CleanMsg ; Untuk melihat pesan mentah

    if RegExMatch(CleanMsg, "^K(\d+):([PR])$", &m) {
        keyID := m[1]
        state := m[2]

        if (state = "P") {
            fnName := "K" keyID
            fn := GetGlobalVar(fnName)

            if IsObject(fn) && Type(fn) = "Func" {
                try fn.Call()
                catch {
                    MsgBox("Gagal menjalankan fungsi: " fnName)
                }
            } else {
                ToolTip "Fungsi tidak ditemukan: " fnName
                SetTimer () => ToolTip(), -1500
            }
        }
    }
}

;================== CALL MAIN FUNCTION ==================
SetupSerial()
;========================================================