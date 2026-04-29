#include "pdf_generator.hpp"
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

void print_usage() {
  std::cout
      << "Penggunaan: idinpdf <columns> <rows> [-p <path>] [-o <output>]\n"
      << "       ATAU: idinpdf ktp [-p <path>] [-o <output>]\n"
      << "Contoh: idinpdf 2 3 -p \"C:\\foto_kegiatan\" -o hasil.pdf\n";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  bool ktp_mode = false;
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

  if (first_arg == "ktp") {
    ktp_mode = true;
    arg_start = 2;
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
      std::cout << "Error: Kolom dan baris harus berupa angka.\n";
      print_usage();
      return 1;
    }
  }

  std::string path = fs::current_path().string();
  std::string output = "idinpdf.pdf";

  for (int i = arg_start; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-p" || arg == "--path") {
      if (i + 1 < argc) {
        path = argv[++i];
      }
    } else if (arg == "-o" || arg == "--output") {
      if (i + 1 < argc) {
        output = argv[++i];
      }
    }
  }

  if (!fs::is_directory(path)) {
    std::cout << "Error: Direktori yang diberikan tidak ditemukan -> " << path
              << "\n";
    return 1;
  }

  AppConfig config = load_or_create_config(path);
  config.ktp_mode = ktp_mode;
  std::vector<std::string> imgs = collect_images(path);

  if (imgs.empty()) {
    std::cout << "Tidak ada gambar ditemukan di '" << path << "'.\n";
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
