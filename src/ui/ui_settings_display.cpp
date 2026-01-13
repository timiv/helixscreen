// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_settings_display.cpp
 * @brief Implementation of DisplaySettingsOverlay
 */

#include "ui_settings_display.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"

#include "settings_manager.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::settings {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<DisplaySettingsOverlay> g_display_settings_overlay;

DisplaySettingsOverlay& get_display_settings_overlay() {
    if (!g_display_settings_overlay) {
        g_display_settings_overlay = std::make_unique<DisplaySettingsOverlay>();
        StaticPanelRegistry::instance().register_destroy(
            "DisplaySettingsOverlay", []() { g_display_settings_overlay.reset(); });
    }
    return *g_display_settings_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

DisplaySettingsOverlay::DisplaySettingsOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

DisplaySettingsOverlay::~DisplaySettingsOverlay() {
    if (subjects_initialized_ && lv_is_initialized()) {
        lv_subject_deinit(&brightness_value_subject_);
    }
    spdlog::debug("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void DisplaySettingsOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // Initialize brightness value subject for label binding
    // 5-arg form: (subject, buf, prev_buf, size, initial_value)
    snprintf(brightness_value_buf_, sizeof(brightness_value_buf_), "100%%");
    lv_subject_init_string(&brightness_value_subject_, brightness_value_buf_, nullptr,
                           sizeof(brightness_value_buf_), brightness_value_buf_);
    lv_xml_register_subject(nullptr, "brightness_value", &brightness_value_subject_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void DisplaySettingsOverlay::register_callbacks() {
    // Brightness slider callback
    lv_xml_register_event_cb(nullptr, "on_brightness_changed", on_brightness_changed);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* DisplaySettingsOverlay::create(lv_obj_t* parent) {
    if (overlay_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    // Create from XML component
    overlay_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "display_settings_overlay", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_;
}

void DisplaySettingsOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    parent_screen_ = parent_screen;

    // Ensure subjects are initialized
    if (!subjects_initialized_) {
        init_subjects();
    }

    // Lazy create overlay
    if (!overlay_ && parent_screen_) {
        create(parent_screen_);
    }

    if (!overlay_) {
        spdlog::error("[{}] Cannot show - overlay not created", get_name());
        return;
    }

    // Initialize all widget values from SettingsManager
    init_brightness_controls();
    init_sleep_dropdown();
    init_bed_mesh_dropdown();
    init_gcode_dropdown();
    init_time_format_dropdown();

    // Push onto navigation stack
    ui_nav_push_overlay(overlay_);
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void DisplaySettingsOverlay::init_brightness_controls() {
    if (!overlay_)
        return;

    lv_obj_t* brightness_slider = lv_obj_find_by_name(overlay_, "brightness_slider");
    if (brightness_slider) {
        // Set initial value from settings
        int brightness = SettingsManager::instance().get_brightness();
        lv_slider_set_value(brightness_slider, brightness, LV_ANIM_OFF);

        // Update subject (label binding happens in XML)
        snprintf(brightness_value_buf_, sizeof(brightness_value_buf_), "%d%%", brightness);
        lv_subject_copy_string(&brightness_value_subject_, brightness_value_buf_);

        spdlog::debug("[{}] Brightness initialized to {}%", get_name(), brightness);
    }
}

void DisplaySettingsOverlay::init_sleep_dropdown() {
    if (!overlay_)
        return;

    lv_obj_t* sleep_row = lv_obj_find_by_name(overlay_, "row_display_sleep");
    lv_obj_t* sleep_dropdown = sleep_row ? lv_obj_find_by_name(sleep_row, "dropdown") : nullptr;
    if (sleep_dropdown) {
        // Set dropdown options
        lv_dropdown_set_options(sleep_dropdown,
                                "Never\n1 minute\n5 minutes\n10 minutes\n30 minutes");

        // Set initial selection based on current setting
        int current_sec = SettingsManager::instance().get_display_sleep_sec();
        int index = SettingsManager::sleep_seconds_to_index(current_sec);
        lv_dropdown_set_selected(sleep_dropdown, index);

        spdlog::debug("[{}] Sleep dropdown initialized to index {} ({}s)", get_name(), index,
                      current_sec);
    }
}

void DisplaySettingsOverlay::init_bed_mesh_dropdown() {
    if (!overlay_)
        return;

    lv_obj_t* bed_mesh_row = lv_obj_find_by_name(overlay_, "row_bed_mesh_mode");
    lv_obj_t* bed_mesh_dropdown =
        bed_mesh_row ? lv_obj_find_by_name(bed_mesh_row, "dropdown") : nullptr;
    if (bed_mesh_dropdown) {
        // Set dropdown options
        lv_dropdown_set_options(bed_mesh_dropdown,
                                SettingsManager::get_bed_mesh_render_mode_options());

        // Set initial selection based on current setting
        int current_mode = SettingsManager::instance().get_bed_mesh_render_mode();
        lv_dropdown_set_selected(bed_mesh_dropdown, current_mode);

        spdlog::debug("[{}] Bed mesh mode dropdown initialized to {} ({})", get_name(),
                      current_mode, current_mode == 0 ? "Auto" : (current_mode == 1 ? "3D" : "2D"));
    }
}

void DisplaySettingsOverlay::init_gcode_dropdown() {
    if (!overlay_)
        return;

    // G-code mode row is hidden by default, but we still initialize it
    lv_obj_t* gcode_row = lv_obj_find_by_name(overlay_, "row_gcode_mode");
    lv_obj_t* gcode_dropdown = gcode_row ? lv_obj_find_by_name(gcode_row, "dropdown") : nullptr;
    if (gcode_dropdown) {
        // Set dropdown options
        lv_dropdown_set_options(gcode_dropdown, SettingsManager::get_gcode_render_mode_options());

        // Set initial selection based on current setting
        int current_mode = SettingsManager::instance().get_gcode_render_mode();
        lv_dropdown_set_selected(gcode_dropdown, current_mode);

        spdlog::debug("[{}] G-code mode dropdown initialized to {} ({})", get_name(), current_mode,
                      current_mode == 0 ? "Auto" : (current_mode == 1 ? "3D" : "2D Layers"));
    }
}

void DisplaySettingsOverlay::init_time_format_dropdown() {
    if (!overlay_)
        return;

    lv_obj_t* time_format_row = lv_obj_find_by_name(overlay_, "row_time_format");
    lv_obj_t* time_format_dropdown =
        time_format_row ? lv_obj_find_by_name(time_format_row, "dropdown") : nullptr;
    if (time_format_dropdown) {
        // Set dropdown options
        lv_dropdown_set_options(time_format_dropdown, SettingsManager::get_time_format_options());

        // Set initial selection based on current setting
        auto current_format = SettingsManager::instance().get_time_format();
        lv_dropdown_set_selected(time_format_dropdown, static_cast<uint32_t>(current_format));

        spdlog::debug("[{}] Time format dropdown initialized to {} ({})", get_name(),
                      static_cast<int>(current_format),
                      current_format == TimeFormat::HOUR_12 ? "12H" : "24H");
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void DisplaySettingsOverlay::handle_brightness_changed(int value) {
    SettingsManager::instance().set_brightness(value);

    // Update subject (label binding happens in XML)
    snprintf(brightness_value_buf_, sizeof(brightness_value_buf_), "%d%%", value);
    lv_subject_copy_string(&brightness_value_subject_, brightness_value_buf_);
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void DisplaySettingsOverlay::on_brightness_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[DisplaySettingsOverlay] on_brightness_changed");
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int value = lv_slider_get_value(slider);
    get_display_settings_overlay().handle_brightness_changed(value);
    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::settings
