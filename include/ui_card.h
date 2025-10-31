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

#ifndef UI_CARD_H
#define UI_CARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * Initialize card theming system
 * Reads card_bg_light and card_bg_dark colors from globals.xml
 * Must be called AFTER theme is initialized and BEFORE ui_card_register()
 */
void ui_card_init(bool use_dark_mode);

/**
 * Register the ui_card component with the LVGL XML system
 * Must be called before any XML files using <ui_card> are registered
 */
void ui_card_register(void);

#ifdef __cplusplus
}
#endif

#endif // UI_CARD_H
