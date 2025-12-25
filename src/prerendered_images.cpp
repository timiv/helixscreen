// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "prerendered_images.h"

#include <spdlog/spdlog.h>

#include <filesystem>

namespace helix {

bool prerendered_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

const char* get_splash_size_name(int screen_width) {
    if (screen_width < 600) {
        return "tiny"; // 480x320 class
    } else if (screen_width < 900) {
        return "small"; // 800x480 class (AD5M)
    } else if (screen_width < 1100) {
        return "medium"; // 1024x600 class
    } else {
        return "large"; // 1280x720+ class
    }
}

std::string get_prerendered_splash_path(int screen_width) {
    const char* size_name = get_splash_size_name(screen_width);

    // Path relative to install directory
    std::string path = "assets/images/prerendered/splash-logo-";
    path += size_name;
    path += ".bin";

    if (prerendered_exists(path)) {
        spdlog::debug("[Prerendered] Using splash: {}", path);
        return "A:" + path;
    }

    spdlog::debug("[Prerendered] Splash fallback to PNG ({}px screen)", screen_width);
    return "A:assets/images/helixscreen-logo.png";
}

int get_printer_image_size(int screen_width) {
    // 300px for medium-large displays (800x480+)
    // 150px for small displays (480x320)
    return (screen_width >= 600) ? 300 : 150;
}

std::string get_prerendered_printer_path(const std::string& printer_name, int screen_width) {
    int size = get_printer_image_size(screen_width);

    // Path relative to install directory
    std::string path = "assets/images/printers/prerendered/";
    path += printer_name;
    path += "-";
    path += std::to_string(size);
    path += ".bin";

    if (prerendered_exists(path)) {
        spdlog::debug("[Prerendered] Using printer image: {}", path);
        return "A:" + path;
    }

    spdlog::debug("[Prerendered] Printer {} fallback to PNG (no {}px)", printer_name, size);
    return "A:assets/images/printers/" + printer_name + ".png";
}

std::string get_prerendered_placeholder_path(const std::string& placeholder_name) {
    // Path relative to install directory
    std::string bin_path = "assets/images/prerendered/";
    bin_path += placeholder_name;
    bin_path += ".bin";

    if (prerendered_exists(bin_path)) {
        spdlog::debug("[Prerendered] Using placeholder: {}", bin_path);
        return "A:" + bin_path;
    }

    // Fallback to original PNG
    std::string png_path = "assets/images/" + placeholder_name + ".png";
    spdlog::debug("[Prerendered] Placeholder fallback to PNG: {}", png_path);
    return "A:" + png_path;
}

} // namespace helix
