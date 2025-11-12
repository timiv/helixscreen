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
 * @brief Initialize subjects for fan select screen
 *
 * Creates and registers reactive subjects:
 * - hotend_fan_selected (int) - Selected hotend fan index in dropdown
 * - part_fan_selected (int) - Selected part cooling fan index in dropdown
 */
void ui_wizard_fan_select_init_subjects();

/**
 * @brief Register event callbacks for fan select screen
 *
 * Registers callbacks:
 * - on_hotend_fan_changed - When hotend fan dropdown selection changes
 * - on_part_fan_changed - When part fan dropdown selection changes
 */
void ui_wizard_fan_select_register_callbacks();

/**
 * @brief Create fan select screen UI
 *
 * Creates the fan selection form from wizard_fan_select.xml
 *
 * @param parent Parent container (wizard_content)
 * @return Root object of the screen, or nullptr on failure
 */
lv_obj_t* ui_wizard_fan_select_create(lv_obj_t* parent);

/**
 * @brief Cleanup fan select screen resources
 *
 * Clears any temporary state and releases resources
 */
void ui_wizard_fan_select_cleanup();

/**
 * @brief Check if fan selection is complete
 *
 * @return true (always validated for baseline implementation)
 */
bool ui_wizard_fan_select_is_validated();