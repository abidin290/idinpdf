#pragma once

#include <map>
#include <string>
#include <vector>

struct AppConfig {
  bool ktp_mode = false;
  std::string header_text = "Dokumentasi Kegiatan";
  std::string header_font = "Helvetica-Bold";
  int header_size = 16;
  int header_margin_mm = 15;
  float border_thickness = 0.5f;
  int margin_mm = 5;
  int spacing_mm = 3;
  int jpeg_quality = 90;

  bool enable_captions = false;
  std::string csv_file_name = "keterangan.csv";
  int caption_font_size = 7;
  int caption_text_padding_mm = 2;

  bool enable_watermark = false;
  std::string watermark_text = "DOKUMEN DESA";
  int watermark_opacity = 60; // Opacity level 0-255

  bool enable_auto_rotate = true;
};

void create_csv_template(const std::string &csv_path,
                         const std::vector<std::string> &image_files);
std::map<std::string, std::string>
load_captions_from_csv(const std::string &csv_path,
                       const std::vector<std::string> &image_files);
AppConfig load_or_create_config(const std::string &working_dir);
std::vector<std::string> collect_images(const std::string &folder);

void make_pdf(const std::vector<std::string> &images, int cols, int rows,
              const std::string &out_pdf, const AppConfig &cfg,
              const std::map<std::string, std::string> &captions);
