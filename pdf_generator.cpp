#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern "C" {
#include "pdfgen.h"
}

#include "exif.h"
#include "mini/ini.h"
#include "pdf_generator.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

std::string get_executable_dir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path().string();
#else
    return fs::current_path().string();
#endif
}

// 1 mm = 2.83465 points
#define MM2PT(x) ((x) * 2.83465f)

// F4 Page Size in points
const float PAGE_WIDTH = 215.0f * 2.83465f;
const float PAGE_HEIGHT = 330.0f * 2.83465f;

// Helper to write STB JPEG to memory buffer
struct MemBuf {
    unsigned char* data = nullptr;
    int size = 0;
};

void stbi_write_mem(void *context, void *data, int size) {
    MemBuf* buf = (MemBuf*)context;
    buf->data = (unsigned char*)realloc(buf->data, buf->size + size);
    memcpy(buf->data + buf->size, data, size);
    buf->size += size;
}

void create_csv_template(const std::string& csv_path, const std::vector<std::string>& image_files) {
    std::ofstream file(csv_path);
    if (file.is_open()) {
        file << "NamaFileGambar,Keterangan\n";
        for (const auto& img : image_files) {
            file << fs::path(img).filename().string() << ",\n";
        }
        file.close();
        std::cout << "Template CSV '" << fs::path(csv_path).filename().string() << "' telah dibuat. Silakan isi keterangannya.\n";
    }
}

std::map<std::string, std::string> load_captions_from_csv(const std::string& csv_path, const std::vector<std::string>& image_files) {
    std::map<std::string, std::string> captions;
    if (!fs::exists(csv_path)) {
        create_csv_template(csv_path, image_files);
        // We throw an exception or return a special map to indicate we just created it
        captions["_CREATED_"] = "YES";
        return captions;
    }

    std::ifstream file(csv_path);
    std::string line;
    bool first_line = true;
    while (std::getline(file, line)) {
        if (first_line) { first_line = false; continue; } // skip header
        size_t comma = line.find(',');
        if (comma != std::string::npos) {
            std::string filename = line.substr(0, comma);
            std::string caption = line.substr(comma + 1);
            
            // Trim carriage returns
            filename.erase(std::remove(filename.begin(), filename.end(), '\r'), filename.end());
            caption.erase(std::remove(caption.begin(), caption.end(), '\r'), caption.end());
            
            if (!caption.empty()) {
                captions[filename] = caption;
            }
        }
    }
    std::cout << "Ditemukan " << captions.size() << " keterangan dari CSV.\n";
    return captions;
}

