/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui_icon_loader.h"
#include "lvgl/src/drivers/sdl/lv_sdl_window.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <vector>

bool ui_set_window_icon(lv_display_t* disp) {
    spdlog::info("[Icon] Setting window icon...");

    if (!disp) {
        spdlog::error("[Icon] Cannot set icon: display is NULL");
        return false;
    }

    // Create a temporary canvas to load and convert the icon
    // This uses only public LVGL APIs
    const int32_t width = 64;
    const int32_t height = 64;

    // Allocate buffer for ARGB8888 format (4 bytes per pixel)
    std::vector<uint32_t> icon_data(width * height);

    // Create a draw buffer for the icon
    lv_draw_buf_t* draw_buf = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_ARGB8888, 0);
    if (!draw_buf) {
        spdlog::warn("[Icon] Failed to create draw buffer, window will use default icon");
        return false;
    }

    // Create a temporary canvas to render the icon
    lv_obj_t* canvas = lv_canvas_create(lv_screen_active());
    if (!canvas) {
        spdlog::warn("[Icon] Failed to create canvas, window will use default icon");
        lv_draw_buf_destroy(draw_buf);
        return false;
    }

    lv_canvas_set_draw_buf(canvas, draw_buf);

    // Load and draw the icon onto the canvas
    lv_obj_t* img = lv_image_create(canvas);
    lv_image_set_src(img, "A:assets/images/helix-icon-64.png");
    lv_obj_set_pos(img, 0, 0);

    // Force multiple render cycles to ensure canvas is drawn
    for (int i = 0; i < 3; i++) {
        lv_refr_now(disp);
    }

    // Copy pixel data from canvas buffer to icon_data
    const uint32_t* buf_data = (const uint32_t*)lv_canvas_get_buf(canvas);
    if (buf_data) {
        // Check if we have any non-zero pixels (debug)
        int non_zero = 0;
        for (int32_t i = 0; i < width * height; i++) {
            icon_data[i] = buf_data[i];
            if (buf_data[i] != 0) non_zero++;
        }

        spdlog::debug("[Icon] Canvas buffer has {} non-zero pixels out of {}", non_zero, width * height);

        // Set the window icon using LVGL's SDL wrapper
        lv_sdl_window_set_icon(disp, icon_data.data(), width, height);

        spdlog::debug("[Icon] Window icon set ({}x{}, {} visible pixels)", width, height, non_zero);
    } else {
        spdlog::warn("[Icon] Failed to get buffer data");
    }

    // Clean up temporary objects
    lv_obj_delete(canvas);
    lv_draw_buf_destroy(draw_buf);

    return buf_data != nullptr;
}
