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

#include "ui_icon.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/others/xml/lv_xml.h"
#include "lvgl/src/others/xml/lv_xml_widget.h"
#include "lvgl/src/others/xml/lv_xml_parser.h"
#include "lvgl/src/others/xml/lv_xml_style.h"
#include "lvgl/src/others/xml/parsers/lv_xml_obj_parser.h"
#include <cstring>

/**
 * Size mapping: semantic name -> {width, height, scale}
 * Material Design icons are 64x64 at scale 256 (1:1 ratio)
 * Scale = (size / 64) * 256 = size * 4
 */
struct IconSize {
    int32_t width;
    int32_t height;
    int32_t scale;
};

static const IconSize SIZE_XS = {16, 16, 64};
static const IconSize SIZE_SM = {24, 24, 96};
static const IconSize SIZE_MD = {32, 32, 128};
static const IconSize SIZE_LG = {48, 48, 192};
static const IconSize SIZE_XL = {64, 64, 256};

/**
 * Variant mapping: semantic name -> {recolor, opacity}
 * Colors are resolved from global theme constants at runtime.
 */
enum class IconVariant {
    NONE,
    PRIMARY,
    SECONDARY,
    ACCENT,
    DISABLED
};

/**
 * Parse size string to IconSize struct
 */
static bool parse_size(const char* size_str, IconSize* out_size) {
    if (strcmp(size_str, "xs") == 0) {
        *out_size = SIZE_XS;
        return true;
    } else if (strcmp(size_str, "sm") == 0) {
        *out_size = SIZE_SM;
        return true;
    } else if (strcmp(size_str, "md") == 0) {
        *out_size = SIZE_MD;
        return true;
    } else if (strcmp(size_str, "lg") == 0) {
        *out_size = SIZE_LG;
        return true;
    } else if (strcmp(size_str, "xl") == 0) {
        *out_size = SIZE_XL;
        return true;
    }

    LV_LOG_WARN("Invalid icon size '%s', using default 'xl'", size_str);
    *out_size = SIZE_XL;
    return false;
}

/**
 * Parse variant string to IconVariant enum
 */
static IconVariant parse_variant(const char* variant_str) {
    if (!variant_str || strlen(variant_str) == 0) {
        return IconVariant::NONE;
    } else if (strcmp(variant_str, "primary") == 0) {
        return IconVariant::PRIMARY;
    } else if (strcmp(variant_str, "secondary") == 0) {
        return IconVariant::SECONDARY;
    } else if (strcmp(variant_str, "accent") == 0) {
        return IconVariant::ACCENT;
    } else if (strcmp(variant_str, "disabled") == 0) {
        return IconVariant::DISABLED;
    } else if (strcmp(variant_str, "none") == 0) {
        return IconVariant::NONE;
    }

    LV_LOG_WARN("Invalid icon variant '%s', using default 'none'", variant_str);
    return IconVariant::NONE;
}

/**
 * Apply size properties to icon widget
 */
static void apply_size(lv_obj_t* obj, const IconSize& size) {
    lv_obj_set_size(obj, size.width, size.height);
    lv_image_set_scale(obj, size.scale);
}

/**
 * Apply variant color styling to icon widget
 *
 * Note: We look up styles by name from the XML-defined styles in icon.xml.
 * This keeps color constants in XML for easy theming.
 */
static void apply_variant(lv_obj_t* obj, IconVariant variant) {
    // Remove any existing variant styles first
    // (LVGL maintains a style list, we want only one variant active)
    static const char* variant_style_names[] = {
        "variant_primary",
        "variant_secondary",
        "variant_accent",
        "variant_disabled",
        "variant_none"
    };

    for (const char* style_name : variant_style_names) {
        lv_xml_style_t* xml_style = lv_xml_get_style_by_name(NULL, style_name);
        if (xml_style) {
            lv_obj_remove_style(obj, &xml_style->style, LV_PART_MAIN);
        }
    }

    // Apply the requested variant style
    const char* style_name = nullptr;
    switch (variant) {
        case IconVariant::PRIMARY:
            style_name = "variant_primary";
            break;
        case IconVariant::SECONDARY:
            style_name = "variant_secondary";
            break;
        case IconVariant::ACCENT:
            style_name = "variant_accent";
            break;
        case IconVariant::DISABLED:
            style_name = "variant_disabled";
            break;
        case IconVariant::NONE:
        default:
            style_name = "variant_none";
            break;
    }

    lv_xml_style_t* xml_style = lv_xml_get_style_by_name(NULL, style_name);
    if (xml_style) {
        lv_obj_add_style(obj, &xml_style->style, LV_PART_MAIN);
    } else {
        LV_LOG_WARN("Icon variant style '%s' not found in XML", style_name);
    }
}

/**
 * XML create function for icon widget
 * Called by LVGL XML parser when <icon> or <lv_image> is encountered
 */
static void* ui_icon_xml_create(lv_xml_parser_state_t* state, const char** attrs) {
    (void)attrs;  // Unused parameter
    lv_obj_t* parent = (lv_obj_t*)lv_xml_state_get_parent(state);
    lv_obj_t* obj = lv_image_create(parent);

    // Set default source to mat_home
    const void* default_src = lv_xml_get_image(NULL, "mat_home");
    if (default_src) {
        lv_image_set_src(obj, default_src);
    }

    // Apply default size (xl = 64x64, scale 256)
    apply_size(obj, SIZE_XL);

    // No variant by default (no recoloring)
    apply_variant(obj, IconVariant::NONE);

    return obj;
}

/**
 * XML apply function for icon widget
 * Called by LVGL XML parser to process attributes
 */
static void ui_icon_xml_apply(lv_xml_parser_state_t* state, const char** attrs) {
    lv_obj_t* obj = (lv_obj_t*)lv_xml_state_get_item(state);

    // First apply common lv_obj properties (width, height, align, etc.)
    lv_xml_obj_apply(state, attrs);

    // Then process icon-specific properties
    IconSize size = SIZE_XL;  // Track size to apply once
    IconVariant variant = IconVariant::NONE;  // Track variant to apply once
    bool size_set = false;
    bool variant_set = false;

    for (int i = 0; attrs[i]; i += 2) {
        const char* name = attrs[i];
        const char* value = attrs[i + 1];

        if (strcmp(name, "src") == 0) {
            // Look up image by name in XML registry
            const void* img_src = lv_xml_get_image(NULL, value);
            if (img_src) {
                lv_image_set_src(obj, img_src);
            } else {
                LV_LOG_WARN("Icon image '%s' not found in XML registry", value);
            }
        }
        else if (strcmp(name, "size") == 0) {
            parse_size(value, &size);
            size_set = true;
        }
        else if (strcmp(name, "variant") == 0) {
            variant = parse_variant(value);
            variant_set = true;
        }
    }

    // Apply size if it was set
    if (size_set) {
        apply_size(obj, size);
    }

    // Apply variant if it was set
    if (variant_set) {
        apply_variant(obj, variant);
    }
}

/**
 * Register the icon widget with LVGL's XML system
 */
void ui_icon_register_widget() {
    lv_xml_widget_register("icon", ui_icon_xml_create, ui_icon_xml_apply);
    LV_LOG_USER("Icon widget registered with XML system");
}
