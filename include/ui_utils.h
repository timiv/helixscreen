/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of GuppyScreen.
 *
 * GuppyScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GuppyScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GuppyScreen. If not, see <https://www.gnu.org/licenses/>.
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
 * Show the right button in a header_bar component
 * This is useful when the button needs to be made visible at runtime
 * after component creation (e.g., for dynamically added actions)
 * @param header_bar_widget Pointer to the header_bar component root
 * @return true if button was found and shown, false otherwise
 */
bool ui_header_bar_show_right_button(lv_obj_t* header_bar_widget);

/**
 * Hide the right button in a header_bar component
 * @param header_bar_widget Pointer to the header_bar component root
 * @return true if button was found and hidden, false otherwise
 */
bool ui_header_bar_hide_right_button(lv_obj_t* header_bar_widget);

/**
 * Set the text of the right button in a header_bar component
 * Note: This does NOT automatically show the button - call ui_header_bar_show_right_button() separately
 * @param header_bar_widget Pointer to the header_bar component root
 * @param text New button text
 * @return true if button was found and updated, false otherwise
 */
bool ui_header_bar_set_right_button_text(lv_obj_t* header_bar_widget, const char* text);

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
