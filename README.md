# 🧠 Autodeck
Autodeck is DIY macropad and volume mixer inspired by <a href= "https://github.com/omriharel/deej"> Deej</a> that only works for Windows. It lets you control the volume of individual apps, control your iot devices, do the automation and much more 

# 🌟Features
Autodeck is using AutoHotkey script to read serial
- Bind apps to different sliders
- Control microphone's input level
- Control your iot devices via HTTP or MQTT
- Very lightweight. Consuming around 1.5MB of RAM
- Runs from your system tray
- Helpful notifications to let you know if something isn't working


# 🔌Hardware 
This project has only been tested on esp32. But I think any microcontroller will also work as long as it has arduino framework

- you can use any potentiometer. either rotation or slider. **Keep in mind that you need to choose linear potentiometer instead of logarithmic**
- You can also use any button for the keypad. In this project I use mechanical switch

## Wiring

You can see community projects gallery <a href ="https://github.com/omriharel/deej/blob/master/community.md">here</a>
# ⚒️ How it works?
The macropad sends serial data over UART. The data looks like this
```
S1:2048
S2:0
S3:4096
K1:P
K1:R
K2:P
K2:R
```
Where:
S is slider, followed by Slider ID, followed by value
K is key, followed by Key ID, followed by state. P is pressed, R is released

**It will only print to serial when there's an update**

Then the data is read and parsed by AutoHotkey script. The script can read data and send data from the serial.

# 🪴How to Use?
## For beginners
1. Download **AutoDeck.exe**, **Config.ahk**, and **AutoDeck.bin** from release page
2. Flash the AutoDeck.bin to ESP32 with <a href = "https://tasmota.github.io/install/">this</a>. Upload the bin and flash it
3. Modify **Config.ahk** according to your hardware. you can scroll down to see the config example

## For advance user (Building from source)
### Compile the ESP32 Code
1. Clone this repo
```
git clone https://github.com/karbyt/autodeck.git
```
2. Install platformio extension in vscode
3. Open this repo in vscode
4. Modify the code as needed
5. Upload to ESP32 using ➡ icon on the top right corner, or type this in terminal
```
pio run --target upload
```
	platformio will auto detect ESP32 COM port or you can specify the COM port in **platformio.ini**

### Compile the AHK Script
1. Go to Autohotkey folder
2. Right click Main.ahk > compile script

# 📜Config Example

## Control Media
You can see <a href= "https://www.autohotkey.com/docs/v2/KeyList.htm">the list of keys here</a>

Control volume
```ahk
; Set volume to 20
K1() {
    SoundSetVolume "20"
}

; Increase volume by 2
K1() {
    Send "{Volume_Up}"
}

; Decrease volume by 2
K1() {
	Send "{Volume_Down}"
}

; Mute toggle
K1() {
    Send "{Volume_Mute}"
}
```

Control Media
```autohotkey
; Play/pause media
K1() {
	Send "{Media_Play_Pause}"
}

; Next media
K1() {
	Send "{Media_Next}"
}

; Prev media
K1() {
	Send "{Media_Prev}"
}

; Stop media
K1() {
	Send "{Media_Stop}"
}
```

## Send Character / Number
```
K1() {
	Send "A"
}
```

## Send String
Sentences
```
; Sentences
K1() {
	Send "Hello Motherfather"
}

;Code snippets
K1() {
	Send 'print("Hello, World{!}")'
}
```

Type current date
```
K1() {
    Send A_YYYY "-" A_MM "-" A_DD " " A_Hour ":" A_Min ":" A_Sec "{Enter}"
}
```

## Send Shortcut / Combo Keys
You can see the list of  <a href= "https://www.autohotkey.com/docs/v2/lib/Send.htm">special modifier keys here</a>

```
; This will open Task Manager (Ctrl + Shift + Esc)
K1() {
	Send "^+{Esc}"
}
```

## Auto Fills Form
This will autofill login form
```
K1() {
	Send "Username{Tab}"
    Send "Password123{Enter}"
}
```

## Open Folder
Open folder in **New Window**
```
K1() {
    Run "C:\Users\Username\Downloads"
}
```

Open folder in **New Tab** (Windows 11)
```
K1() {
    Explorer_NewTab("C:\Users\Username\Pictures")
}
```
## Open Url
This will open with default browser
```
K1() {
    Run "https://www.chatgpt.com"
}
```

## Open App
```
; Open notepad
K1() {
	Run "notepad.exe"
}

; Open calculator
K1() {
	Run "calc.exe"
}

; Open custom program
K1() {
	Run "C:\Program Files\Google\Chrome\Application\chrome.exe"
}
```

## Show Toast
```
K1() {
	Toast("Hello Autodeck!", "This is a toast message.", successIcon, "107c10", 3000)
}
```

## Send HTTP Request
```
K1() {
	SendHttpRequest("http://192.168.1.100/cmd?cmnd=Power1%202")
}
```

## Only Active in Certain Window
```
; only works in notepad, otherwise it won't do anything
K1() {
    if WinActive("ahk_exe notepad.exe") {
        Send ^c
    }
}

; this will send different scenario in different app
K1() {
    activeExe := WinGetProcessName("A")

    if (activeExe = "notepad.exe") {
        Send "this is special for notepad"
    } else if (activeExe = "Code.exe") {
        Send "this is special for vscode"
    } else {
        Send "this works globally"
    }
}
```

## Run Python Script
```
K1() {
	Run "python C:\Users\Username\Documents\hello.py"
}

; Run a Python script relative to this script location
K1() {
    scriptPath := A_ScriptDir "\scripts\example.py"
    Run "python " scriptPath
}
```


---
## ToDo
- Tap Dance Feature
- Layers
- Rotary Encoder
- OLED