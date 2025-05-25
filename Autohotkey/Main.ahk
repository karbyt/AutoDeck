#SingleInstance Force
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

F1::K1()
SetupSerial()
Return