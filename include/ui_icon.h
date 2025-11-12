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
 * Register the custom icon widget with LVGL's XML system.
 *
 * This enables the <icon> XML component to create instances of the custom
 * ui_icon widget, which extends lv_image with semantic property handling:
 *
 * Properties:
 *   - src: Material icon name (e.g., "mat_home", "mat_print")
 *   - size: Semantic size string - "xs", "sm", "md", "lg", "xl"
 *   - variant: Color variant - "primary", "secondary", "accent", "disabled", "none"
 *   - color: Custom color override (e.g., "0xFF0000", "#FF0000")
 *
 * Size mapping:
 *   xs: 16x16 (scale 64)
 *   sm: 24x24 (scale 96)
 *   md: 32x32 (scale 128)
 *   lg: 48x48 (scale 192)
 *   xl: 64x64 (scale 256)
 *
 * Variant mapping (reads from globals.xml theme constants):
 *   primary:   Recolored with #text_primary (100% opacity)
 *   secondary: Recolored with #text_secondary (100% opacity)
 *   accent:    Recolored with #primary_color (100% opacity)
 *   disabled:  Recolored with #text_secondary (50% opacity)
 *   none:      No recoloring (0% opacity)
 *
 * Call once at application startup, BEFORE registering XML components.
 *
 * Example initialization order:
 *   material_icons_register();
 *   ui_icon_register_widget();  // <-- This must come before icon.xml registration
 *   lv_xml_register_component_from_file("A:ui_xml/icon.xml");
 */
void ui_icon_register_widget();

/**
 * Change the icon source at runtime.
 *
 * @param icon          Icon widget created by ui_icon_register_widget()
 * @param icon_name     Material icon name (e.g., "mat_home")
 */
void ui_icon_set_source(lv_obj_t* icon, const char* icon_name);

/**
 * Change the icon size at runtime.
 *
 * @param icon          Icon widget
 * @param size_str      Size string: "xs", "sm", "md", "lg", or "xl"
 */
void ui_icon_set_size(lv_obj_t* icon, const char* size_str);

/**
 * Change the icon color variant at runtime.
 *
 * @param icon          Icon widget
 * @param variant_str   Variant string: "primary", "secondary", "accent", "disabled", or "none"
 */
void ui_icon_set_variant(lv_obj_t* icon, const char* variant_str);

/**
 * Set custom color for icon at runtime.
 *
 * @param icon      Icon widget
 * @param color     LVGL color value
 * @param opa       Opacity (0-255, use LV_OPA_COVER for full recoloring)
 */
void ui_icon_set_color(lv_obj_t* icon, lv_color_t color, lv_opa_t opa);
