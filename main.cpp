#include "pdf_generator.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;

void print_usage() {
  std::cout
      << "Penggunaan: idinpdf <columns> <rows> [-p <path>] [-o <output>]\n"
      << "       ATAU: idinpdf ktp [-p <path>] [-o <output>]\n"
      << "       ATAU: idinpdf install (Untuk memunculkan menu klik kanan)\n"
      << "Contoh: idinpdf 2 3 -p \"C:\\foto_kegiatan\" -o hasil.pdf\n";
}

void calculate_auto_grid(int count, int& cols, int& rows) {
  if (count <= 1) { cols = 1; rows = 1; }
  else if (count == 2) { cols = 1; rows = 2; }
  else if (count <= 4) { cols = 2; rows = 2; }
  else if (count <= 6) { cols = 2; rows = 3; }
  else if (count <= 8) { cols = 2; rows = 4; }
  else if (count <= 9) { cols = 3; rows = 3; }
  else if (count <= 12) { cols = 3; rows = 4; }
  else { cols = 3; rows = (count + 2) / 3; }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  bool ktp_mode = false;
  bool auto_fit_mode = false;
  int cols = 0, rows = 0;
  int arg_start = 3;

  std::string first_arg = argv[1];

  if (first_arg == "help") {
    std::cout << "=================================================\n";
    std::cout << "           PANDUAN PENGGUNAAN IDINPDF            \n";
    std::cout << "=================================================\n\n";
    std::cout << "Aplikasi ini memiliki 2 Mode Utama:\n\n";
    std::cout << "1. MODE GRID (Kolase Foto)\n";
    std::cout << "   Digunakan untuk menyusun banyak foto ke dalam satu "
                 "halaman PDF.\n";
    std::cout << "   Perintah : idinpdf <kolom> <baris> [-p <path_folder>]\n";
    std::cout << "   Contoh   : idinpdf 2 3\n";
    std::cout
        << "              (Menyusun 2 kolom dan 3 baris foto per halaman).\n\n";
    std::cout << "2. MODE KTP (Fotokopi KTP Otomatis)\n";
    std::cout
        << "   Digunakan untuk merapikan foto KTP warga agar pas dicetak.\n";
    std::cout << "   Aplikasi akan otomatis mencari file 'dpnktp.dll' atau "
                 "'dpnktp.jpg'\n";
    std::cout << "   sebagai gambar pendamping di bagian bawah.\n";
    std::cout << "   Perintah : idinpdf ktp [-p <path_folder>]\n\n";
    std::cout << "=================================================\n";
    std::cout << "PENGATURAN TAMBAHAN (via config.ini)\n";
    std::cout << "- Anda dapat mengubah Teks Judul, Margin, Ukuran Kertas,\n";
    std::cout << "  hingga Watermark dengan membuka file 'config.ini' yang\n";
    std::cout << "  otomatis terbuat di folder foto Anda.\n";
    std::cout
        << "- Untuk judul multi-baris, gunakan karakter '|' sebagai pemisah.\n";
    std::cout << "  Contoh: header_text=Dokumentasi Kegiatan|Tahun 2026\n";
    std::cout << "=================================================\n";
    return 0;
  }

  if (first_arg == "install") {
    std::string exe_path;
    try {
        exe_path = fs::canonical(fs::path(argv[0])).string();
    } catch (...) {
        exe_path = fs::absolute(fs::path(argv[0])).string();
    }
    
    std::string esc_exe;
    for (char c : exe_path) {
        if (c == '\\') esc_exe += "\\\\";
        else esc_exe += c;
    }
    std::ofstream vbs("temp_shortcut.vbs");
    vbs << "Set ws = CreateObject(\"WScript.Shell\")\n";
    vbs << "Set fso = CreateObject(\"Scripting.FileSystemObject\")\n";
    vbs << "stFolder = ws.SpecialFolders(\"SendTo\")\n";
    vbs << "On Error Resume Next\n";
    vbs << "fso.DeleteFile stFolder & \"\\IdinPDF*.lnk\"\n";
    vbs << "On Error GoTo 0\n";
    
    // 1. KTP
    vbs << "Set link = ws.CreateShortcut(stFolder & \"\\IdinPDF - Mode KTP.lnk\")\n";
    vbs << "link.TargetPath = \"" << exe_path << "\"\n";
    vbs << "link.Arguments = \"ktp\"\n";
    vbs << "link.Save\n";
    
    // 2. Auto Grid
    vbs << "Set link2 = ws.CreateShortcut(stFolder & \"\\IdinPDF - Auto Grid.lnk\")\n";
    vbs << "link2.TargetPath = \"" << exe_path << "\"\n";
    vbs << "link2.Arguments = \"auto\"\n";
    vbs << "link2.Save\n";
    
    // 3. Custom Grid
    vbs << "Set link3 = ws.CreateShortcut(stFolder & \"\\IdinPDF - Custom Grid.lnk\")\n";
    vbs << "link3.TargetPath = \"" << exe_path << "\"\n";
    vbs << "link3.Arguments = \"custom\"\n";
    vbs << "link3.Save\n";
    
    vbs.close();
    system("cscript //nologo temp_shortcut.vbs");
    std::remove("temp_shortcut.vbs");
    
    std::cout << "BERHASIL! Fitur 'SendTo' Multi-Shortcut telah terinstal.\n";
    std::cout << "Sekarang Anda bisa memblok beberapa foto sekaligus, lalu Klik Kanan -> Send To.\n";
    std::cout << "Anda akan melihat 2 pilihan cerdas IdinPDF siap digunakan!\n";
    return 0;
  }


  
  if (first_arg == "ktp") {
      ktp_mode = true;
      arg_start = 2;
  } else if (first_arg == "auto") {
      auto_fit_mode = true;
      arg_start = 2;
  } else if (first_arg == "custom") {
      arg_start = 2;
      std::cout << "\n==================================\n";
      std::cout << "       IDINPDF CUSTOM GRID        \n";
      std::cout << "==================================\n";
      std::cout << "Masukkan jumlah Kolom : ";
      std::cin >> cols;
      std::cout << "Masukkan jumlah Baris : ";
      std::cin >> rows;
      if (std::cin.fail() || cols <= 0 || rows <= 0) {
          std::cout << "Error: Input tidak valid. Harus angka > 0.\n";
          return 1;
      }
  } else if (fs::exists(first_arg) && fs::is_regular_file(first_arg)) {
      auto_fit_mode = true;
      arg_start = 1;
  } else {
      if (argc < 3) {
        print_usage();
        return 1;
      }
      try {
          cols = std::stoi(argv[1]);
          rows = std::stoi(argv[2]);
          if (cols <= 0 || rows <= 0) {
              std::cout << "Error: Kolom dan baris harus lebih besar dari 0.\n";
              return 1;
          }
      } catch (...) {
          std::cout << "Error: Argumen tidak valid.\n";
          print_usage();
          return 1;
      }
      arg_start = 3;
  }

  std::vector<std::string> imgs;
  std::string path = fs::current_path().string();
  std::string output = "idinpdf.pdf";

  // Check if there are raw files appended (SendTo mode check after ktp/auto)
  if (arg_start < argc && fs::exists(argv[arg_start]) && fs::is_regular_file(argv[arg_start])) {
      for (int i = arg_start; i < argc; ++i) {
          if (fs::is_regular_file(argv[i])) {
              imgs.push_back(argv[i]);
          }
      }
      path = fs::path(imgs[0]).parent_path().string();
  } else {
      // Normal arg parsing
      for (int i = arg_start; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" || arg == "--path") {
          if (i + 1 < argc) path = argv[++i];
        } else if (arg == "-o" || arg == "--output") {
          if (i + 1 < argc) output = argv[++i];
        }
      }

      if (!fs::is_directory(path)) {
        std::cout << "Error: Direktori yang diberikan tidak ditemukan -> " << path << "\n";
        return 1;
      }
      
      imgs = collect_images(path);
  }

  if (auto_fit_mode) {
      calculate_auto_grid(imgs.size(), cols, rows);
  }

  AppConfig config = load_or_create_config(path);
  config.ktp_mode = ktp_mode;

  if (imgs.empty()) {
    std::cout << "Tidak ada gambar ditemukan.\n";
    return 1;
  }

  std::map<std::string, std::string> captions;
  if (config.enable_captions) {
    std::string csv_path = (fs::path(path) / config.csv_file_name).string();
    captions = load_captions_from_csv(csv_path, imgs);

    if (captions.find("_CREATED_") != captions.end()) {
      std::cout << "Silakan isi file CSV yang baru dibuat dan jalankan lagi "
                   "program ini.\n";
      return 0;
    }
  }

  std::string output_file_path = (fs::path(path) / output).string();
  make_pdf(imgs, cols, rows, output_file_path, config, captions);

  return 0;
}
