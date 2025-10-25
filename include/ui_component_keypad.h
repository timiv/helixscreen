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

#include "lvgl/lvgl.h"

/**
 * Callback function signature for keypad confirmation
 * @param value The confirmed numeric value (clamped to min/max)
 * @param user_data User data pointer passed to ui_keypad_show()
 */
typedef void (*ui_keypad_callback_t)(float value, void* user_data);

/**
 * Configuration for numeric keypad
 */
struct ui_keypad_config_t {
	float initial_value;      // Initial value to display
	float min_value;          // Minimum allowed value
	float max_value;          // Maximum allowed value
	const char* title_label;  // Title label (e.g., "Nozzle Temp", "Heat Bed Temp")
	const char* unit_label;   // Unit label (e.g., "°C", "mm")
	bool allow_decimal;       // Enable decimal point button
	bool allow_negative;      // Enable negative sign button
	ui_keypad_callback_t callback;  // Called on OK confirmation
	void* user_data;          // User data passed to callback
};

/**
 * Initialize the keypad modal component
 * Creates the modal widget and stores reference
 * Call once after component registration
 * @param parent Parent widget (usually screen or panel root)
 */
void ui_keypad_init(lv_obj_t* parent);

/**
 * Show the numeric keypad modal
 * @param config Configuration and callback
 */
void ui_keypad_show(const ui_keypad_config_t* config);

/**
 * Hide the numeric keypad modal (cancel)
 * Does NOT invoke callback
 */
void ui_keypad_hide();

/**
 * Check if keypad is currently visible
 * @return true if visible, false if hidden
 */
bool ui_keypad_is_visible();
