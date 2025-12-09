// SPDX-License-Identifier: GPL-3.0-or-later

#include "splash_screen.h"

#include "ui_theme.h"

#include "helix_timing.h"

#include <spdlog/spdlog.h>

#include <lvgl.h>

namespace helix {

void show_splash_screen(int screen_width, int screen_height) {
    spdlog::debug("Showing splash screen");

    // Get the active screen
    lv_obj_t* screen = lv_screen_active();

    // Apply theme background color (app_bg_color runtime constant set by ui_theme_init)
    ui_theme_apply_bg_color(screen, "app_bg_color", LV_PART_MAIN);

    // Disable scrollbars on screen
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Create centered container for logo (disable scrolling)
    lv_obj_t* container = lv_obj_create(screen);
    lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);         // Disable scrollbars
    lv_obj_set_style_opa(container, LV_OPA_TRANSP, LV_PART_MAIN); // Start invisible for fade-in
    lv_obj_center(container);

    // Create image widget for logo
    lv_obj_t* logo = lv_image_create(container);
    const char* logo_path = "A:assets/images/helixscreen-logo.png";
    lv_image_set_src(logo, logo_path);

    // Get actual image dimensions
    lv_image_header_t header;
    lv_result_t res = lv_image_decoder_get_info(logo_path, &header);

    if (res == LV_RESULT_OK) {
        // Scale logo to fill more of the screen (60% of screen width)
        lv_coord_t target_size = (screen_width * 3) / 5; // 60% of screen width
        if (screen_height < 500) {                       // Tiny screen
            target_size = screen_width / 2;              // 50% on tiny screens
        }

        // Calculate scale: (target_size * 256) / actual_width
        // LVGL uses 1/256 scale units (256 = 100%, 128 = 50%, etc.)
        uint32_t width = header.w;  // Copy bit-field to local var for logging
        uint32_t height = header.h; // Copy bit-field to local var for logging
        uint32_t scale = (static_cast<uint32_t>(target_size) * 256U) / width;
        lv_image_set_scale(logo, static_cast<uint16_t>(scale));

        spdlog::debug("Logo: {}x{} scaled to {} (scale factor: {})", width, height, target_size,
                      scale);
    } else {
        spdlog::warn("Could not get logo dimensions, using default scale");
        lv_image_set_scale(logo, 128); // 50% scale as fallback
    }

    // Create fade-in animation (0.5 seconds)
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, container);
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
    uint32_t splash_start = helix_get_ticks();
    uint32_t splash_duration = 2000; // 2 seconds total

    while (helix_get_ticks() - splash_start < splash_duration) {
        lv_timer_handler(); // Process animations and rendering
        helix_delay(5);
    }

    // Clean up splash screen
    lv_obj_delete(container);

    spdlog::debug("Splash screen complete");
}

} // namespace helix
