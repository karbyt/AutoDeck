; ===== CONFIG SERIAL PORT =====
global Config_ComPort := "COM3"  
global Config_BaudRate := 115200    

; ===== MAX ADC valUE =====
; For ESP32, the ADC value can range from 0 to 4095
; For Arduino, it can range from 0 to 1023
global Slider_Maxvalue := 4095


; ===== CONFIG JSON =====
global CONFIG_JSON := '
(
{
  "general": {
    "maxLayers": 3,
    "ledBuiltinPin": 2
  },
  "buttons": {
    "enabled": true,
    "numRows": 3,
    "numCols": 4,
    "rowPins": [13, 12, 14],
    "colPins": [33, 32, 15, 4],
    "debounceDelay": 50
  },
  "potentiometers": {
    "enabled": true,
    "numPots": 2,
    "potPins": [34, 35],
    "alpha": 0.005,
    "reportThreshold": 20
  },
  "rotaryEncoders": {
    "enabled": true,
    "numEncoders": 1,
    "encoderPinsA": [5],
    "encoderPinsB": [17]
  }
}
)'


; ; ===== MQTT CONFIGURATION (OPTIONAL) =====
; MQTT_BROKER := "mqtt://192.168.1.100:1883"
; MQTT_USER := ""
; MQTT_PASS := ""

; ; ===== MQTT SUBSCRIPTION LIST (OPTIONAL) =====
; MQTT_SUBSCRIPTIONS := [
;     { Topic: "ngisormejo/command", Callback: YourCallback1 },
;     { Topic: "sensor/data", Callback: YourCallback2 },
;     { Topic: "ringlight", Callback: RinglightCallback }
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

; RinglightCallback(topic, payload) {
;     global ringlightbrightness := payload
; }

; ===== KEYS CONFIGURATION =====
B1(state) {
    if (state = "P") {
        SetMasterMute(true)
    } 
    else if (state = "R") {
        SetMasterMute(false)
    }
}


; ===== POTS CONFIGURATION =====
P2(val) {
    SetMasterVolume(val)
}

P4(val) {
    SetAppVolume("chrome.exe",val)
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

^r::Reload