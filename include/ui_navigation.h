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

#include "lvgl/lvgl.h"

// Panel indices
#define PANEL_HOME 0
#define PANEL_CONTROLS 1
#define PANEL_FILAMENT 2
#define PANEL_SETTINGS 3
#define PANEL_ADVANCED 4
#define PANEL_COUNT 5

// Initialize navigation system with active panel subject
// MUST be called BEFORE creating navigation bar XML
void ui_navigation_init();

// Create navigation bar from XML and wire up icon highlighting
// Returns the navbar widget
// NOTE: ui_navigation_init() must be called first!
lv_obj_t* ui_navigation_create(lv_obj_t* parent);

// Switch to a specific panel (triggers icon color updates)
void ui_navigation_switch_to_panel(int panel_index);

// Get current active panel index
int ui_navigation_get_active_panel();

// Set panel widgets for show/hide management
// panels array should have PANEL_COUNT elements (can have NULLs for not-yet-created panels)
void ui_navigation_set_panels(lv_obj_t** panels);
