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
 * Extrusion Control Panel
 *
 * Provides extrude/retract controls with safety checks.
 * Requires nozzle temperature >= 170°C to enable extrusion.
 */

/**
 * Initialize extrusion panel reactive subjects.
 * Must be called before creating XML components.
 */
void ui_panel_controls_extrusion_init_subjects();

/**
 * Setup event handlers for extrusion panel after XML creation.
 * @param panel - Root object of extrusion_panel (returned from lv_xml_create)
 * @param parent_screen - Parent screen object (for navigation)
 */
void ui_panel_controls_extrusion_setup(lv_obj_t* panel, lv_obj_t* parent_screen);

/**
 * Update nozzle temperature display and safety state.
 * @param current - Current nozzle temperature in °C
 * @param target - Target nozzle temperature in °C
 */
void ui_panel_controls_extrusion_set_temp(int current, int target);

/**
 * Get currently selected extrusion amount.
 * @return Extrusion amount in mm (5, 10, 25, or 50)
 */
int ui_panel_controls_extrusion_get_amount();

/**
 * Check if extrusion is allowed (nozzle hot enough).
 * @return true if nozzle >= 170°C, false otherwise
 */
bool ui_panel_controls_extrusion_is_allowed();

/**
 * Set nozzle temperature limits (for validation).
 * Call this after querying Moonraker for heater configuration.
 * @param min_temp - Minimum safe nozzle temperature in °C (default: 0)
 * @param max_temp - Maximum safe nozzle temperature in °C (default: 500)
 */
void ui_panel_controls_extrusion_set_limits(int min_temp, int max_temp);