AppConfig load_or_create_config(const std::string& working_dir) {
    AppConfig cfg;
    std::string config_path = (fs::path(working_dir) / "config.ini").string();
    mINI::INIFile file(config_path);
    mINI::INIStructure ini;
    
    bool file_exists = file.read(ini);

    if (file_exists) {
        if (ini.has("settings")) {
            if (ini["settings"].has("header_text")) cfg.header_text = ini["settings"]["header_text"];
            if (ini["settings"].has("header_font")) cfg.header_font = ini["settings"]["header_font"];
            if (ini["settings"].has("header_size")) cfg.header_size = std::stoi(ini["settings"]["header_size"]);
            if (ini["settings"].has("header_margin_mm")) cfg.header_margin_mm = std::stoi(ini["settings"]["header_margin_mm"]);
            if (ini["settings"].has("border_thickness")) cfg.border_thickness = std::stof(ini["settings"]["border_thickness"]);
            if (ini["settings"].has("margin_mm")) cfg.margin_mm = std::stoi(ini["settings"]["margin_mm"]);
            if (ini["settings"].has("spacing_mm")) cfg.spacing_mm = std::stoi(ini["settings"]["spacing_mm"]);
            if (ini["settings"].has("jpeg_quality")) cfg.jpeg_quality = std::stoi(ini["settings"]["jpeg_quality"]);
        }
        if (ini.has("captions")) {
            if (ini["captions"].has("enable_captions")) cfg.enable_captions = (ini["captions"]["enable_captions"] == "true");
            if (ini["captions"].has("csv_file_name")) cfg.csv_file_name = ini["captions"]["csv_file_name"];
            if (ini["captions"].has("caption_font_size")) cfg.caption_font_size = std::stoi(ini["captions"]["caption_font_size"]);
            if (ini["captions"].has("caption_text_padding_mm")) cfg.caption_text_padding_mm = std::stoi(ini["captions"]["caption_text_padding_mm"]);
        }
        if (ini.has("features")) {
            if (ini["features"].has("enable_watermark")) cfg.enable_watermark = (ini["features"]["enable_watermark"] == "true");
            if (ini["features"].has("watermark_text")) cfg.watermark_text = ini["features"]["watermark_text"];
            if (ini["features"].has("watermark_opacity")) cfg.watermark_opacity = std::stoi(ini["features"]["watermark_opacity"]);
            if (ini["features"].has("enable_auto_rotate")) cfg.enable_auto_rotate = (ini["features"]["enable_auto_rotate"] == "true");
        }
    }

    // Selalu pastikan nilai default (atau yang baru) tertulis jika key tidak ada
    ini["settings"]["header_text"] = cfg.header_text;
    ini["settings"]["header_font"] = cfg.header_font;
    ini["settings"]["header_size"] = std::to_string(cfg.header_size);
    ini["settings"]["header_margin_mm"] = std::to_string(cfg.header_margin_mm);
    
    // Perbaiki formatting float untuk border
    std::ostringstream ss;
    ss << cfg.border_thickness;
    ini["settings"]["border_thickness"] = ss.str();
    
    ini["settings"]["margin_mm"] = std::to_string(cfg.margin_mm);
    ini["settings"]["spacing_mm"] = std::to_string(cfg.spacing_mm);
    ini["settings"]["jpeg_quality"] = std::to_string(cfg.jpeg_quality);
    
    ini["captions"]["enable_captions"] = cfg.enable_captions ? "true" : "false";
    ini["captions"]["csv_file_name"] = cfg.csv_file_name;
    ini["captions"]["caption_font_size"] = std::to_string(cfg.caption_font_size);
    ini["captions"]["caption_text_padding_mm"] = std::to_string(cfg.caption_text_padding_mm);

    ini["features"]["enable_watermark"] = cfg.enable_watermark ? "true" : "false";
    ini["features"]["watermark_text"] = cfg.watermark_text;
    ini["features"]["watermark_opacity"] = std::to_string(cfg.watermark_opacity);
    ini["features"]["enable_auto_rotate"] = cfg.enable_auto_rotate ? "true" : "false";
    
    // Generate ulang file config untuk memasukkan fitur-fitur baru ke config.ini yang sudah ada
    file.generate(ini);
    
    if (!file_exists) {
        std::cout << "File konfigurasi 'config.ini' dibuat di '" << working_dir << "'\n";
    }

    return cfg;
}

