; --- KONFIGURASI SERIAL PORT ---
global Config_ComPort := "COM6"      ; Ganti dengan COM Port ESP32 Anda
global Config_BaudRate := 115200     ; Baudrate default

; --- Aksi untuk masing-masing tombol K1 sampai K12 ---
K1() {
    SoundSetVolume(+5)
}

K2() {
    Send "{Volume_Mute}"
}

K3() {
    str := "Typing this slowly..."
    for char in StrSplit(str)
    {
        Send char
        Sleep 100
    }
}

K4() {
    activeExe := WinGetProcessName("A")

    if (activeExe = "notepad.exe") {
        Send "this is special for notepad"
    } else if (activeExe = "Code.exe") {
        Send "this is special for vscode"
    } else {
        Send "this works globally"
    }
}

K5() {
    Send "{Ctrl}{Shift}{Esc}"
}

; Tambahkan K5, K6, dst sesuai kebutuhan this is special for vscodethis is special for vscode