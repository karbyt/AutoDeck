; --- KONFIGURASI SERIAL PORT ---
global Config_ComPort := "COM6"      ; Ganti dengan COM Port ESP32 Anda
global Config_BaudRate := 115200     ; Baudrate default

; --- Aksi untuk masing-masing tombol K1 sampai K12 ---
K1() {
    Send "Tes Baris 1 {Enter}"
    Send "Tes Baris 2 {Enter}{Up}{Up}"

}

K2() {
    Send "Pada pukul 07:45:33 WIB, Rina menerima e-mail berjudul “Laporan #Q3-2025: Proyeksi & Realisasi” dari alamat finance_divisi@corp-id.net; isi pesannya mencantumkan angka-angka penting seperti pendapatan sebesar Rp1.234.567.890,-, pengeluaran Rp987.654.321,-, serta margin laba kotor sebesar ±19,87%. Dalam lampiran “data_final(versi_2b).xlsx”, terdapat sheet bernama “Grafik_∆%_2025” yang menampilkan fluktuasi ±∆5,25% bulanan, dengan catatan kaki: Data valid per 31/03/2025, update berikutnya tgl. 30/06/2025 @ 10:00. Harap konfirmasi via WhatsApp ke +62-812-3456-7890 atau gunakan tautan https://bit.ly/laporanQ3_2025."
}

K3() {
    Run "calc.exe"
}

K4() {
    Send "Hello from Macropad Key 4!{Enter}"
    MsgBox "Baris pertama"
    Sleep 500
    MsgBox "Baris kedua setelah jeda"
}

K5() {
    Send "{Ctrl}{Shift}{Esc}"
}

; Tambahkan K5, K6, dst sesuai kebutuhan
