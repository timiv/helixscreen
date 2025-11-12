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

// Material Design Icons (64x64) - from original HelixScreen
// These can be scaled dynamically using lv_image_set_scale()

// Navigation & Movement
LV_IMG_DECLARE(home);
LV_IMG_DECLARE(home_printer);
LV_IMG_DECLARE(arrow_down);
LV_IMG_DECLARE(arrow_left);
LV_IMG_DECLARE(arrow_right);
LV_IMG_DECLARE(arrow_up);
LV_IMG_DECLARE(back);
LV_IMG_DECLARE(move);
LV_IMG_DECLARE(z_closer);
LV_IMG_DECLARE(z_farther);
LV_IMG_DECLARE(home_z);

// Print & Files
LV_IMG_DECLARE(print);
LV_IMG_DECLARE(list);
LV_IMG_DECLARE(grid_view);
LV_IMG_DECLARE(pause_img);
LV_IMG_DECLARE(resume);
LV_IMG_DECLARE(cancel);
LV_IMG_DECLARE(sd_img);
LV_IMG_DECLARE(refresh_img);

// Temperature & Heating
LV_IMG_DECLARE(bed);
LV_IMG_DECLARE(heater);
LV_IMG_DECLARE(cooldown_img);

// Extrusion & Filament
LV_IMG_DECLARE(extruder);
LV_IMG_DECLARE(extrude);
LV_IMG_DECLARE(extrude_img);
LV_IMG_DECLARE(filament_img);
LV_IMG_DECLARE(load_filament_img);
LV_IMG_DECLARE(unload_filament_img);
LV_IMG_DECLARE(retract_img);

// Fan & Cooling
LV_IMG_DECLARE(fan);
LV_IMG_DECLARE(fan_on);
LV_IMG_DECLARE(fan_off_img);

// Lighting
LV_IMG_DECLARE(light_img);
LV_IMG_DECLARE(light_off);

// Network & Communication
LV_IMG_DECLARE(network_img);

// WiFi & Network
LV_IMG_DECLARE(wifi);
LV_IMG_DECLARE(wifi_off);
LV_IMG_DECLARE(wifi_lock);
LV_IMG_DECLARE(wifi_check);
LV_IMG_DECLARE(wifi_alert);
LV_IMG_DECLARE(wifi_strength_1);
LV_IMG_DECLARE(wifi_strength_2);
LV_IMG_DECLARE(wifi_strength_3);
LV_IMG_DECLARE(wifi_strength_4);
LV_IMG_DECLARE(wifi_strength_1_lock);
LV_IMG_DECLARE(wifi_strength_2_lock);
LV_IMG_DECLARE(wifi_strength_3_lock);
LV_IMG_DECLARE(wifi_strength_4_lock);

// Tuning & Adjustments
LV_IMG_DECLARE(fine_tune_img);
LV_IMG_DECLARE(flow_down_img);
LV_IMG_DECLARE(flow_up_img);
LV_IMG_DECLARE(speed_down_img);
LV_IMG_DECLARE(speed_up_img);
LV_IMG_DECLARE(pa_minus_img);
LV_IMG_DECLARE(pa_plus_img);

// Calibration & Advanced
LV_IMG_DECLARE(bedmesh_img);
LV_IMG_DECLARE(belts_calibration_img);
LV_IMG_DECLARE(inputshaper_img);
LV_IMG_DECLARE(limit_img);

// System & Info
LV_IMG_DECLARE(info_img);
LV_IMG_DECLARE(sysinfo_img);
LV_IMG_DECLARE(power_devices_img);
LV_IMG_DECLARE(motor_img);
LV_IMG_DECLARE(motor_off_img);
LV_IMG_DECLARE(update_img);
LV_IMG_DECLARE(emergency);
LV_IMG_DECLARE(delete_img);

// Monitoring & Display
LV_IMG_DECLARE(chart_img);
LV_IMG_DECLARE(layers_img);
LV_IMG_DECLARE(clock_img);
LV_IMG_DECLARE(hourglass);

// Misc
LV_IMG_DECLARE(spoolman_img);

// Register all Material Design icons with LVGL XML system
void material_icons_register();
