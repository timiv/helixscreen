// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "splash_screen.h"

#include "ui_utils.h"

#include "helix_timing.h"
#include "helix_version.h"
#include "prerendered_images.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <lvgl.h>

namespace helix {

namespace {

/**
 * @brief Try to find a prerendered image at the deployed path or the build path
 *
 * On embedded devices, images are at assets/images/prerendered/ (deployed).
 * On desktop dev, images are at build/assets/images/prerendered/.
 * Returns LVGL path with A: prefix, or empty string if not found.
 */
std::string find_prerendered(const std::string& relative_path) {
    // Try deployed path first (embedded)
    if (std::filesystem::exists(relative_path)) {
        return "A:" + relative_path;
    }
    // Try build path (desktop development)
    std::string build_path = "build/" + relative_path;
    if (std::filesystem::exists(build_path)) {
        return "A:" + build_path;
    }
    return "";
}

} // namespace

void show_splash_screen(int screen_width, int screen_height) {
    spdlog::debug("[Splash Screen] Showing splash screen ({}x{})", screen_width, screen_height);

    // Get the active screen
    lv_obj_t* screen = lv_screen_active();

    // Apply theme background color (screen_bg runtime constant set by theme_manager_init)
    theme_manager_apply_bg_color(screen, "screen_bg", LV_PART_MAIN);

    // Disable scrollbars on screen
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Try full-screen 3D splash first (theme-aware dark/light variant)
    bool dark_mode = theme_manager_is_dark_mode();
    const char* size_name = get_splash_3d_size_name(screen_width, screen_height);
    const char* mode_name = dark_mode ? "dark" : "light";

    std::string splash_3d_path = find_prerendered(
        std::string("assets/images/prerendered/splash-3d-") + mode_name + "-" + size_name + ".bin");

    // Fallback: try base "tiny" if tiny_alt not found
    if (splash_3d_path.empty() && std::string(size_name) == "tiny_alt") {
        size_name = "tiny";
        splash_3d_path = find_prerendered(std::string("assets/images/prerendered/splash-3d-") +
                                          mode_name + "-tiny.bin");
    }

    // The widget we'll animate and clean up
    lv_obj_t* splash_widget = nullptr;

    if (!splash_3d_path.empty()) {
        // Full-screen 3D splash: image IS the entire screen, no container needed
        lv_obj_t* img = lv_image_create(screen);
        lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(img, 0, LV_PART_MAIN);
        lv_image_set_src(img, splash_3d_path.c_str());
        lv_obj_center(img);
        lv_obj_set_style_opa(img, LV_OPA_TRANSP, LV_PART_MAIN); // Start invisible for fade-in
        splash_widget = img;
        spdlog::info("[Splash Screen] Using 3D splash ({}, {})", mode_name, size_name);
    } else {
        // Fallback: centered logo in container
        lv_obj_t* container = lv_obj_create(screen);
        lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
        lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_center(container);

        lv_obj_t* logo = lv_image_create(container);

        // Try pre-rendered centered logo
        std::string prerendered_path = find_prerendered(
            std::string("assets/images/prerendered/splash-logo-") + size_name + ".bin");
        if (!prerendered_path.empty()) {
            lv_image_set_src(logo, prerendered_path.c_str());
            spdlog::info("[Splash Screen] Using pre-rendered splash (instant display)");
        } else {
            // PNG fallback with runtime scaling
            const char* png_path = "A:assets/images/helixscreen-logo.png";
            lv_image_set_src(logo, png_path);

            lv_image_header_t header;
            lv_result_t res = lv_image_decoder_get_info(png_path, &header);

            if (res == LV_RESULT_OK) {
                lv_coord_t target_size = (screen_width * 3) / 5; // 60%
                if (screen_height < 500) {
                    target_size = screen_width / 2; // 50% on tiny screens
                }

                uint32_t width = header.w;
                uint32_t height = header.h;
                uint32_t scale = (static_cast<uint32_t>(target_size) * 256U) / width;
                lv_image_set_scale(logo, static_cast<uint16_t>(scale));

                spdlog::info("[Splash Screen] PNG fallback: {}x{} scaled to {} (scale factor: {})",
                             width, height, target_size, scale);
            } else {
                spdlog::warn("[Splash Screen] Could not get logo dimensions, using default scale");
                lv_image_set_scale(logo, 128);
            }
        }

        splash_widget = container;
    }

    // Version number in lower-right corner (subtle, theme-aware)
    lv_obj_t* version_label = lv_label_create(screen);
    lv_label_set_text(version_label, "v" HELIX_VERSION);
    lv_obj_set_style_text_color(
        version_label, dark_mode ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_opa(version_label, LV_OPA_40, LV_PART_MAIN);
    lv_obj_align(version_label, LV_ALIGN_BOTTOM_RIGHT, -8, -6);

    // Create fade-in animation (0.5 seconds)
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, splash_widget);
    lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&anim, 500); // 500ms = 0.5 seconds
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&anim, [](void* obj, int32_t value) {
        lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(value),
                             LV_PART_MAIN);
    });
    lv_anim_start(&anim);

    // Run LVGL timer to process fade-in animation and keep splash visible
    // Total display time: 2 seconds (including 0.5s fade-in)
    uint32_t splash_start = helix::timing::get_ticks();
    uint32_t splash_duration = 2000; // 2 seconds total

    while (helix::timing::get_ticks() - splash_start < splash_duration) {
        lv_timer_handler(); // Process animations and rendering
        helix::timing::delay(5);
    }

    // Clean up splash screen (guard against early shutdown)
    lv_obj_safe_delete(version_label);
    lv_obj_safe_delete(splash_widget);

    spdlog::debug("[Splash Screen] complete");
}

} // namespace helix
