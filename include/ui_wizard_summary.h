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
 * @brief Initialize subjects for summary screen
 *
 * Creates and registers reactive subjects:
 * - summary_printer_name (string) - Configured printer name
 * - summary_printer_type (string) - Selected printer type
 * - summary_network (string) - Network configuration summary
 * - summary_bed (string) - Bed configuration summary
 * - summary_hotend (string) - Hotend configuration summary
 * - summary_part_fan (string) - Part fan configuration summary
 */
void ui_wizard_summary_init_subjects();

/**
 * @brief Register event callbacks for summary screen
 *
 * No interactive callbacks for summary screen (read-only display)
 */
void ui_wizard_summary_register_callbacks();

/**
 * @brief Create summary screen UI
 *
 * Creates the configuration summary from wizard_summary.xml
 *
 * @param parent Parent container (wizard_content)
 * @return Root object of the screen, or nullptr on failure
 */
lv_obj_t* ui_wizard_summary_create(lv_obj_t* parent);

/**
 * @brief Cleanup summary screen resources
 *
 * Clears any temporary state and releases resources
 */
void ui_wizard_summary_cleanup();

/**
 * @brief Check if summary is validated
 *
 * @return true (always validated - no user input required)
 */
bool ui_wizard_summary_is_validated();