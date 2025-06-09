
; Library untuk komunikasi serial
class Serial
{
    _COM_Port := ""
    _Baud_Rate := 9600
    _Parity := "N"
    _Data_Bits := 8
    _Stop_Bits := 1
    _COM_FileHandle := 0
    _Received_Partial := ""
    _Last_Received_HEX := ""
    _Bytes_Received_Count := 0
    _Is_Available := true

    _Is_Actually_Connected := false     ; Flag status koneksi aktual
    _Was_Disconnected_Notified := false ; Mencegah notifikasi disconnect berulang saat mencoba reconnect
    _Reconnect_Attempt_Interval := 3000 ; Interval (ms) untuk mencoba konek ulang

    _Event_Delims := "`r`n"
    _Event_Omit_Chars := ""
    _Event_Registered_Function := ""
    _Event_Separator := "`n"
    _Poll_Interval_Saved := 100
    _BoundCheckReadRegister := ""
    _IsTimerRunning := false
    AllReceived := ""
    Chunk := ""

    __New(Port := "COM6", Baud := 9600, Parity := "N", Data := 8, Stop := 1)
    {
        this._COM_Port := Port
        this._Baud_Rate := Baud
        this._Parity := Parity
        this._Data_Bits := Data
        this._Stop_Bits := Stop
        this._Received_Partial := ""
        this._COM_FileHandle := 0
        this._Is_Available := true
    }

    __Delete()
    {
        if (this._COM_FileHandle != 0)
            this.__Close_COM()
        if (this._IsTimerRunning)
            this.__StopTimer()
    }

    __Open_Port()
    {
        if (this._COM_FileHandle != 0)
            return -1

        Settings_String := Trim(this._COM_Port) ":baud=" this._Baud_Rate " parity=" this._Parity " data=" this._Data_Bits " stop=" this._Stop_Bits " dtr=Off"
        
        DCB := Buffer(28)

        Loop {
            BCD_Result := DllCall("BuildCommDCB", "Str", Settings_String, "Ptr", DCB.Ptr)
            if (BCD_Result != 0)
                break
            
            ; PERBAIKAN MsgBox: Gunakan nilai numerik atau opsi v2.0.11+
            ; 3 = YesNoCancel buttons, 32 = IconQuestion. Ini sudah benar.
            UserChoice := MsgBox("It appears that the device may not be connected to " this._COM_Port ".`nWould you like to change COM ports?", "COM Failure", 3 | 32)
            if (UserChoice = "Yes") {
                ; PERBAIKAN InputBox: Parameter ke-3 adalah Options (String), Parameter ke-4 adalah Default (String)
                InputObj := InputBox("Select New COM port", "Which COM port should be selected (e.g., COM1, COM2)?", "", this._COM_Port)
                if InputObj.Result = "OK" ; InputObj adalah objek, jadi .Result dan .Value bisa diakses
                    this._COM_Port := InputObj.Value
                else if InputObj.Result = "Cancel" ; Atau "Timeout"
                    return 0
                Settings_String := Trim(this._COM_Port) ":baud=" this._Baud_Rate " parity=" this._Parity " data=" this._Data_Bits " stop=" this._Stop_Bits " dtr=Off"
            } else { ; UserChoice adalah "No" atau "Cancel" atau window ditutup
                return 0
            }
        }

        COM_Port_Name := StrLen(this._COM_Port) > 4 ? "\\.\" Trim(this._COM_Port) : Trim(this._COM_Port)

        Loop 20 {
            this._COM_FileHandle := DllCall("CreateFile"
                , "Str", COM_Port_Name
                , "UInt", 0xC0000000 ; GENERIC_READ | GENERIC_WRITE
                , "UInt", 3          ; OPEN_EXISTING
                , "Ptr", 0           ; NULL
                , "UInt", 3          ; OPEN_EXISTING (ulangi karena parameter ke-4 adalah lpSecurityAttributes, ke-5 adalah dwCreationDisposition)
                , "UInt", 0          ; FILE_ATTRIBUTE_NORMAL
                , "Ptr", 0           ; NULL
                , "UPtr")
            if (this._COM_FileHandle != 0 && this._COM_FileHandle != -1) ; -1 adalah INVALID_HANDLE_VALUE
                break
            this._COM_FileHandle := 0
            Sleep 100
            if (A_Index = 20) {
                ;MsgBox "There is a problem with Serial Port communication.`nFailed Dll CreateFile, COM_FileHandle=" this._COM_FileHandle "`nClose other program that uses this COM port`nThe Script Will Now Exit.", "Error", 16 ; 16 = IconError
                ShowWinToast("Serial Error", "There is a problem with Serial Port communication.`nClose other program that uses " this._COM_Port "`nThe Script Will Now Exit.", cableIcon) ; Gunakan ShowWinToast untuk notifikasi

                ExitApp()
            }
        }
        
        SCS_Result := DllCall("SetCommState", "UPtr", this._COM_FileHandle, "Ptr", DCB.Ptr)
        if (SCS_Result = 0) {
            ;MsgBox "There is a problem with Serial Port communication.`nFailed Dll SetCommState.`nClose other program that uses this COM port`nThe Script Will Now Exit.", "Error", 16 ; 16 = IconError
            ShowWinToast("Serial Error", "There is a problem with Serial Port communication.`nClose other program that uses " this._COM_Port "`nThe Script Will Now Exit.", cableIcon) ; Gunakan ShowWinToast untuk notifikasi
            this.__Close_COM()
            return 0
        }

        TimeoutsBuf := Buffer(20)
        NumPut("UInt", 0xFFFFFFFF, TimeoutsBuf, 0) ; ReadIntervalTimeout
        NumPut("UInt", 0, TimeoutsBuf, 4)          ; ReadTotalTimeoutMultiplier
        NumPut("UInt", 1, TimeoutsBuf, 8)          ; ReadTotalTimeoutConstant (was 1, can be 0 for non-blocking if multiplier is 0)
        NumPut("UInt", 0, TimeoutsBuf, 12)         ; WriteTotalTimeoutMultiplier
        NumPut("UInt", 100, TimeoutsBuf, 16)       ; WriteTotalTimeoutConstant

        SCT_Result := DllCall("SetCommTimeouts", "UPtr", this._COM_FileHandle, "Ptr", TimeoutsBuf.Ptr)
        if (SCT_Result = 0) {
            MsgBox "There is a problem with Serial Port communication.`nFailed Dll SetCommTimeouts.`nThe Script Will Now Exit.", "Error", 16 ; 16 = IconError
            this.__Close_COM()
            ExitApp()
        }
        return 1
    }

