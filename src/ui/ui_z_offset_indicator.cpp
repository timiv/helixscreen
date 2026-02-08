// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_z_offset_indicator.h"

#include "ui_update_queue.h"

#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml_parser.h"
#include "lvgl/src/xml/lv_xml_widget.h"
#include "lvgl/src/xml/parsers/lv_xml_obj_parser.h"
#include "nozzle_renderer_bambu.h"
#include "nozzle_renderer_faceted.h"
#include "settings_manager.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

// ============================================================================
// Widget Data
// ============================================================================

struct ZOffsetIndicatorData {
    int32_t current_y_px = 0;          // Current animated nozzle Y position (in 0.1px units)
    int32_t target_y_px = 0;           // Target Y position (in 0.1px units)
    int32_t arrow_opacity = 0;         // 0-255, animated for direction flash
    int arrow_direction = 0;           // +1 (farther/up) or -1 (closer/down)
    bool use_faceted_toolhead = false; // Which nozzle renderer to use
};

// ============================================================================
// Mapping
// ============================================================================

/// Map microns (-2000 to +2000) to pixel offset in 0.1px units (-240 to +240)
/// Negative microns (closer to bed) = nozzle moves DOWN (positive Y in screen coords)
/// Positive microns (farther from bed) = nozzle moves UP (negative Y in screen coords)
static int32_t microns_to_tenths_px(int microns) {
    if (microns > 2000)
        microns = 2000;
    if (microns < -2000)
        microns = -2000;
    // -2000 microns -> +240 (nozzle down toward bed)
    // +2000 microns -> -240 (nozzle up away from bed)
    return -(microns * 240) / 2000;
}

// ============================================================================
// Animation Callbacks (forward declarations)
// ============================================================================

static void position_anim_cb(void* var, int32_t value);
static void arrow_anim_cb(void* var, int32_t value);

// ============================================================================
// Drawing
// ============================================================================

static void indicator_draw_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    lv_layer_t* layer = lv_event_get_layer(e);
    auto* data = static_cast<ZOffsetIndicatorData*>(lv_obj_get_user_data(obj));
    if (!data)
        return;

    // Get widget dimensions
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    int32_t w = lv_area_get_width(&coords);
    int32_t h = lv_area_get_height(&coords);
    int32_t cx = coords.x1 + w / 2;
    int32_t cy = coords.y1 + h / 2;

    // Bed line: horizontal line near the bottom of the widget area
    // Position it at ~80% of the widget height from the top
    int32_t bed_y = coords.y1 + (h * 4) / 5;
    lv_color_t bed_color = theme_manager_get_color("text_muted");

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = bed_color;
    line_dsc.width = 3;
    line_dsc.p1.x = coords.x1 + 4;
    line_dsc.p1.y = bed_y;
    line_dsc.p2.x = coords.x1 + w - 4;
    line_dsc.p2.y = bed_y;
    line_dsc.round_start = true;
    line_dsc.round_end = true;
    lv_draw_line(layer, &line_dsc);

    // Nozzle: centered horizontally, Y offset from widget center by animated position
    // current_y_px is in 0.1px units, so divide by 10
    int32_t nozzle_cy = cy + data->current_y_px / 10;
    lv_color_t nozzle_color = lv_color_hex(0x808080); // Neutral gray for indicator
    static constexpr int32_t NOZZLE_SCALE = 5;        // Compact nozzle

    if (data->use_faceted_toolhead) {
        draw_nozzle_faceted(layer, cx, nozzle_cy, nozzle_color, NOZZLE_SCALE);
    } else {
        draw_nozzle_bambu(layer, cx, nozzle_cy, nozzle_color, NOZZLE_SCALE);
    }

    // Direction arrow: draw when opacity > 0
    if (data->arrow_opacity > 0) {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = theme_manager_get_color("text");
        label_dsc.opa = static_cast<lv_opa_t>(data->arrow_opacity);
        label_dsc.align = LV_TEXT_ALIGN_CENTER;

        // Use the default font for arrow glyphs
        label_dsc.font = lv_font_get_default();

        // Arrow text: up arrow for farther (+1), down arrow for closer (-1)
        const char* arrow_text = (data->arrow_direction > 0) ? LV_SYMBOL_UP : LV_SYMBOL_DOWN;
        label_dsc.text = arrow_text;

        // Position to the right of the nozzle
        int32_t arrow_x = cx + NOZZLE_SCALE * 4;
        int32_t font_h = lv_font_get_line_height(label_dsc.font);
        lv_area_t arrow_area = {arrow_x - 10, nozzle_cy - font_h / 2, arrow_x + 10,
                                nozzle_cy + font_h / 2};
        lv_draw_label(layer, &label_dsc, &arrow_area);
    }
}

// ============================================================================
// Animation Callbacks
// ============================================================================

