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
 * @brief Initialize subjects for hotend select screen
 *
 * Creates and registers reactive subjects:
 * - hotend_heater_selected (int) - Selected heater index in dropdown
 * - hotend_sensor_selected (int) - Selected sensor index in dropdown
 */
void ui_wizard_hotend_select_init_subjects();

/**
 * @brief Register event callbacks for hotend select screen
 *
 * Registers callbacks:
 * - on_hotend_heater_changed - When heater dropdown selection changes
 * - on_hotend_sensor_changed - When sensor dropdown selection changes
 */
void ui_wizard_hotend_select_register_callbacks();

/**
 * @brief Create hotend select screen UI
 *
 * Creates the hotend selection form from wizard_hotend_select.xml
 *
 * @param parent Parent container (wizard_content)
 * @return Root object of the screen, or nullptr on failure
 */
lv_obj_t* ui_wizard_hotend_select_create(lv_obj_t* parent);

/**
 * @brief Cleanup hotend select screen resources
 *
 * Clears any temporary state and releases resources
 */
void ui_wizard_hotend_select_cleanup();

/**
 * @brief Check if hotend selection is complete
 *
 * @return true (always validated for baseline implementation)
 */
bool ui_wizard_hotend_select_is_validated();