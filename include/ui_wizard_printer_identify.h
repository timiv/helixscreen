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
 * @brief Initialize subjects for printer identification screen
 *
 * Creates and registers reactive subjects:
 * - printer_name (string) - User-entered printer name
 * - printer_type_selected (int) - Selected index in roller
 * - printer_detection_status (string) - Auto-detection status message
 */
void ui_wizard_printer_identify_init_subjects();

/**
 * @brief Register event callbacks for printer identification screen
 *
 * Registers callbacks:
 * - on_printer_name_changed - When name textarea changes
 * - on_printer_type_changed - When type roller selection changes
 */
void ui_wizard_printer_identify_register_callbacks();

/**
 * @brief Create printer identification screen UI
 *
 * Creates the printer identification form from wizard_printer_identify.xml
 *
 * @param parent Parent container (wizard_content)
 * @return Root object of the screen, or nullptr on failure
 */
lv_obj_t* ui_wizard_printer_identify_create(lv_obj_t* parent);

/**
 * @brief Cleanup printer identification screen resources
 *
 * Clears any temporary state and releases resources
 */
void ui_wizard_printer_identify_cleanup();

/**
 * @brief Check if printer identification is complete
 *
 * @return true if printer name is entered and type is selected
 */
bool ui_wizard_printer_identify_is_validated();