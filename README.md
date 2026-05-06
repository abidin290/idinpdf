# 📄 IdinPDF Generator (C++ Edition)

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()
[![Build](https://img.shields.io/badge/Build-CMake-green.svg)]()

**IdinPDF** adalah utilitas baris perintah (CLI) dan ekstensi menu konteks Windows (*Send To*) yang dirancang khusus untuk mempercepat pembuatan dokumen PDF dari kumpulan foto. Aplikasi ini sangat cocok untuk keperluan **Dokumentasi Kegiatan Desa**, penyusunan laporan lapangan, dan pembuatan **Fotokopi KTP Massal** secara instan.

Awalnya dibangun menggunakan Python, versi ini telah **sepenuhnya ditulis ulang menggunakan C++ murni**. Menghasilkan aplikasi tunggal (`.exe`) secepat kilat, tanpa memerlukan instalasi Python, tanpa dependensi eksternal yang memberatkan, dan 100% *portable*.

---

## ✨ Fitur Utama

- 🚀 **Performa Tinggi & Portabel:** Ditulis dalam C++ murni tanpa virtual machine (VM) atau interpreter. Dikompilasi secara *statis* (Static Linking) sehingga 100% mandiri dan tidak memerlukan file DLL bawaan *compiler* (bebas *error libstdc++*).
- 🔲 **Auto Grid & Custom Grid:** Susun foto ke dalam format Grid kustom (contoh: 2x3, 4x2). Kini dilengkapi dengan fitur "Custom Grid" interaktif langsung dari menu klik kanan.
- 💳 **Mode KTP Cerdas:** Mode khusus untuk "Fotokopi KTP". Otomatis menyatukan banyak foto KTP warga dengan gambar pendamping (Garuda / `dpnktp.dll`) di bawahnya agar pas dicetak.
- 🖱️ **Integrasi Windows Send To:** Terintegrasi langsung dengan klik-kanan Windows. Cukup blok (seleksi) banyak foto -> Klik Kanan -> *Send To* -> `IdinPDF` untuk langsung membuat PDF tanpa membuka terminal.
- ⚙️ **Kustomisasi `config.ini`:** Aplikasi otomatis membuat file konfigurasi di setiap folder foto. Anda bebas mengubah:
  - Judul Dokumen (Mendukung Multi-Baris menggunakan `|`).
  - Ketebalan Border & Margin (mm).
  - Watermark Teks & Opasitas transparan.
  - Rotasi Otomatis (EXIF Orientation).
- 📦 **Portable Resource DLL:** Gambar garuda/identitas pendamping dikemas secara aman ke dalam file `dpnktp.dll` melalui Windows Resource Compiler, memastikan kebersihan folder kerja Anda.

---

## 🛠️ Cara Build (Kompilasi) dari Source Code

Proyek ini menggunakan sistem build **CMake**. Pastikan komputer Anda telah terinstal MinGW-w64 (GCC/G++) dan CMake.

1. Buka Terminal / CMD di dalam folder proyek.
2. Jalankan skrip kompilasi bawaan:
   ```cmd
   build.bat
   ```
3. CMake akan otomatis mengunduh pustaka (library) pihak ketiga dan mengkompilasinya menjadi `idinpdf.exe` dan `dpnktp.dll` di dalam folder `build/`.

### Library Pihak Ketiga yang Digunakan (Auto-Fetched):
- [pdfgen](https://github.com/AndreRenaud/PDFGen) - C PDF Generator murni.
- [stb_image & stb_image_resize](https://github.com/nothings/stb) - Pemuat dan pemroses gambar.
- [easyexif](https://github.com/mayanklahiri/easyexif) - Pembaca metadata EXIF untuk auto-rotate.
- [mINI](https://github.com/pulzed/mINI) - Pembaca/penulis file konfigurasi INI.

---

## 📖 Panduan Penggunaan & Instalasi

Aplikasi ini dapat digunakan dengan 2 cara: **Instalasi Penuh (Installer)** atau **Mode Portable (CLI)**.

### 1. Instalasi Sangat Mudah (Sangat Direkomendasikan)
Untuk pengguna umum Windows 10 ke atas, cukup jalankan file **`IdinPDF_Installer.exe`** (jika Anda sudah mem-build-nya melalui Inno Setup). 
Installer otomatis akan:
- Memasang IdinPDF ke sistem tanpa membutuhkan hak akses Administrator.
- Menambahkan Path IdinPDF secara otomatis agar bisa dipanggil dari CMD di mana saja.
- Menciptakan 3 menu ajaib di dalam fitur *Send To* Windows Anda:

1. `IdinPDF - Auto Grid`: Menyusun otomatis semua gambar yang Anda blok ke dalam 1 halaman PDF.
2. `IdinPDF - Mode KTP`: Menyusun KTP dengan garuda di bawahnya.
3. `IdinPDF - Custom Grid` ✨: Akan memunculkan jendela hitam sebentar untuk menanyakan kepada Anda berapa jumlah kolom dan baris yang Anda inginkan secara interaktif!

**Cara Pakai:** Buka File Explorer -> Seleksi/Blok beberapa foto -> Klik Kanan -> *Send to* -> Pilih salah satu menu *IdinPDF*. File PDF akan tercipta secara instan!

*(Bagi developer: Anda masih bisa menginstal shortcut secara manual tanpa installer Inno Setup dengan mengetik perintah `.\idinpdf install` di terminal).*

### 2. Penggunaan via Terminal (Advanced)
Jika Anda membutuhkan kontrol penuh (misalnya mendikte secara ketat jumlah kolom dan baris):

```cmd
# Menyusun foto dalam grid 2 kolom dan 3 baris di folder saat ini
.\idinpdf 2 3

# Memproses foto dari folder spesifik dan menentukan nama output
.\idinpdf 2 3 -p "C:\Dokumentasi\Desa_A" -o "Laporan_Banjir.pdf"

# Menjalankan Mode KTP via terminal
.\idinpdf ktp -p "C:\Dokumentasi\KTP_Warga"

# Menampilkan panduan bantuan
.\idinpdf help
```

---

## ⚙️ Kustomisasi `config.ini`

Setiap kali Anda memproses sebuah folder baru, aplikasi akan melahirkan file `config.ini` di folder tersebut jika belum ada. Buka dengan Notepad untuk mengubah tampilannya:

```ini
[settings]
; Teks judul di atas halaman. Gunakan | untuk baris baru.
header_text=Dokumentasi Kegiatan|Tahun 2026
header_font=Helvetica-Bold
header_size=16
border_thickness=0.5
margin_mm=5
spacing_mm=3

; Mengaktifkan teks transparan melintang di foto
enable_watermark=true
watermark_text=DOKUMEN DESA RAHASIA
```

---

## 📜 Lisensi & Atribusi
Proyek ini dibuat untuk keperluan otomatisasi administrasi dan terbuka untuk modifikasi *open-source*. Hak cipta untuk pustaka pihak ketiga (*stb*, *pdfgen*, dll) sepenuhnya milik masing-masing pembuatnya.
