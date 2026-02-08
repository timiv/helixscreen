// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later
/// @file ui_z_offset_indicator.h
/// @brief Visual nozzle-over-bed cross-section indicator for z-offset adjustment

#pragma once

#include "lvgl/lvgl.h"

/// @brief Set the z-offset value to display (animated transition)
/// @param obj The z_offset_indicator widget
/// @param microns Z-offset in microns (-2000 to +2000). Negative = closer to bed.
void ui_z_offset_indicator_set_value(lv_obj_t* obj, int microns);

/// @brief Flash a direction arrow (fades out over 400ms)
/// @param obj The z_offset_indicator widget
/// @param direction +1 for farther/up arrow, -1 for closer/down arrow
void ui_z_offset_indicator_flash_direction(lv_obj_t* obj, int direction);

/// @brief Register the <z_offset_indicator> widget with the LVGL XML system
/// Must be called before any XML files using <z_offset_indicator> are parsed
void ui_z_offset_indicator_register(void);
