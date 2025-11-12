// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

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
 * @brief Initialize subjects for bed select screen
 *
 * Creates and registers reactive subjects:
 * - bed_heater_selected (int) - Selected heater index in dropdown
 * - bed_sensor_selected (int) - Selected sensor index in dropdown
 */
void ui_wizard_bed_select_init_subjects();

/**
 * @brief Register event callbacks for bed select screen
 *
 * Registers callbacks:
 * - on_bed_heater_changed - When heater dropdown selection changes
 * - on_bed_sensor_changed - When sensor dropdown selection changes
 */
void ui_wizard_bed_select_register_callbacks();

/**
 * @brief Create bed select screen UI
 *
 * Creates the bed selection form from wizard_bed_select.xml
 *
 * @param parent Parent container (wizard_content)
 * @return Root object of the screen, or nullptr on failure
 */
lv_obj_t* ui_wizard_bed_select_create(lv_obj_t* parent);

/**
 * @brief Cleanup bed select screen resources
 *
 * Clears any temporary state and releases resources
 */
void ui_wizard_bed_select_cleanup();

/**
 * @brief Check if bed selection is complete
 *
 * @return true (always validated for baseline implementation)
 */
bool ui_wizard_bed_select_is_validated();