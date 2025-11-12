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

#ifndef UI_SWITCH_H
#define UI_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * Register responsive constants for switch sizing based on screen dimensions
 * Must be called AFTER globals.xml is registered and BEFORE test_panel.xml
 */
void ui_switch_register_responsive_constants(void);

/**
 * Register the ui_switch component with the LVGL XML system
 * Must be called before any XML files using <ui_switch> are registered
 */
void ui_switch_register(void);

#ifdef __cplusplus
}
#endif

#endif // UI_SWITCH_H
