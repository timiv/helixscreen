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

#ifndef UI_JOG_PAD_H
#define UI_JOG_PAD_H

#include <lvgl.h>
#include "ui_panel_motion.h"  // For jog_direction_t and jog_distance_t enums

// Jog pad event callbacks
typedef void (*jog_pad_jog_cb_t)(jog_direction_t direction, float distance_mm, void* user_data);
typedef void (*jog_pad_home_cb_t)(void* user_data);

/**
 * Create a circular jog pad widget (Bambu Lab style)
 *
 * Features:
 * - Two-zone design: inner ring (small jogs), outer ring (large jogs)
 * - 8 directional zones (N, S, E, W, NE, NW, SE, SW)
 * - Center home button
 * - Theme-aware colors (reads from component scope or uses fallbacks)
 * - Visual press feedback
 *
 * @param parent Parent LVGL object
 * @return Created jog pad object (use as clickable container)
 */
lv_obj_t* ui_jog_pad_create(lv_obj_t* parent);

/**
 * Set jog callback (called when directional zone is clicked)
 *
 * @param obj Jog pad object
 * @param cb Callback function (direction, distance_mm, user_data)
 * @param user_data Optional user data passed to callback
 */
void ui_jog_pad_set_jog_callback(lv_obj_t* obj, jog_pad_jog_cb_t cb, void* user_data);

/**
 * Set home callback (called when center button is clicked)
 *
 * @param obj Jog pad object
 * @param cb Callback function (user_data)
 * @param user_data Optional user data passed to callback
 */
void ui_jog_pad_set_home_callback(lv_obj_t* obj, jog_pad_home_cb_t cb, void* user_data);

/**
 * Set current jog distance mode (affects which distance is used for zones)
 *
 * Inner zone distance:
 * - JOG_DIST_0_1MM or JOG_DIST_1MM → uses that distance
 * - JOG_DIST_10MM or JOG_DIST_100MM → defaults to 1mm
 *
 * Outer zone distance:
 * - JOG_DIST_10MM or JOG_DIST_100MM → uses that distance
 * - JOG_DIST_0_1MM or JOG_DIST_1MM → defaults to 10mm
 *
 * @param obj Jog pad object
 * @param distance Distance mode
 */
void ui_jog_pad_set_distance(lv_obj_t* obj, jog_distance_t distance);

/**
 * Get current jog distance mode
 *
 * @param obj Jog pad object
 * @return Current distance mode
 */
jog_distance_t ui_jog_pad_get_distance(lv_obj_t* obj);

/**
 * Refresh colors from theme (call when theme changes)
 *
 * @param obj Jog pad object
 */
void ui_jog_pad_refresh_colors(lv_obj_t* obj);

#endif // UI_JOG_PAD_H
