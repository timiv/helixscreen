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

#pragma once

#include <string>
#include <ctime>
#include "lvgl/lvgl.h"

/**
 * Format print time from minutes to human-readable string
 * Examples: "5m", "1h30m", "8h"
 * @param minutes Total print time in minutes
 * @return Formatted string
 */
std::string format_print_time(int minutes);

/**
 * Format filament weight from grams to human-readable string
 * Examples: "2.5g", "45g", "120g"
 * @param grams Filament weight in grams
 * @return Formatted string
 */
std::string format_filament_weight(float grams);

/**
 * Format file size from bytes to human-readable string
 * Examples: "1.2 KB", "45 MB", "1.5 GB"
 * @param bytes File size in bytes
 * @return Formatted string
 */
std::string format_file_size(size_t bytes);

/**
 * Format timestamp to date/time string
 * Examples: "Jan 15 14:30", "Dec 5 09:15"
 * @param timestamp Unix timestamp (time_t)
 * @return Formatted date/time string
 */
std::string format_modified_date(time_t timestamp);

/**
 * Get responsive padding for content areas below headers
 * Returns smaller padding on tiny/small screens for more compact layouts
 * @param screen_height Current screen height in pixels
 * @return Padding value in pixels (20px for large/medium, 10px for small, 6px for tiny)
 */
lv_coord_t ui_get_header_content_padding(lv_coord_t screen_height);

/**
 * Get responsive header height based on screen size
 * Returns smaller header on tiny/small screens for more compact layouts
 * @param screen_height Current screen height in pixels
 * @return Header height in pixels (60px for large/medium, 48px for small, 40px for tiny)
 */
lv_coord_t ui_get_responsive_header_height(lv_coord_t screen_height);

// ============================================================================
// App-level resize handling
// ============================================================================

/**
 * Callback type for resize notifications
 * Called when the display size changes (debounced to avoid excessive calls)
 */
typedef void (*ui_resize_callback_t)(void);

/**
 * Initialize the app-level resize handler
 * Must be called once during app initialization, after screen is created
 * @param screen The main screen object to monitor for size changes
 */
void ui_resize_handler_init(lv_obj_t* screen);

/**
 * Register a callback to be called when the display is resized
 * Callbacks are invoked after a brief debounce period (250ms by default)
 * @param callback Function to call on resize
 */
void ui_resize_handler_register(ui_resize_callback_t callback);

// ============================================================================
// Image Scaling Utilities
// ============================================================================

/**
 * Scale image to cover a target area (like CSS object-fit: cover)
 * Image may be cropped but will fill the entire area with no empty space
 *
 * @param image_widget The lv_image widget to scale
 * @param target_width Target width in pixels
 * @param target_height Target height in pixels
 * @return true if scaling succeeded, false if image info could not be obtained
 */
bool ui_image_scale_to_cover(lv_obj_t* image_widget, lv_coord_t target_width, lv_coord_t target_height);

/**
 * Scale image to fit within a target area (like CSS object-fit: contain)
 * Entire image will be visible, may have empty space around it
 *
 * @param image_widget The lv_image widget to scale
 * @param target_width Target width in pixels
 * @param target_height Target height in pixels
 * @param align Alignment within the target area (default: LV_IMAGE_ALIGN_CENTER)
 * @return true if scaling succeeded, false if image info could not be obtained
 */
bool ui_image_scale_to_contain(lv_obj_t* image_widget, lv_coord_t target_width, lv_coord_t target_height,
                                lv_image_align_t align = LV_IMAGE_ALIGN_CENTER);
