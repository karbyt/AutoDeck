; ===== CONFIG SERIAL PORT =====
global Config_ComPort := "COM6"  
global Config_BaudRate := 115200    

; ===== MAX ADC valUE =====
; For ESP32, the ADC value can range from 0 to 4095
; For Arduino, it can range from 0 to 1023
global Slider_Maxvalue := 4095


; ===== KEYS CONFIGURATION =====
K1(state) {
    if (state = "P") {
        Run "notepad.exe"
    }
}

K2(state) {
}

K3(state) {
    SendInput "Halo dari Macropad ESP32!{Enter}"
}

K4(state) {
    activeExe := WinGetProcessName("A")
    if (activeExe = "notepad.exe") {
        SendInput "Ini teks khusus untuk Notepad.{Enter}"
    } else if (activeExe = "Code.exe") {
        SendInput "{Ctrl Down}kc{Ctrl Up}" ; Contoh shortcut VS Code (Toggle Line Comment)
    } else {
        SendInput "Tombol K4 ditekan.{Enter}"
    }
}

K5(state) {
    SendInput "{Ctrl}{Shift}{Esc}" ; Open Task Manager
}





; ===== SLIDERS CONFIGURATION =====
S1(val) {
    SoundSetVolume(val)
    SetAppVolume("chrome.exe", val)
}

S2(val) {
    SetMasterVolume(val)
    ;ToolTip "Volume Master: " val "%"
}

S3(val) {
    SetAppVolume("chrome.exe", val)
    ToolTip "Volume Spotify: " val "%"
    SetTimer () => ToolTip(), -700
}

S4(val) {
    ToolTip "Slider S4: " val "%"
    SetTimer () => ToolTip(), -700
}



; ===== ROTARY ENCODER CONFIGURATION =====
R1(direction) {
    if (direction = "CW") {
        SendInput "{Volume_Down}"
    }
    else if (direction = "CCW") {
        SendInput "{Volume_Up}"
    }
}

F7::{
    SoundPlay "C:\Windows\Media\Windows Notify.wav"

}
; 