static void position_anim_cb(void* var, int32_t value) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(var);
    auto* data = static_cast<ZOffsetIndicatorData*>(lv_obj_get_user_data(obj));
    if (!data)
        return;

    data->current_y_px = value;

    // Defer invalidation to avoid calling during render phase
    ui_async_call(
        [](void* obj_ptr) {
            auto* o = static_cast<lv_obj_t*>(obj_ptr);
            if (lv_obj_is_valid(o)) {
                lv_obj_invalidate(o);
            }
        },
        obj);
}

static void arrow_anim_cb(void* var, int32_t value) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(var);
    auto* data = static_cast<ZOffsetIndicatorData*>(lv_obj_get_user_data(obj));
    if (!data)
        return;

    data->arrow_opacity = value;

    // Defer invalidation to avoid calling during render phase
    ui_async_call(
        [](void* obj_ptr) {
            auto* o = static_cast<lv_obj_t*>(obj_ptr);
            if (lv_obj_is_valid(o)) {
                lv_obj_invalidate(o);
            }
        },
        obj);
}

// ============================================================================
// Delete Callback
// ============================================================================

static void indicator_delete_cb(lv_event_t* e) {
    lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
    lv_anim_delete(obj, position_anim_cb);
    lv_anim_delete(obj, arrow_anim_cb);
    auto* data = static_cast<ZOffsetIndicatorData*>(lv_obj_get_user_data(obj));
    delete data;
    lv_obj_set_user_data(obj, nullptr);
}

// ============================================================================
// Public API
// ============================================================================

void ui_z_offset_indicator_set_value(lv_obj_t* obj, int microns) {
    if (!obj)
        return;
    auto* data = static_cast<ZOffsetIndicatorData*>(lv_obj_get_user_data(obj));
    if (!data)
        return;

    int32_t new_target = microns_to_tenths_px(microns);
    data->target_y_px = new_target;

    // Stop any existing position animation
    lv_anim_delete(obj, position_anim_cb);

    if (SettingsManager::instance().get_animations_enabled()) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, obj);
        lv_anim_set_values(&anim, data->current_y_px, new_target);
        lv_anim_set_duration(&anim, 200);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&anim, position_anim_cb);
        lv_anim_start(&anim);
    } else {
        // Jump directly to target
        data->current_y_px = new_target;
        lv_obj_invalidate(obj);
    }

    spdlog::trace("[ZOffsetIndicator] Set value: {} microns -> {} tenths_px", microns, new_target);
}

void ui_z_offset_indicator_flash_direction(lv_obj_t* obj, int direction) {
    if (!obj)
        return;
    auto* data = static_cast<ZOffsetIndicatorData*>(lv_obj_get_user_data(obj));
    if (!data)
        return;

    data->arrow_direction = direction;

    // Stop any existing arrow animation
    lv_anim_delete(obj, arrow_anim_cb);

    if (SettingsManager::instance().get_animations_enabled()) {
        lv_anim_t anim;
        lv_anim_init(&anim);
        lv_anim_set_var(&anim, obj);
        lv_anim_set_values(&anim, 255, 0); // Fade from full opacity to transparent
        lv_anim_set_duration(&anim, 400);
        lv_anim_set_path_cb(&anim, lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&anim, arrow_anim_cb);
        lv_anim_start(&anim);
    } else {
        // No animation - skip arrow entirely
        data->arrow_opacity = 0;
    }

    spdlog::trace("[ZOffsetIndicator] Flash direction: {}", direction > 0 ? "up" : "down");
}

// ============================================================================
// XML Widget Registration
// ============================================================================

static void* z_offset_indicator_xml_create(lv_xml_parser_state_t* state, const char** attrs) {
    LV_UNUSED(attrs);

    void* parent = lv_xml_state_get_parent(state);
    lv_obj_t* obj = lv_obj_create((lv_obj_t*)parent);

    if (!obj) {
        spdlog::error("[ZOffsetIndicator] Failed to create lv_obj");
        return nullptr;
    }

    // Set default size
    lv_obj_set_size(obj, LV_PCT(100), 80);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    // Remove default styles and make transparent
    lv_obj_remove_style_all(obj);

    // Allocate and attach widget data
    auto* data = new ZOffsetIndicatorData{};
    data->use_faceted_toolhead = false;
    lv_obj_set_user_data(obj, data);

    // Register draw and delete callbacks
    // NOTE: lv_obj_add_event_cb() is appropriate here - this is a custom widget, not a UI button
    lv_obj_add_event_cb(obj, indicator_draw_cb, LV_EVENT_DRAW_POST, nullptr);
    lv_obj_add_event_cb(obj, indicator_delete_cb, LV_EVENT_DELETE, nullptr);

    spdlog::trace("[ZOffsetIndicator] Created widget");
    return (void*)obj;
}

void ui_z_offset_indicator_register(void) {
    lv_xml_register_widget("z_offset_indicator", z_offset_indicator_xml_create, lv_xml_obj_apply);
    spdlog::trace("[ZOffsetIndicator] Registered <z_offset_indicator> widget");
}