    __Open_Port_Silent()
    {
        if (this._COM_FileHandle != 0) { ; Jika handle masih ada, tutup dulu
            this.__Close_COM()
        }

        Settings_String := Trim(this._COM_Port) ":baud=" this._Baud_Rate " parity=" this._Parity " data=" this._Data_Bits " stop=" this._Stop_Bits " dtr=Off"
        DCB := Buffer(28)

        BCD_Result := DllCall("BuildCommDCB", "Str", Settings_String, "Ptr", DCB.Ptr)
        if (BCD_Result = 0) {
            return 0 ; Gagal membuat DCB
        }

        COM_Port_Name := StrLen(this._COM_Port) > 4 ? "\\.\" Trim(this._COM_Port) : Trim(this._COM_Port)

        this._COM_FileHandle := DllCall("CreateFile"
            , "Str", COM_Port_Name
            , "UInt", 0xC0000000
            , "UInt", 3
            , "Ptr", 0
            , "UInt", 3
            , "UInt", 0
            , "Ptr", 0
            , "UPtr")
        
        if (this._COM_FileHandle = 0 || this._COM_FileHandle = -1) {
            this._COM_FileHandle := 0
            return 0 ; Gagal membuka file port
        }
        
        SCS_Result := DllCall("SetCommState", "UPtr", this._COM_FileHandle, "Ptr", DCB.Ptr)
        if (SCS_Result = 0) {
            this.__Close_COM()
            return 0 ; Gagal set state
        }

        TimeoutsBuf := Buffer(20)
        NumPut("UInt", 0xFFFFFFFF, TimeoutsBuf, 0)
        NumPut("UInt", 0, TimeoutsBuf, 4)
        NumPut("UInt", 1, TimeoutsBuf, 8)
        NumPut("UInt", 0, TimeoutsBuf, 12)
        NumPut("UInt", 100, TimeoutsBuf, 16)

        SCT_Result := DllCall("SetCommTimeouts", "UPtr", this._COM_FileHandle, "Ptr", TimeoutsBuf.Ptr)
        if (SCT_Result = 0) {
            this.__Close_COM()
            return 0 ; Gagal set timeout
        }
        
        this._Is_Actually_Connected := true ; Set flag koneksi berhasil
        return 1
    }