std::vector<std::string> collect_images(const std::string& folder) {
    std::vector<std::string> files;
    std::vector<std::string> exts = {".jpg", ".jpeg", ".png", ".bmp", ".gif", ".tiff", ".webp"};
    
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (std::find(exts.begin(), exts.end(), ext) != exts.end()) {
                files.push_back(entry.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

MemBuf process_image_mem(const unsigned char* mem_data, int mem_size, int max_w, int max_h, const AppConfig& cfg) {
    MemBuf result = {nullptr, 0};
    
    int orientation = 1;
    if (cfg.enable_auto_rotate) {
        easyexif::EXIFInfo info;
        if (info.parseFrom(mem_data, mem_size) == PARSE_EXIF_SUCCESS) {
            orientation = info.Orientation;
        }
    }

    int width, height, channels;
    unsigned char *data = stbi_load_from_memory(mem_data, mem_size, &width, &height, &channels, 3); // force RGB
    if (!data) return result;

    if (cfg.enable_auto_rotate && orientation > 1) {
        unsigned char* rotated = nullptr;
        int new_w = width, new_h = height;
        if (orientation == 6 || orientation == 8) {
            new_w = height; new_h = width;
        }
        rotated = (unsigned char*)malloc(new_w * new_h * 3);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int src_idx = (y * width + x) * 3;
                int dst_idx = 0;
                if (orientation == 8) dst_idx = ((width - 1 - x) * new_w + y) * 3;
                else if (orientation == 3) dst_idx = ((height - 1 - y) * width + (width - 1 - x)) * 3;
                else if (orientation == 6) dst_idx = (x * new_w + (height - 1 - y)) * 3;
                else dst_idx = src_idx;
                rotated[dst_idx] = data[src_idx];
                rotated[dst_idx+1] = data[src_idx+1];
                rotated[dst_idx+2] = data[src_idx+2];
            }
        }
        free(data);
        data = rotated;
        width = new_w;
        height = new_h;
    }

    int out_w = width, out_h = height;
    if (width > max_w || height > max_h) {
        float ratio_w = (float)max_w / width;
        float ratio_h = (float)max_h / height;
        float ratio = std::min(ratio_w, ratio_h);
        out_w = (int)(width * ratio);
        out_h = (int)(height * ratio);
    }
    
    unsigned char* resized = (unsigned char*)malloc(out_w * out_h * 3);
    stbir_resize_uint8_linear(data, width, height, 0, resized, out_w, out_h, 0, (stbir_pixel_layout)3);
    stbi_write_jpg_to_func(stbi_write_mem, &result, out_w, out_h, 3, resized, cfg.jpeg_quality);
    
    free(data);
    free(resized);
    return result;
}

MemBuf process_image(const std::string& path, int max_w, int max_h, const AppConfig& cfg) {
    MemBuf result;
    
    // Check EXIF if needed
    int orientation = 1;
    if (cfg.enable_auto_rotate) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(size);
            if (file.read((char*)buffer.data(), size)) {
                easyexif::EXIFInfo info;
                if (info.parseFrom(buffer.data(), size) == PARSE_EXIF_SUCCESS) {
                    orientation = info.Orientation;
                }
            }
        }
    }

    int width, height, channels;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 3); // force RGB
    if (!data) return result;

    // Handle EXIF orientation
    if (cfg.enable_auto_rotate && orientation > 1) {
        unsigned char* rotated = nullptr;
        int new_w = width, new_h = height;
        if (orientation == 6 || orientation == 8) {
            new_w = height; new_h = width;
        }
        rotated = (unsigned char*)malloc(new_w * new_h * 3);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int src_idx = (y * width + x) * 3;
                int dst_idx = 0;
                
                if (orientation == 8) { // Rotate 270 CW (or 90 CCW)
                    dst_idx = ((width - 1 - x) * new_w + y) * 3;
                } else if (orientation == 3) { // Rotate 180
                    dst_idx = ((height - 1 - y) * width + (width - 1 - x)) * 3;
                } else if (orientation == 6) { // Rotate 90 CW
                    dst_idx = (x * new_w + (height - 1 - y)) * 3;
                } else {
                    dst_idx = src_idx; // simple fallback
                }
                
                rotated[dst_idx] = data[src_idx];
                rotated[dst_idx+1] = data[src_idx+1];
                rotated[dst_idx+2] = data[src_idx+2];
            }
        }
        free(data);
        data = rotated;
        width = new_w;
        height = new_h;
    }

    // Resize
    int out_w = width, out_h = height;
    if (width > max_w || height > max_h) {
        float ratio_w = (float)max_w / width;
        float ratio_h = (float)max_h / height;
        float ratio = std::min(ratio_w, ratio_h);
        out_w = (int)(width * ratio);
        out_h = (int)(height * ratio);
    }
    
    unsigned char* resized = (unsigned char*)malloc(out_w * out_h * 3);
    stbir_resize_uint8_linear(data, width, height, 0, resized, out_w, out_h, 0, (stbir_pixel_layout)3); // 3 = RGB
    
    stbi_write_jpg_to_func(stbi_write_mem, &result, out_w, out_h, 3, resized, cfg.jpeg_quality);
    
    free(data);
    free(resized);
    return result;
}

