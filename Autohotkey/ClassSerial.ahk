; ClassSerial.ahk (AHK v2)
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
            ; 3 = YesNoCancel buttons, 32 = IconQuestion
            UserChoice := MsgBox("It appears that the device may not be connected to " this._COM_Port ".`nWould you like to change COM ports?", "COM Failure", 3 | 32)
            if (UserChoice = "Yes") {
                InputObj := InputBox("Select New COM port", "Which COM port should be selected (e.g., COM1, COM2)?", {Default: this._COM_Port})
                if InputObj.Result = "OK"
                    this._COM_Port := InputObj.Value
                else if InputObj.Result = "Cancel"
                    return 0
                Settings_String := Trim(this._COM_Port) ":baud=" this._Baud_Rate " parity=" this._Parity " data=" this._Data_Bits " stop=" this._Stop_Bits " dtr=Off"
            } else {
                return 0
            }
        }

        COM_Port_Name := StrLen(this._COM_Port) > 4 ? "\\.\" Trim(this._COM_Port) : Trim(this._COM_Port)

        Loop 20 {
            this._COM_FileHandle := DllCall("CreateFile"
                , "Str", COM_Port_Name
                , "UInt", 0xC0000000
                , "UInt", 3
                , "Ptr", 0
                , "UInt", 3
                , "UInt", 0
                , "Ptr", 0
                , "UPtr")
            if (this._COM_FileHandle != 0 && this._COM_FileHandle != -1)
                break
            this._COM_FileHandle := 0
            Sleep 100
            if (A_Index = 20) {
                MsgBox "There is a problem with Serial Port communication.`nFailed Dll CreateFile, COM_FileHandle=" this._COM_FileHandle "`nThe Script Will Now Exit.", "Error", 16 ; 16 = IconError
                ExitApp()
            }
        }
        
        SCS_Result := DllCall("SetCommState", "UPtr", this._COM_FileHandle, "Ptr", DCB.Ptr)
        if (SCS_Result = 0) {
            MsgBox "There is a problem with Serial Port communication.`nFailed Dll SetCommState.`nThe Script Will Now Exit.", "Error", 16 ; 16 = IconError
            this.__Close_COM()
            return 0
        }

        TimeoutsBuf := Buffer(20)
        NumPut("UInt", 0xFFFFFFFF, TimeoutsBuf, 0)
        NumPut("UInt", 0, TimeoutsBuf, 4)
        NumPut("UInt", 1, TimeoutsBuf, 8)
        NumPut("UInt", 0, TimeoutsBuf, 12)
        NumPut("UInt", 100, TimeoutsBuf, 16)

        SCT_Result := DllCall("SetCommTimeouts", "UPtr", this._COM_FileHandle, "Ptr", TimeoutsBuf.Ptr)
        if (SCT_Result = 0) {
            MsgBox "There is a problem with Serial Port communication.`nFailed Dll SetCommTimeouts.`nThe Script Will Now Exit.", "Error", 16 ; 16 = IconError
            this.__Close_COM()
            ExitApp()
        }
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
        if (this._COM_FileHandle = 0) {
            MsgBox "COM Port not open for writing.", "Error", 16 ; 16 = IconError
            return 0
        }

        this.__StopTimer()
        this._Is_Available := false

        Data_Length := bufferToWrite.Size
        pBytesSent := Buffer(4)

        WF_Result := DllCall("WriteFile"
            , "UPtr", this._COM_FileHandle
            , "Ptr", bufferToWrite.Ptr
            , "UInt", Data_Length
            , "Ptr", pBytesSent.Ptr
            , "Ptr", 0)
        
        Bytes_Sent := NumGet(pBytesSent, 0, "UInt")

        if (WF_Result = 0 || Bytes_Sent != Data_Length) {
            Sleep 10
            WF_Result := DllCall("WriteFile"
                , "UPtr", this._COM_FileHandle
                , "Ptr", bufferToWrite.Ptr
                , "UInt", Data_Length
                , "Ptr", pBytesSent.Ptr
                , "Ptr", 0)
            Bytes_Sent := NumGet(pBytesSent, 0, "UInt")
            if (WF_Result = 0 || Bytes_Sent != Data_Length) {
                MsgBox "Failed Dll WriteFile to " this._COM_Port ".`nResult: " WF_Result "`nData Length: " Data_Length "`nBytes Sent: " Bytes_Sent, "Write Error", 16 ; 16 = IconError
            }
        }
        
        this._Is_Available := true
        this.__ResumeTimer()
        return Bytes_Sent
    }

    WriteString(asciiMessage, encoding := "CP0") {
        if (this._COM_FileHandle = 0) {
            if (this.__Open_Port() = 0) {
                MsgBox "Failed to open COM port for writing string.", "Error", 16 ; 16 = IconError
                return 0
            }
        }
        
        strByteLength := StrPut(asciiMessage, encoding) -1
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
        if (!this._Is_Available || this._COM_FileHandle = 0)
            return ""

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
             if (LastError != 0 && LastError != 995) { ; 995 = ERROR_OPERATION_ABORTED
                MsgBox("Failed Dll ReadFile on " this._COM_Port ". Result: " Read_Result ", GLE: " LastError ".`nThe Script Will Now Exit.", "Read Error", 16) ; 16 = IconError
                this.__Close_COM()
                ExitApp()
             }
             this._Bytes_Received_Count := 0
             return ""
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

        if (this._COM_FileHandle = 0) {
            if (this.__Open_Port() = 0) {
                MsgBox "Could not open COM port " this._COM_Port " for event parsing.", "Error", 16 ; 16 = IconError
                return 0
            }
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
            SetTimer(this._BoundCheckReadRegister, 0)
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
        this.__StopTimer()
        ReceivedHex := ""
        try { ; Tambahkan try-catch di sekitar operasi yang mungkin gagal saat disconnect
            ReceivedHex := this.__Read_from_COM(255)
        } catch as e {
            ; MsgBox "Error during __Read_from_COM: " e.Message, "Read Exception", 16
            ; Jika ReadFile gagal karena disconnect, itu akan ditangani di dalam __Read_from_COM.
            ; Jika masalah lain, ini akan menangkapnya.
            ; Untuk saat ini, kita bisa biarkan saja karena __Read_from_COM sudah ExitApp jika error parah.
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
                if (Trim(CurrentPart) != "") {
                    this.Chunk := CurrentPart
                    this.AllReceived .= CurrentPart this._Event_Separator
                    if IsObject(this._Event_Registered_Function)
                        this._Event_Registered_Function.Call(CurrentPart)
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

        this.__ResumeTimer()
    }

    __HexToASCII(HexStr)
    {
        AsciiTranslation := ""
        Loop (StrLen(HexStr) // 2) {
            CurrentHexByte := SubStr(HexStr, (A_Index * 2) - 1, 2)
            AsciiTranslation .= Chr("0x" CurrentHexByte)
        }
        return AsciiTranslation
    }

    __AscToHex_Recursive(str) {
		Return str="" ? "" : Format("{:02X}", Ord(SubStr(str,1,1))) this.__AscToHex_Recursive(SubStr(str,2))
	}

    Send_Byte_String(commaSeparatedByteString) {
        if (this._COM_FileHandle = 0) {
            if (this.__Open_Port() = 0) {
                MsgBox "Failed to open COM port for writing byte string.", "Error", 16 ; 16 = IconError
                return 0
            }
        }
        parts := StrSplit(commaSeparatedByteString, ",")
        dataBuf := Buffer(parts.Length)
        For i, partStr in parts {
            NumPut("UChar", Integer(Trim(partStr)), dataBuf, i - 1)
        }
        return this.__WriteBytes(dataBuf)
    }
}