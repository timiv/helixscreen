// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_switch.h"

#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"
#include "lvgl/src/xml/lv_xml_parser.h"
#include "lvgl/src/xml/lv_xml_style.h"
#include "lvgl/src/xml/lv_xml_widget.h"
#include "lvgl/src/xml/parsers/lv_xml_obj_parser.h"
#include "sound_manager.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <cstring>

/**
 * Size preset bundles for ui_switch
 * Maps semantic size names to dimension values (queried from XML constants)
 */
struct SwitchSizePreset {
    int32_t width;
    int32_t height;
    int32_t knob_pad;
    int32_t vert_margin;  // Vertical margin to reserve space for knob overflow
    int32_t horiz_margin; // Horizontal margin for knob extending past track edges
};

/**
 * Query a switch size constant from XML and calculate margins
 * Returns dimension value, or fallback if constant not found
 * Margins are calculated as ~25% of height for knob overflow
 */
static int32_t get_switch_dimension(const char* const_name, int32_t fallback) {
    const char* value_str = lv_xml_get_const(nullptr, const_name);
    if (!value_str) {
        spdlog::warn("[Switch] Constant '{}' not found, using fallback {}", const_name, fallback);
        return fallback;
    }

    int32_t value = lv_xml_atoi(value_str);
    spdlog::trace("[Switch] Loaded constant '{}' = {}px", const_name, value);
    return value;
}

/**
 * Determine screen breakpoint suffix based on display resolution
 * Returns "_small", "_medium", or "_large"
 */
static const char* get_breakpoint_suffix() {
    lv_display_t* display = lv_display_get_default();
    if (!display) {
        return "_medium"; // Fallback
    }

    int32_t hor_res = lv_display_get_horizontal_resolution(display);
    int32_t ver_res = lv_display_get_vertical_resolution(display);
    int32_t greater_res = (hor_res > ver_res) ? hor_res : ver_res;

    // Match original breakpoint logic
    if (greater_res <= 480) {
        return "_small";
    } else if (greater_res <= 800) {
        return "_medium";
    } else {
        return "_large";
    }
}

/**
 * Build a size preset by querying responsive constants (2D matrix)
 * size_suffix: "_tiny", "_small", "_medium", or "_large" (semantic size)
 * Combines with screen breakpoint to query: switch_{property}_{size}_{breakpoint}
 */
static SwitchSizePreset build_size_preset(const char* size_suffix) {
    const char* breakpoint_suffix = get_breakpoint_suffix();

    char width_const[64], height_const[64], knob_pad_const[64];
    snprintf(width_const, sizeof(width_const), "switch_width%s%s", size_suffix, breakpoint_suffix);
    snprintf(height_const, sizeof(height_const), "switch_height%s%s", size_suffix,
             breakpoint_suffix);
    snprintf(knob_pad_const, sizeof(knob_pad_const), "switch_knob_pad%s%s", size_suffix,
             breakpoint_suffix);

    int32_t width = get_switch_dimension(width_const, 40);
    int32_t height = get_switch_dimension(height_const, 20);
    int32_t knob_pad = get_switch_dimension(knob_pad_const, 1);

    // Calculate margins: knob extends ~25% beyond track on each side
    int32_t vert_margin = (height + 3) / 4; // height * 0.25, rounded up
    int32_t horiz_margin = vert_margin;     // Same for horizontal overflow

    spdlog::trace("[Switch] Built preset: size={}, breakpoint={} -> {}x{}, pad={}", size_suffix,
                  breakpoint_suffix, width, height, knob_pad);

    return {width, height, knob_pad, vert_margin, horiz_margin};
}

/**
 * Parse size string to SwitchSizePreset by querying XML constants
 * Returns true if valid size found, false otherwise
 */
static bool parse_size_preset(const char* size_str, SwitchSizePreset* out_preset) {
    const char* suffix = nullptr;

    if (strcmp(size_str, "tiny") == 0) {
        suffix = "_tiny";
    } else if (strcmp(size_str, "small") == 0) {
        suffix = "_small";
    } else if (strcmp(size_str, "medium") == 0) {
        suffix = "_medium";
    } else if (strcmp(size_str, "large") == 0) {
        suffix = "_large";
    } else {
        spdlog::warn("[Switch] Invalid size '{}', ignoring preset", size_str);
        return false;
    }

    *out_preset = build_size_preset(suffix);
    return true;
}

/**
 * Apply size preset to switch widget
 * Sets width, height, knob padding, and vertical margin as a bundle
 */