void make_pdf(const std::vector<std::string>& images, int cols, int rows, const std::string& out_pdf, const AppConfig& cfg, const std::map<std::string, std::string>& captions) {
    if (images.empty()) return;

    pdf_info info = {
        "IdinPDF C++",
        "IdinPDF Generator",
        "Dokumentasi",
        "IdinPDF",
        "", ""
    };
    pdf_doc *pdf = pdf_create(PDF_A4_WIDTH, PDF_A4_HEIGHT, &info); // We'll override page size

    if (cfg.ktp_mode) {
        // Mode KTP
        MemBuf dpn_data = {nullptr, 0};

#ifdef _WIN32
        HMODULE hDll = LoadLibraryA("dpnktp.dll");
        if (hDll) {
            HRSRC hRes = FindResourceA(hDll, "IDR_DPNKTP", (LPCSTR)RT_RCDATA);
            if (hRes) {
                HGLOBAL hData = LoadResource(hDll, hRes);
                DWORD dataSize = SizeofResource(hDll, hRes);
                void* raw_data = LockResource(hData);
                if (raw_data && dataSize > 0) {
                    dpn_data = process_image_mem((const unsigned char*)raw_data, dataSize, 856*4, 540*4, cfg);
                }
            }
            FreeLibrary(hDll);
        }
#endif

        // Fallback to local file if DLL fails or image not found in DLL
        if (!dpn_data.data) {
            std::string dpnktp_path;
            std::string exe_dir = get_executable_dir();
            
            if (fs::exists(exe_dir + "/dpnktp.jpg")) dpnktp_path = exe_dir + "/dpnktp.jpg";
            else if (fs::exists(exe_dir + "/dpnktp.png")) dpnktp_path = exe_dir + "/dpnktp.png";
            else if (fs::exists("dpnktp.jpg")) dpnktp_path = "dpnktp.jpg";
            else if (fs::exists("dpnktp.png")) dpnktp_path = "dpnktp.png";
            
            if (!dpnktp_path.empty()) {
                dpn_data = process_image(dpnktp_path, 856*4, 540*4, cfg); // upscale req for quality
            }
        }

        int count = 0;
        for (size_t i = 0; i < images.size(); ++i) {
            std::string current_name = fs::path(images[i]).filename().string();
            if (current_name == "dpnktp.jpg" || current_name == "dpnktp.png") continue;

            pdf_object *page = pdf_append_page(pdf);
            pdf_page_set_size(pdf, page, PAGE_WIDTH, PAGE_HEIGHT);

            float ktp_w = MM2PT(85.6f);
            float ktp_h = MM2PT(54.0f);
            float x = (PAGE_WIDTH - ktp_w) / 2.0f;
            
            float y_top_img = PAGE_HEIGHT - MM2PT(60.0f) - ktp_h;
            float y_bottom_img = y_top_img - MM2PT(20.0f) - ktp_h;

            // Top Image
            MemBuf top_data = process_image(images[i], ktp_w * 4, ktp_h * 4, cfg);
            if (top_data.data) {
                pdf_add_image_data(pdf, page, x, y_top_img, ktp_w, ktp_h, top_data.data, top_data.size);
                free(top_data.data);
            } else {
                pdf_add_rectangle(pdf, page, x, y_top_img, ktp_w, ktp_h, cfg.border_thickness, PDF_RED);
            }
            
            // Bottom Image
            if (dpn_data.data) {
                pdf_add_image_data(pdf, page, x, y_bottom_img, ktp_w, ktp_h, dpn_data.data, dpn_data.size);
            } else {
                pdf_add_rectangle(pdf, page, x, y_bottom_img, ktp_w, ktp_h, cfg.border_thickness, PDF_RED);
                pdf_add_text(pdf, page, "dpnktp tdk ditemukan", 10.0f, x + 10.0f, y_bottom_img + ktp_h/2.0f, PDF_RED);
            }
            count++;
        }
        
        if (dpn_data.data) free(dpn_data.data);
        
        pdf_save(pdf, out_pdf.c_str());
        pdf_destroy(pdf);
        std::cout << "Selesai! Mode KTP -> " << out_pdf << " (" << count << " halaman)\n";
        return;
    }

    // --- GRID MODE ---
    float header_margin = MM2PT(cfg.header_margin_mm);
    float margin = MM2PT(cfg.margin_mm);
    float spacing = MM2PT(cfg.spacing_mm);
    float caption_padding = MM2PT(cfg.caption_text_padding_mm);
    
    int images_per_page = cols * rows;
    size_t total_pages = (images.size() + images_per_page - 1) / images_per_page;

    for (size_t p = 0; p < total_pages; ++p) {
        pdf_object *page = pdf_append_page(pdf);
        pdf_page_set_size(pdf, page, PAGE_WIDTH, PAGE_HEIGHT);

        float header_offset = MM2PT(10);
        if (!cfg.header_text.empty()) {
            std::vector<std::string> lines;
            std::stringstream ss(cfg.header_text);
            std::string line;
            while (std::getline(ss, line, '|')) {
                lines.push_back(line);
            }
            
            pdf_set_font(pdf, cfg.header_font.c_str());
            float current_y = PAGE_HEIGHT - header_margin;
            for (const auto& l : lines) {
                float text_width = 0;
                pdf_get_font_text_width(pdf, cfg.header_font.c_str(), l.c_str(), cfg.header_size, &text_width);
                float start_x = (PAGE_WIDTH - text_width) / 2.0f;
                pdf_add_text(pdf, page, l.c_str(), cfg.header_size, start_x, current_y, PDF_BLACK);
                current_y -= (cfg.header_size * 1.5f);
            }
            header_offset = header_margin + (cfg.header_size * 1.5f * lines.size()) + MM2PT(10);
        }

        float usable_w = PAGE_WIDTH - 2 * margin;
        float usable_h = PAGE_HEIGHT - header_offset - margin;
        float cell_w = (usable_w - (cols - 1) * spacing) / cols;
        float cell_h = (usable_h - (rows - 1) * spacing) / rows;

        float caption_area_height = 0;
        if (cfg.enable_captions) {
            caption_area_height = (cfg.caption_font_size * 2.5f) + caption_padding;
        }

        float image_area_height = cell_h - caption_area_height;
        float x0 = margin;
        float y0 = PAGE_HEIGHT - header_offset;

        for (int idx = 0; idx < images_per_page; ++idx) {
            size_t img_idx = p * images_per_page + idx;
            if (img_idx >= images.size()) break;

            int row_idx = idx / cols;
            int col_idx = idx % cols;

            float x = x0 + col_idx * (cell_w + spacing);
            float y_cell_bottom = y0 - (row_idx + 1) * cell_h - row_idx * spacing;

            float img_y_pos = y_cell_bottom + caption_area_height;
            float img_draw_w = cell_w - MM2PT(1);
            float img_draw_h = image_area_height - MM2PT(1);
            float img_draw_x = x + (cell_w - img_draw_w) / 2.0f;
            float img_draw_y = img_y_pos + (image_area_height - img_draw_h) / 2.0f;

            // Process image
            MemBuf jpeg_data = process_image(images[img_idx], img_draw_w * 4, img_draw_h * 4, cfg); 
            
            if (jpeg_data.data) {
                // Add image
                pdf_add_image_data(pdf, page, img_draw_x, img_draw_y, img_draw_w, img_draw_h, jpeg_data.data, jpeg_data.size);
                free(jpeg_data.data);
                
                // Add watermark if enabled
                if (cfg.enable_watermark && !cfg.watermark_text.empty()) {
                    // Estimate center
                    float estimated_w = cfg.watermark_text.length() * 12.0f * 0.5f;
                    // Grey color watermark
                    uint32_t wm_color = PDF_RGB(150, 150, 150);
                    pdf_add_text(pdf, page, cfg.watermark_text.c_str(), 12.0f, img_draw_x + (img_draw_w - estimated_w)/2.0f, img_draw_y + img_draw_h/2.0f, wm_color);
                }
            } else {
                pdf_add_rectangle(pdf, page, img_draw_x, img_draw_y, img_draw_w, img_draw_h, cfg.border_thickness, PDF_RED);
                pdf_add_text(pdf, page, "Gagal Muat", 10.0f, img_draw_x + img_draw_w/2.0f - 20.0f, img_draw_y + img_draw_h/2.0f, PDF_RED);
            }

            // Draw border
            pdf_add_rectangle(pdf, page, x, y_cell_bottom, cell_w, cell_h, cfg.border_thickness, PDF_BLACK);

            // Caption
            if (cfg.enable_captions) {
                std::string basename = fs::path(images[img_idx]).filename().string();
                std::string stem = fs::path(images[img_idx]).stem().string();
                std::string caption_text = stem; // Default to filename without extension
                
                if (captions.find(basename) != captions.end()) {
                    caption_text = captions.at(basename);
                }
                
                pdf_set_font(pdf, "Helvetica"); 
                
                float text_width = 0;
                pdf_get_font_text_width(pdf, "Helvetica", caption_text.c_str(), cfg.caption_font_size, &text_width);
                float text_x = x + (cell_w - text_width) / 2.0f;
                
                pdf_add_text(pdf, page, caption_text.c_str(), cfg.caption_font_size, text_x, y_cell_bottom + caption_padding / 2.0f + 2.0f, PDF_BLACK);
            }
        }
    }

    pdf_save(pdf, out_pdf.c_str());
    pdf_destroy(pdf);
    
    std::cout << "Selesai! " << images.size() << " foto -> " << out_pdf << " (" << total_pages << " halaman)\n";
}