    __Close_COM()
    {
        if (this._COM_FileHandle != 0) {
            DllCall("CloseHandle", "UPtr", this._COM_FileHandle)
            this._COM_FileHandle := 0
        }
    }

    __WriteBytes(bufferToWrite)
    {
        if (!this._Is_Actually_Connected || this._COM_FileHandle = 0) {
            ; Jika tidak terkoneksi, jangan coba menulis.
            ; Pemanggil (WriteString/SendByteString) bisa menambahkan notifikasi jika mau.
            return 0 
        }

        this.__StopTimer()
        this._Is_Available := false

        Data_Length := bufferToWrite.Size
        pBytesSent := Buffer(4)
        WF_Result := 0
        Bytes_Sent := 0

        try {
            WF_Result := DllCall("WriteFile"
                , "UPtr", this._COM_FileHandle
                , "Ptr", bufferToWrite.Ptr
                , "UInt", Data_Length
                , "Ptr", pBytesSent.Ptr
                , "Ptr", 0)
            Bytes_Sent := NumGet(pBytesSent, 0, "UInt")

            if (WF_Result = 0 || Bytes_Sent != Data_Length) {
                LastError := A_LastError
                if (LastError = 1167 || LastError = 31 || LastError = 995) { ; ERROR_DEVICE_NOT_CONNECTED, ERROR_GEN_FAILURE, ERROR_OPERATION_ABORTED
                    this._Is_Actually_Connected := false
                    this.__Close_COM()
                    if (!this._Was_Disconnected_Notified) {
                        try 
                            ;Util_ShowToast("Serial Disconnected", "Write failed. Port " this._COM_Port " disconnected.", cableIcon, "C50C00", 3000)
                            ShowWinToast("Serial Disconnected", "Write failed. Port " this._COM_Port " disconnected.", cableIcon)
                        catch {

                        }
                        this._Was_Disconnected_Notified := true
                    }
                } else {
                    Sleep 10 ; Coba lagi sekali untuk error lain
                    WF_Result := DllCall("WriteFile", "UPtr", this._COM_FileHandle, "Ptr", bufferToWrite.Ptr, "UInt", Data_Length, "Ptr", pBytesSent.Ptr, "Ptr", 0)
                    Bytes_Sent := NumGet(pBytesSent, 0, "UInt")
                    if (WF_Result = 0 || Bytes_Sent != Data_Length) {
                        MsgBox "Failed Dll WriteFile to " this._COM_Port ".`nResult: " WF_Result "`nGLE: " A_LastError "`nData Length: " Data_Length "`nBytes Sent: " Bytes_Sent, "Write Error", 16
                    }
                }
            }
        } catch as e {
            MsgBox "Exception during WriteFile to " this._COM_Port ": " e.Message, "Write Exception", 16
            this._Is_Actually_Connected := false
            this.__Close_COM()
             if (!this._Was_Disconnected_Notified) {
                try 
                    ;Util_ShowToast("Serial Disconnected", "Write exception. Port " this._COM_Port " lost.", cableIcon, "C50C00", 3000)
                    ShowWinToast("Serial Disconnected", "Write exception. Port " this._COM_Port " lost.", cableIcon)
                catch {

                }
                this._Was_Disconnected_Notified := true
            }
        }
        
        this._Is_Available := true
        this.__ResumeTimer() ; Penting untuk melanjutkan timer agar bisa mencoba reconnect
        return Bytes_Sent
    }