static void apply_size_preset(lv_obj_t* obj, const SwitchSizePreset& preset) {
    lv_obj_set_size(obj, preset.width, preset.height);
    lv_obj_set_style_pad_all(obj, preset.knob_pad, LV_PART_KNOB);

    // Add margins to reserve space for knob overflow
    // The knob extends beyond the track on all sides
    lv_obj_set_style_margin_top(obj, preset.vert_margin, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(obj, preset.vert_margin, LV_PART_MAIN);
    lv_obj_set_style_margin_left(obj, preset.horiz_margin, LV_PART_MAIN);
    lv_obj_set_style_margin_right(obj, preset.horiz_margin, LV_PART_MAIN);

    // Allow knob to overflow container bounds (prevents clipping)
    // NOTE: LV_OBJ_FLAG_OVERFLOW_VISIBLE when SET means "clip overflow"
    //       We need to CLEAR this flag to allow overflow
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    spdlog::trace("[Switch] Applied size preset: {}x{}, knob_pad={}, margins=v{}/h{}", preset.width,
                  preset.height, preset.knob_pad, preset.vert_margin, preset.horiz_margin);
}

/**
 * @brief Event callback for LV_EVENT_VALUE_CHANGED — plays toggle on/off sound
 *
 * Hooked at the component level so ALL <ui_switch> instances get audio
 * feedback. Checks LV_STATE_CHECKED to determine on vs off.
 */
static void switch_value_changed_sound_cb(lv_event_t* e) {
    auto* sw = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SoundManager::instance().play(checked ? "toggle_on" : "toggle_off");
}

/**
 * XML create handler for ui_switch
 * Creates an lv_switch widget when <ui_switch> is encountered in XML
 */
static void* ui_switch_xml_create(lv_xml_parser_state_t* state, const char** attrs) {
    LV_UNUSED(attrs);

    void* parent = lv_xml_state_get_parent(state);
    lv_obj_t* obj = lv_switch_create((lv_obj_t*)parent);

    if (!obj) {
        spdlog::error("[Switch] Failed to create lv_switch");
        return nullptr;
    }

    lv_obj_add_event_cb(obj, switch_value_changed_sound_cb, LV_EVENT_VALUE_CHANGED, nullptr);

    return (void*)obj;
}

/**
 * XML apply handler for ui_switch
 * Applies attributes from XML to the switch widget with 3-pass size handling
 */
static void ui_switch_xml_apply(lv_xml_parser_state_t* state, const char** attrs) {
    void* item = lv_xml_state_get_item(state);
    lv_obj_t* obj = (lv_obj_t*)item;

    if (!obj) {
        spdlog::error("[Switch] NULL object in xml_apply");
        return;
    }

    // PASS 1: Extract size preset AND explicit dimension overrides
    SwitchSizePreset preset;
    bool preset_found = false;
    int32_t explicit_width = -1;
    int32_t explicit_height = -1;
    int32_t explicit_knob_pad = -1;

    for (int i = 0; attrs[i]; i += 2) {
        const char* name = attrs[i];
        const char* value = attrs[i + 1];

        if (strcmp(name, "size") == 0) {
            preset_found = parse_size_preset(value, &preset);
        } else if (strcmp(name, "width") == 0) {
            explicit_width = atoi(value);
        } else if (strcmp(name, "height") == 0) {
            explicit_height = atoi(value);
        } else if (strcmp(name, "knob_pad") == 0) {
            explicit_knob_pad = atoi(value);
        }
    }

    // Apply standard lv_obj properties first (LVGL theme + XML attributes)
    lv_xml_obj_apply(state, attrs);

    // Apply custom styling AFTER theme (to override defaults)
    //
    // Switch anatomy (3 layers, drawn back-to-front):
    //
    // LV_PART_MAIN - Background track (always visible)
    //   - Base rectangle behind everything
    //   - Visible when UNCHECKED (behind knob)
    //   - Mostly covered by INDICATOR when CHECKED
    //
    // LV_PART_INDICATOR - Filled/active portion (drawn on top of MAIN)
    //   - Always drawn by LVGL, styled differently per state
    //   - UNCHECKED: invisible or same as MAIN (you don't notice it)
    //   - CHECKED: the "filled" track showing switch is ON
    //
    // LV_PART_KNOB - The sliding handle (drawn last, on top)
    //   - Circular button that slides left/right
    //   - Always visible in both states
    // Get accent colors for switch styling
    const char* primary_str = lv_xml_get_const(NULL, "primary");
    const char* tertiary_str = lv_xml_get_const(NULL, "tertiary");

    // CHECKED state indicator: secondary accent color, 40% opacity
    lv_color_t secondary = theme_manager_get_color("secondary");
    lv_obj_set_style_bg_color(obj, secondary, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(obj, 102, LV_PART_INDICATOR | LV_STATE_CHECKED);

    if (primary_str && tertiary_str) {
        // Knob color: more saturated of primary vs tertiary
        lv_color_t knob_color = theme_get_knob_color();

        // CHECKED state knob: saturated accent color
        lv_obj_set_style_bg_color(obj, knob_color, LV_PART_KNOB | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_CHECKED);
    }

    // UNCHECKED state: 40% track opacity
    // Knob color comes from theme_core's switch_knob_style (brighter of secondary/tertiary)
    lv_obj_set_style_bg_opa(obj, 102, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_DEFAULT);

    // DISABLED state: mode-aware styling using theme colors for proper contrast
    // Light mode: mix toward dark theme colors; Dark mode: mix toward light theme colors
    lv_color_t track_color = lv_obj_get_style_bg_color(obj, LV_PART_MAIN);
    bool is_dark = theme_manager_is_dark_mode();

    // Get theme colors for mixing (preserves theme warmth/coolness)
    const char* dark_color_str = lv_xml_get_const(NULL, "elevated_bg");
    const char* light_color_str = lv_xml_get_const(NULL, "text_subtle");

    if (dark_color_str && light_color_str) {
        lv_color_t dark_color = theme_manager_parse_hex_color(dark_color_str);
        lv_color_t light_color = theme_manager_parse_hex_color(light_color_str);

        lv_color_t disabled_track;
        lv_color_t disabled_knob;
        lv_opa_t track_opa;

        if (is_dark) {
            // Dark mode: lighten track toward theme's light color
            disabled_track = lv_color_mix(light_color, track_color, LV_OPA_20);
            disabled_knob = lv_color_mix(light_color, disabled_track, LV_OPA_40);
            track_opa = 77; // ~30%
        } else {
            // Light mode: darken track toward theme's dark color for visibility
            disabled_track = lv_color_mix(dark_color, track_color, LV_OPA_40);
            disabled_knob = lv_color_mix(dark_color, track_color, LV_OPA_30);
            track_opa = 128; // ~50%
        }

        lv_obj_set_style_bg_color(obj, disabled_track, LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(obj, track_opa, LV_PART_MAIN | LV_STATE_DISABLED);

        lv_obj_set_style_bg_color(obj, disabled_track, LV_PART_INDICATOR | LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(obj, track_opa, LV_PART_INDICATOR | LV_STATE_DISABLED);

        lv_obj_set_style_bg_color(obj, disabled_knob, LV_PART_KNOB | LV_STATE_DISABLED);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_DISABLED);
    }

    // PASS 2: Apply size preset (if found), then process other custom properties
    if (preset_found) {
        apply_size_preset(obj, preset);
    }

    for (int i = 0; attrs[i]; i += 2) {
        const char* name = attrs[i];
        const char* value = attrs[i + 1];

        if (strcmp(name, "checked") == 0) {
            // Handle checked state
            if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                lv_obj_add_state(obj, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(obj, LV_STATE_CHECKED);
            }
        } else if (strcmp(name, "orientation") == 0) {
            // Handle orientation
            if (strcmp(value, "horizontal") == 0) {
                lv_switch_set_orientation(obj, LV_SWITCH_ORIENTATION_HORIZONTAL);
            } else if (strcmp(value, "vertical") == 0) {
                lv_switch_set_orientation(obj, LV_SWITCH_ORIENTATION_VERTICAL);
            } else if (strcmp(value, "auto") == 0) {
                lv_switch_set_orientation(obj, LV_SWITCH_ORIENTATION_AUTO);
            }
        }
    }

    // PASS 3: Apply explicit overrides AFTER preset
    // This allows size="medium" width="100" to override just width
    if (explicit_width > 0) {
        lv_obj_set_width(obj, explicit_width);
        spdlog::trace("[Switch] Explicit width override: {}px", explicit_width);
    }
    if (explicit_height > 0) {
        lv_obj_set_height(obj, explicit_height);
        spdlog::trace("[Switch] Explicit height override: {}px", explicit_height);
    }
    if (explicit_knob_pad >= 0) {
        lv_obj_set_style_pad_all(obj, explicit_knob_pad, LV_PART_KNOB);
        spdlog::trace("[Switch] Explicit knob_pad override: {}px", explicit_knob_pad);
    }

    // Log final state
    int32_t actual_w = lv_obj_get_width(obj);
    int32_t actual_h = lv_obj_get_height(obj);
    int32_t actual_knob_pad = lv_obj_get_style_pad_left(obj, LV_PART_KNOB);
    spdlog::trace("[Switch] Final size: {}x{}, knob_pad={}px", actual_w, actual_h, actual_knob_pad);
}

/**
 * Register responsive constants for switch sizing based on screen dimensions
 * Call this BEFORE registering XML components that use switches
 */
/**
 * Register test panel-specific constants
 *
 * Queries responsive switch dimensions (2D matrix) and creates test panel
 * aliases plus computed values like row heights.
 *
 * Called once at startup from xml_registration.cpp
 */
void ui_switch_register_responsive_constants() {
    spdlog::trace("[Switch] Registering test panel responsive constants");

    // Determine current screen breakpoint
    const char* breakpoint_suffix = get_breakpoint_suffix();

    // Query default switch size (small size for current breakpoint)
    char width_const[64], height_const[64], knob_pad_const[64];
    snprintf(width_const, sizeof(width_const), "switch_width_small%s", breakpoint_suffix);
    snprintf(height_const, sizeof(height_const), "switch_height_small%s", breakpoint_suffix);
    snprintf(knob_pad_const, sizeof(knob_pad_const), "switch_knob_pad_small%s", breakpoint_suffix);

    const char* switch_width = lv_xml_get_const(nullptr, width_const);
    const char* switch_height = lv_xml_get_const(nullptr, height_const);
    const char* knob_pad = lv_xml_get_const(nullptr, knob_pad_const);

    if (!switch_width || !switch_height || !knob_pad) {
        spdlog::error("[Switch] Responsive constants not found for breakpoint {}",
                      breakpoint_suffix);
        return;
    }

    // Get display for breakpoint detection (for computed values like row heights)
    lv_display_t* display = lv_display_get_default();
    int32_t ver_res = lv_display_get_vertical_resolution(display);

    // Compute row heights based on switch height + padding
    // These are test panel specific and can't be in globals.xml
    int32_t height_val = lv_xml_atoi(switch_height);
    int32_t row_padding = (ver_res <= 480) ? 20 : (ver_res <= 800) ? 18 : 20;

    char row_height[16], row_height_large[16];
    snprintf(row_height, sizeof(row_height), "%d", height_val + (2 * row_padding));
    snprintf(row_height_large, sizeof(row_height_large), "%d", height_val + (2 * row_padding) + 10);

    // Label fonts for test panel (could be moved to globals.xml if needed elsewhere)
    const char* label_font = lv_xml_get_const(nullptr, "font_body");
    const char* label_large_font = lv_xml_get_const(nullptr, "font_heading");

    if (!label_font) {
        label_font = "montserrat_16"; // Fallback
    }
    if (!label_large_font) {
        label_large_font = "montserrat_20"; // Fallback
    }

    // Get globals scope for constant registration
    lv_xml_component_scope_t* scope = lv_xml_component_get_scope("globals");
    if (!scope) {
        spdlog::error("[Switch] Failed to get globals scope for constant registration");
        return;
    }

    // Register test panel aliases (for backward compatibility)
    lv_xml_register_const(scope, "test_switch_width", switch_width);
    lv_xml_register_const(scope, "test_switch_height", switch_height);
    lv_xml_register_const(scope, "test_switch_knob_pad", knob_pad);

    // Register computed test panel constants
    lv_xml_register_const(scope, "test_row_height", row_height);
    lv_xml_register_const(scope, "test_row_height_large", row_height_large);
    lv_xml_register_const(scope, "test_label_font", label_font);
    lv_xml_register_const(scope, "test_label_large_font", label_large_font);

    // Large variant aliases (query from 2D matrix)
    char width_lg[64], height_lg[64], knob_pad_lg[64];
    snprintf(width_lg, sizeof(width_lg), "switch_width_large%s", breakpoint_suffix);
    snprintf(height_lg, sizeof(height_lg), "switch_height_large%s", breakpoint_suffix);
    snprintf(knob_pad_lg, sizeof(knob_pad_lg), "switch_knob_pad_large%s", breakpoint_suffix);

    const char* switch_width_lg = lv_xml_get_const(nullptr, width_lg);
    const char* switch_height_lg = lv_xml_get_const(nullptr, height_lg);
    const char* knob_pad_lg_val = lv_xml_get_const(nullptr, knob_pad_lg);

    if (switch_width_lg)
        lv_xml_register_const(scope, "test_switch_width_large", switch_width_lg);
    if (switch_height_lg)
        lv_xml_register_const(scope, "test_switch_height_large", switch_height_lg);
    if (knob_pad_lg_val)
        lv_xml_register_const(scope, "test_switch_knob_pad_large", knob_pad_lg_val);

    spdlog::trace(
        "[Switch] Registered test constants (breakpoint={}): switch={}x{} (pad={}), row={}",
        breakpoint_suffix, switch_width, switch_height, knob_pad, row_height);
}

/**
 * Register the ui_switch widget with LVGL's XML system
 */
void ui_switch_register() {
    lv_xml_register_widget("ui_switch", ui_switch_xml_create, ui_switch_xml_apply);
    spdlog::trace("[Switch] Registered ui_switch widget with XML system (queries responsive "
                  "constants at runtime)");
}
