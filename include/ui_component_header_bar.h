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

#ifndef UI_COMPONENT_HEADER_BAR_H
#define UI_COMPONENT_HEADER_BAR_H

#include "lvgl.h"

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize header_bar component system
 * Registers global resize handler
 * Call this once during app initialization
 */
void ui_component_header_bar_init();

/**
 * Setup a header_bar instance for responsive height management
 * Call this in panel setup functions after finding the header widget
 * @param header The header_bar widget instance
 * @param screen The parent screen for measuring height
 */
void ui_component_header_bar_setup(lv_obj_t* header, lv_obj_t* screen);

// ============================================================================
// HEADER BAR API
// ============================================================================

/**
 * Show the right action button in a header_bar component
 * @param header_bar_widget Pointer to the header_bar component root
 * @return true if button was found and shown, false otherwise
 */
bool ui_header_bar_show_right_button(lv_obj_t* header_bar_widget);

/**
 * Hide the right action button in a header_bar component
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

#endif // UI_COMPONENT_HEADER_BAR_H