    WriteString(asciiMessage, encoding := "CP0") {
        if (!this._Is_Actually_Connected || this._COM_FileHandle = 0) {
            ; Opsional: notifikasi jika diperlukan dari sini
            ; try Util_ShowToast("Serial Write Error", "Cannot write, port " this._COM_Port " not connected.", "YOUR_WARNING_ICON_PATH.png", "FFC400", 2000) catch {}
            return 0
        }
        
        ; ... (sisa logika StrPut dan buffer sama seperti sebelumnya) ...
        strByteLength := StrPut(asciiMessage, encoding) 
        if (encoding = "UTF-16" || encoding = "CP1200")
             strByteLength := (strByteLength - 1) * 2
        else if (strByteLength > 0)
             strByteLength -=1

        if (strByteLength <= 0 && asciiMessage != "") { 
            strByteLength := StrLen(asciiMessage)
        }
        if (strByteLength = 0 && asciiMessage = "") {
             Return this.__WriteBytes(Buffer(0))
        }

        buf := Buffer(strByteLength)
        StrPut(asciiMessage, buf, strByteLength, encoding)
        
        return this.__WriteBytes(buf)
    }
    
    __Read_from_COM(Num_Bytes_To_Read)
    {
        if (!this._Is_Available || this._COM_FileHandle = 0 || !this._Is_Actually_Connected)
            return "" ; Jangan coba baca jika kita tahu sudah disconnect atau port belum siap

        ReadBuf := Buffer(Num_Bytes_To_Read)
        pBytesReceived := Buffer(4)

        Read_Result := DllCall("ReadFile"
            , "UPtr", this._COM_FileHandle
            , "Ptr", ReadBuf.Ptr
            , "UInt", Num_Bytes_To_Read
            , "Ptr", pBytesReceived.Ptr
            , "Ptr", 0)
        
        Local_Bytes_Received := NumGet(pBytesReceived, 0, "UInt")

        if (Read_Result = 0) {
             LastError := A_LastError
             ; 995 = ERROR_OPERATION_ABORTED (bisa terjadi jika port ditutup saat operasi baca)
             ; 1167 = ERROR_DEVICE_NOT_CONNECTED
             ; 31 = ERROR_GEN_FAILURE (seringkali karena perangkat dicabut)
             if (LastError != 0 && (LastError = 1167 || LastError = 31 || LastError = 995)) { 
                this._Bytes_Received_Count := 0
                if (this._Is_Actually_Connected) { 
                    this._Is_Actually_Connected := false ; Set flag bahwa koneksi hilang
                    ; Notifikasi akan ditangani oleh __Check_Read_Register
                }
                return "__DISCONNECTED__" ; Kembalikan string khusus untuk menandakan disconnect
             } else if (LastError != 0) {
                ; Error lain yang lebih serius, laporkan tapi jangan ExitApp
                ; MsgBox("Critical ReadFile Error on " this._COM_Port ". Result: " Read_Result ", GLE: " LastError ". Check connection or port settings. Will attempt to reconnect.", "Read Error", 16)
                this._Is_Actually_Connected := false 
                this.__Close_COM() 
                return "__DISCONNECTED__" ; Perlakukan sebagai disconnect
             }
             this._Bytes_Received_Count := 0
             return "" ; Kasus lain Read_Result = 0 tanpa error spesifik (mis. timeout baca 0 byte)
        }

        this._Bytes_Received_Count := Local_Bytes_Received
        Data_HEX := ""
        if (this._Bytes_Received_Count > 0) {
            Loop this._Bytes_Received_Count {
                Data_HEX .= Format("{:02X}", NumGet(ReadBuf, A_Index - 1, "UChar"))
            }
            this._Last_Received_HEX := Data_HEX
        }
        return Data_HEX
    }

    Event_Parse_Start(Register_Function_Obj, Delims := "`r`n", Poll_Interval := 100, Omit_Chars := "")
    {
        this._Event_Delims := Delims
        this._Event_Omit_Chars := Omit_Chars
        this._Event_Registered_Function := Register_Function_Obj
        this._Poll_Interval_Saved := Poll_Interval
        this._Is_Actually_Connected := false ; Mulai dengan false, biarkan __Open_Port menentukan
        this._Was_Disconnected_Notified := false

        if (this._COM_FileHandle = 0) {
            if (this.__Open_Port() = 0) { ; Panggil __Open_Port() yang asli (bisa interaktif)
                ; MsgBox sudah ada di __Open_Port atau pemanggil (SetupSerialCommunication)
                return 0 ; Gagal buka awal
            }
            this._Is_Actually_Connected := true ; Jika __Open_Port() sukses
        } else {
             this._Is_Actually_Connected := true ; Sudah terbuka sebelumnya
        }

        this.__StartTimer(this._Poll_Interval_Saved)
        return 1
    }

