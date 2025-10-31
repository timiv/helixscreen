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

#include "ui_card.h"
#include "ui_theme.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/others/xml/lv_xml.h"
#include "lvgl/src/others/xml/lv_xml_widget.h"
#include "lvgl/src/others/xml/lv_xml_parser.h"
#include "lvgl/src/others/xml/lv_xml_style.h"
#include "lvgl/src/others/xml/parsers/lv_xml_obj_parser.h"
#include <spdlog/spdlog.h>

/**
 * Card theme colors (read from globals.xml at initialization)
 */
static lv_color_t card_bg_color_light;
static lv_color_t card_bg_color_dark;
static bool card_theme_initialized = false;
static bool use_dark_mode = false;

void ui_card_init(bool use_dark_mode_param) {
    use_dark_mode = use_dark_mode_param;

    // Read card background colors from globals.xml
    const char* light_str = lv_xml_get_const(NULL, "card_bg_light");
    const char* dark_str = lv_xml_get_const(NULL, "card_bg_dark");

    if (!light_str || !dark_str) {
        spdlog::error("[Card] Failed to read card_bg_light/card_bg_dark from globals.xml");
        // Fallback to LVGL defaults
        card_bg_color_light = lv_color_white();
        card_bg_color_dark = lv_color_hex(0x282b30);
        spdlog::warn("[Card] Using LVGL default colors as fallback");
    } else {
        card_bg_color_light = ui_theme_parse_color(light_str);
        card_bg_color_dark = ui_theme_parse_color(dark_str);
        spdlog::info("[Card] Initialized: light={}, dark={}", light_str, dark_str);
    }

    card_theme_initialized = true;
}

/**
 * XML create handler for ui_card
 * Creates an lv_obj widget when <ui_card> is encountered in XML
 */
static void *ui_card_xml_create(lv_xml_parser_state_t *state, const char **attrs)
{
    LV_UNUSED(attrs);

    void *parent = lv_xml_state_get_parent(state);
    lv_obj_t *obj = lv_obj_create((lv_obj_t *)parent);

    if (!obj) {
        spdlog::error("[Card] Failed to create lv_obj");
        return NULL;
    }

    spdlog::trace("[Card] Created base lv_obj");
    return (void *)obj;
}

/**
 * XML apply handler for ui_card
 * Applies theme defaults + XML attributes to the card widget
 */
static void ui_card_xml_apply(lv_xml_parser_state_t *state, const char **attrs)
{
    void *item = lv_xml_state_get_item(state);
    lv_obj_t *obj = (lv_obj_t *)item;

    if (!obj) {
        spdlog::error("[Card] NULL object in xml_apply");
        return;
    }

    if (!card_theme_initialized) {
        spdlog::error("[Card] Theme not initialized - call ui_card_init() before registering components");
        return;
    }

    // Apply theme-aware defaults FIRST (lowest priority)
    // These can be overridden by XML attributes

    // 1. Background color (theme-aware: light/dark)
    lv_color_t bg_color = use_dark_mode ? card_bg_color_dark : card_bg_color_light;
    lv_obj_set_style_bg_color(obj, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);

    // 2. Border: 0 (no border by default)
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);

    // 3. Shadow: 0 (no shadow)
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);

    // 4. Border radius: Let LVGL theme provide responsive default (8px/12px)
    //    User can override in globals.xml with card_radius constant
    //    or per-instance with style_radius attribute

    // 5. Padding: Let LVGL theme provide responsive default (16/20/24px)
    //    User can override per-instance with style_pad_all attribute

    spdlog::trace("[Card] Applied theme defaults: bg={}, border=0, shadow=0",
                  use_dark_mode ? "dark" : "light");

    // Now apply standard lv_obj properties from XML (highest priority)
    // This allows XML attributes to override our defaults
    lv_xml_obj_apply(state, attrs);
}

void ui_card_register(void)
{
    if (!card_theme_initialized) {
        spdlog::error("[Card] Cannot register - theme not initialized. Call ui_card_init() first.");
        return;
    }

    lv_xml_register_widget("ui_card", ui_card_xml_create, ui_card_xml_apply);
    spdlog::info("[Card] Registered <ui_card> widget with LVGL XML system");
}
