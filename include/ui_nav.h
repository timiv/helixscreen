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

// Navigation panel IDs (order matches app_layout.xml panel children)
typedef enum {
    UI_PANEL_HOME,          // Panel 0: Home
    UI_PANEL_PRINT_SELECT,  // Panel 1: Print Select (beneath Home)
    UI_PANEL_CONTROLS,      // Panel 2: Controls
    UI_PANEL_FILAMENT,      // Panel 3: Filament
    UI_PANEL_SETTINGS,      // Panel 4: Settings
    UI_PANEL_ADVANCED,      // Panel 5: Advanced
    UI_PANEL_COUNT
} ui_panel_id_t;

// Initialize navigation system with reactive subjects
// MUST be called BEFORE creating navigation bar XML
void ui_nav_init();

// Wire up event handlers to an existing navbar widget created from XML
// Call this after creating navigation_bar component from XML
void ui_nav_wire_events(lv_obj_t* navbar);

// Set active panel (triggers reactive icon color updates)
void ui_nav_set_active(ui_panel_id_t panel_id);

// Get current active panel
ui_panel_id_t ui_nav_get_active();

// Set panel widgets for show/hide management
// panels array should have UI_PANEL_COUNT elements (can have NULLs for not-yet-created panels)
void ui_nav_set_panels(lv_obj_t** panels);

// Set app_layout widget reference (to prevent hiding it when hiding overlays)
void ui_nav_set_app_layout(lv_obj_t* app_layout);

// Navigation history stack for overlay panels (motion, temp, extrusion, etc.)
// Push an overlay panel onto the history stack and show it
// When the overlay's back button is pressed, ui_nav_go_back() will restore the previous panel
void ui_nav_push_overlay(lv_obj_t* overlay_panel);

// Go back to the previous panel in the navigation history
// Hides the current overlay and shows the previous panel
// Returns true if navigation occurred, false if history is empty
bool ui_nav_go_back();