    __StartTimer(Poll_Interval) {
        this._Poll_Interval_Saved := Poll_Interval
        if !(IsObject(this._BoundCheckReadRegister))
            this._BoundCheckReadRegister := this.__Check_Read_Register.Bind(this)
        
        SetTimer(this._BoundCheckReadRegister, this._Poll_Interval_Saved)
        this._IsTimerRunning := true
    }

    __StopTimer() {
        if IsObject(this._BoundCheckReadRegister) && this._IsTimerRunning {
            SetTimer(this._BoundCheckReadRegister, 0) ; Use 0 to disable timer in v2
        }
        this._IsTimerRunning := false
    }

    __ResumeTimer() {
        if IsObject(this._BoundCheckReadRegister) && !this._IsTimerRunning && this._Poll_Interval_Saved > 0 {
            SetTimer(this._BoundCheckReadRegister, this._Poll_Interval_Saved)
            this._IsTimerRunning := true
        }
    }
    
    Event_Parse_Pause() {
        this.__StopTimer()
    }

    Event_Parse_Resume() {
        this.__ResumeTimer()
    }

    Event_Parse_Stop()
    {
        this.__StopTimer()
        if (this._COM_FileHandle != 0)
            this.__Close_COM()
    }

    __Check_Read_Register()
    {
        this.__StopTimer() ; Hentikan timer saat ini

        if (!this._Is_Actually_Connected) {
            ; Jika tidak terkoneksi, coba buka port lagi (mode silent)
            if (this.__Open_Port_Silent()) { 
                this._Is_Actually_Connected := true
                this._Was_Disconnected_Notified := false ; Reset flag notifikasi
                try 
                    ;Util_ShowToast("Serial Connected", "Device on " this._COM_Port " `nis connected.", cableIcon, "0078D4", 3000)
                    ShowWinToast("Serial Connected", "Device on " this._COM_Port " `nis connected.", cableIcon)
                catch
                    MsgBox "Serial Connected: Device on " this._COM_Port " reconnected. (Toast N/A)", "Connection Status"
                
                SetTimer(this._BoundCheckReadRegister, this._Poll_Interval_Saved) ; Kembali ke polling normal
                this._IsTimerRunning := true
                return 
            } else {
                this._Is_Actually_Connected := false 
                if (!this._Was_Disconnected_Notified) {
                    try 
                        ;Util_ShowToast("Serial Disconnected", "Device on " this._COM_Port " lost.`nReconnecting...", cableIcon, "C50C00", 3000)
                        ShowWinToast("Serial Disconnected", "Device on " this._COM_Port " lost.`nReconnecting...", cableIcon)
                    catch
                        MsgBox "Serial Disconnected: Device on " this._COM_Port " lost. Retrying... (Toast N/A)", "Connection Status"
                    this._Was_Disconnected_Notified := true
                }
                SetTimer(this._BoundCheckReadRegister, this._Reconnect_Attempt_Interval) ; Coba lagi nanti
                this._IsTimerRunning := true
                return
            }
        }

        ; Jika sampai sini, _Is_Actually_Connected = true dan port seharusnya terbuka
        ReceivedHex := ""
        try {
            ReceivedHex := this.__Read_from_COM(255)
        } catch as e {
            this._Is_Actually_Connected := false ; Asumsikan disconnect jika ada exception tak terduga
            this.__Close_COM()
            if (!this._Was_Disconnected_Notified) {
                 try 
                    ;Util_ShowToast("Serial Read Error", "Error: " e.Message ". Disconnecting.", cableIcon, "C50C00", 3000)
                    ShowWinToast("Serial Read Error", "Error: " e.Message ". Disconnecting.", cableIcon)
                 catch
                    MsgBox "Serial Read Error: " e.Message ". Disconnecting. (Toast N/A)", "Connection Error"
                 this._Was_Disconnected_Notified := true
            }
            SetTimer(this._BoundCheckReadRegister, this._Reconnect_Attempt_Interval)
            this._IsTimerRunning := true
            return
        }
        
        if (ReceivedHex = "__DISCONNECTED__") {
            ; __Read_from_COM mendeteksi disconnect, _Is_Actually_Connected sudah di-set false di sana
            this.__Close_COM() 
            if (!this._Was_Disconnected_Notified) {
                try 
                    ;Util_ShowToast("Serial Disconnected", "Device on " this._COM_Port " lost.`nReconnecting...", cableIcon, "C50C00", 3000)
                    ShowWinToast("Serial Disconnected", "Device on " this._COM_Port " lost.`nCheck your cable and try to reconnect", cableIcon)
                catch
                     MsgBox "Serial Disconnected: Device on " this._COM_Port " disconnected. Retrying... (Toast N/A)", "Connection Status"
                this._Was_Disconnected_Notified := true
            }
            SetTimer(this._BoundCheckReadRegister, this._Reconnect_Attempt_Interval)
            this._IsTimerRunning := true
            return
        }

        if (ReceivedHex != "") {
            TranslatedASCII := this.__HexToASCII(ReceivedHex)
            this._Received_Partial .= TranslatedASCII
        }

        if (this._Event_Delims is String && InStr(this._Received_Partial, this._Event_Delims))
        {
            Parts := StrSplit(this._Received_Partial, this._Event_Delims, this._Event_Omit_Chars)
            ProcessedChars := 0
            Loop Parts.Length - 1 { 
                CurrentPart := Parts[A_Index]
                if (Trim(CurrentPart) != "" || this._Event_Omit_Chars = "") { 
                    this.Chunk := CurrentPart
                    this.AllReceived .= CurrentPart this._Event_Separator
                    if IsObject(this._Event_Registered_Function)
                        try this._Event_Registered_Function.Call(CurrentPart)
                        catch as e
                           MsgBox "Error in Event Callback Function: " e.Message, "Callback Error", 16
                }
                ProcessedChars += StrLen(CurrentPart) + StrLen(this._Event_Delims)
            }
            
            if (ProcessedChars > 0 && ProcessedChars <= StrLen(this._Received_Partial))
                this._Received_Partial := SubStr(this._Received_Partial, ProcessedChars + 1)
            else if (Parts.Length > 0 && Parts[Parts.Length] = "" && (StrLen(this._Received_Partial) >= StrLen(this._Event_Delims) && SubStr(this._Received_Partial, -StrLen(this._Event_Delims) + 1) = this._Event_Delims))
                this._Received_Partial := ""
            else if (Parts.Length > 0)
                this._Received_Partial := Parts[Parts.Length]
        }

        this.__ResumeTimer() ; Akan menggunakan _Poll_Interval_Saved
    }

    __HexToASCII(HexStr)
    {
        AsciiTranslation := ""
        Loop (StrLen(HexStr) // 2) {
            CurrentHexByte := SubStr(HexStr, (A_Index * 2) - 1, 2)
            try AsciiTranslation .= Chr("0x" CurrentHexByte)
            catch
                AsciiTranslation .= "?" ; Handle invalid hex byte
        }
        return AsciiTranslation
    }

    __AscToHex_Recursive(str) {
		Return str="" ? "" : Format("{:02X}", Ord(SubStr(str,1,1))) this.__AscToHex_Recursive(SubStr(str,2))
	}

    Send_Byte_String(commaSeparatedByteString) {
        if (!this._Is_Actually_Connected || this._COM_FileHandle = 0) {
            ; Opsional: notifikasi jika diperlukan dari sini
            ; try Util_ShowToast("Serial Write Error", "Cannot send bytes, port " this._COM_Port " not connected.", "YOUR_WARNING_ICON_PATH.png", "FFC400", 2000) catch {}
            return 0
        }

        ; ... (sisa logika StrSplit dan buffer sama seperti sebelumnya) ...
        parts := StrSplit(commaSeparatedByteString, ",", A_Space A_Tab)
        dataBuf := Buffer(parts.Length)
        For i, partStr in parts {
            NumPut("UChar", Integer(Trim(partStr)), dataBuf, i - 1)
        }
        return this.__WriteBytes(dataBuf)
    }
}