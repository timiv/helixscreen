// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_settings_filament_sensors.cpp
 * @brief Implementation of FilamentSensorSettingsOverlay
 */

#include "ui_settings_filament_sensors.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"

#include "filament_sensor_manager.h"
#include "filament_sensor_types.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::settings {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<FilamentSensorSettingsOverlay> g_filament_sensor_settings_overlay;

FilamentSensorSettingsOverlay& get_filament_sensor_settings_overlay() {
    if (!g_filament_sensor_settings_overlay) {
        g_filament_sensor_settings_overlay = std::make_unique<FilamentSensorSettingsOverlay>();
        StaticPanelRegistry::instance().register_destroy(
            "FilamentSensorSettingsOverlay", []() { g_filament_sensor_settings_overlay.reset(); });
    }
    return *g_filament_sensor_settings_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

FilamentSensorSettingsOverlay::FilamentSensorSettingsOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

FilamentSensorSettingsOverlay::~FilamentSensorSettingsOverlay() {
    spdlog::debug("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void FilamentSensorSettingsOverlay::register_callbacks() {
    // Master toggle callback (used by XML event_cb)
    lv_xml_register_event_cb(nullptr, "on_filament_master_toggle_changed",
                             on_filament_master_toggle_changed);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* FilamentSensorSettingsOverlay::create(lv_obj_t* parent) {
    if (overlay_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    // Create from XML component
    overlay_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "filament_sensors_overlay", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_;
}

void FilamentSensorSettingsOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    parent_screen_ = parent_screen;

    // Lazy create overlay
    if (!overlay_ && parent_screen_) {
        create(parent_screen_);
    }

    if (!overlay_) {
        spdlog::error("[{}] Cannot show - overlay not created", get_name());
        return;
    }

    // Update sensor count and populate list
    update_sensor_count_label();
    populate_sensor_list();

    // Push onto navigation stack
    ui_nav_push_overlay(overlay_);
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void FilamentSensorSettingsOverlay::update_sensor_count_label() {
    if (!overlay_)
        return;

    lv_obj_t* count_label = lv_obj_find_by_name(overlay_, "sensor_count_label");
    if (count_label) {
        auto& mgr = helix::FilamentSensorManager::instance();
        lv_label_set_text_fmt(count_label, "(%zu)", mgr.sensor_count());
    }
}

void FilamentSensorSettingsOverlay::populate_sensor_list() {
    if (!overlay_) {
        return;
    }

    lv_obj_t* sensors_list = lv_obj_find_by_name(overlay_, "sensors_list");
    if (!sensors_list) {
        spdlog::error("[{}] Could not find sensors_list container", get_name());
        return;
    }

    // Clear existing rows (except placeholder which is handled by XML binding)
    lv_obj_t* placeholder = lv_obj_find_by_name(sensors_list, "no_sensors_placeholder");
    uint32_t child_count = lv_obj_get_child_count(sensors_list);
    for (int i = static_cast<int>(child_count) - 1; i >= 0; i--) {
        lv_obj_t* child = lv_obj_get_child(sensors_list, i);
        if (child != placeholder) {
            lv_obj_delete(child);
        }
    }

    // Get discovered sensors
    auto& mgr = helix::FilamentSensorManager::instance();
    auto sensors = mgr.get_sensors();

    spdlog::debug("[{}] Populating sensor list with {} sensors", get_name(), sensors.size());

    // Create a row for each sensor
    for (const auto& sensor : sensors) {
        // Create sensor row from XML component
        const char* attrs[] = {
            "sensor_name", sensor.sensor_name.c_str(), "sensor_type",
            sensor.type == helix::FilamentSensorType::MOTION ? "motion" : "switch", nullptr};
        auto* row =
            static_cast<lv_obj_t*>(lv_xml_create(sensors_list, "filament_sensor_row", attrs));
        if (!row) {
            spdlog::error("[{}] Failed to create sensor row for {}", get_name(),
                          sensor.sensor_name);
            continue;
        }

        // Store klipper_name as user data for callbacks
        // Note: We need to allocate this because sensor goes out of scope
        char* klipper_name = static_cast<char*>(lv_malloc(sensor.klipper_name.size() + 1));
        if (!klipper_name) {
            spdlog::error("[{}] Failed to allocate memory for sensor name: {}", get_name(),
                          sensor.klipper_name);
            continue;
        }
        strcpy(klipper_name, sensor.klipper_name.c_str());
        lv_obj_set_user_data(row, klipper_name);

        // Register cleanup to free allocated string when row is deleted
        // (LV_EVENT_DELETE is acceptable exception to "no lv_obj_add_event_cb" rule)
        lv_obj_add_event_cb(
            row,
            [](lv_event_t* e) {
                lv_obj_t* obj = lv_event_get_target_obj(e);
                char* data = static_cast<char*>(lv_obj_get_user_data(obj));
                if (data) {
                    lv_free(data);
                }
            },
            LV_EVENT_DELETE, nullptr);

        // Wire up role dropdown
        lv_obj_t* role_dropdown = lv_obj_find_by_name(row, "role_dropdown");
        if (role_dropdown) {
            // Set options with proper newline separators (XML can't do this)
            lv_dropdown_set_options(role_dropdown, "None\nRunout\nToolhead\nEntry");

            // Set current role
            lv_dropdown_set_selected(role_dropdown, static_cast<uint32_t>(sensor.role));

            // Store klipper_name reference for callback
            lv_obj_set_user_data(role_dropdown, klipper_name);

            // Wire up value change
            lv_obj_add_event_cb(
                role_dropdown,
                [](lv_event_t* e) {
                    auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
                    auto* klipper_name_ptr =
                        static_cast<const char*>(lv_obj_get_user_data(dropdown));
                    if (!klipper_name_ptr)
                        return;

                    int index = static_cast<int>(lv_dropdown_get_selected(dropdown));
                    auto role = static_cast<helix::FilamentSensorRole>(index);

                    auto& mgr = helix::FilamentSensorManager::instance();
                    mgr.set_sensor_role(klipper_name_ptr, role);
                    mgr.save_config();
                    spdlog::info("[FilamentSensorSettingsOverlay] Sensor {} role changed to {}",
                                 klipper_name_ptr, helix::role_to_config_string(role));
                },
                LV_EVENT_VALUE_CHANGED, nullptr);
        }

        // Wire up enable toggle
        lv_obj_t* enable_toggle = lv_obj_find_by_name(row, "enable_toggle");
        if (enable_toggle) {
            // Set current state
            if (sensor.enabled) {
                lv_obj_add_state(enable_toggle, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(enable_toggle, LV_STATE_CHECKED);
            }

            // Store klipper_name reference for callback
            lv_obj_set_user_data(enable_toggle, klipper_name);

            // Wire up value change
            lv_obj_add_event_cb(
                enable_toggle,
                [](lv_event_t* e) {
                    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
                    auto* klipper_name_ptr = static_cast<const char*>(lv_obj_get_user_data(toggle));
                    if (!klipper_name_ptr)
                        return;

                    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);

                    auto& mgr = helix::FilamentSensorManager::instance();
                    mgr.set_sensor_enabled(klipper_name_ptr, enabled);
                    mgr.save_config();
                    spdlog::info("[FilamentSensorSettingsOverlay] Sensor {} enabled: {}",
                                 klipper_name_ptr, enabled ? "ON" : "OFF");
                },
                LV_EVENT_VALUE_CHANGED, nullptr);
        }

        spdlog::debug("[{}]   Created row for sensor: {}", get_name(), sensor.sensor_name);
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void FilamentSensorSettingsOverlay::handle_master_toggle_changed(bool enabled) {
    auto& mgr = helix::FilamentSensorManager::instance();
    mgr.set_master_enabled(enabled);
    mgr.save_config();
    spdlog::info("[{}] Master enabled: {}", get_name(), enabled ? "ON" : "OFF");
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void FilamentSensorSettingsOverlay::on_filament_master_toggle_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[FilamentSensorSettingsOverlay] on_filament_master_toggle_changed");
    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    get_filament_sensor_settings_overlay().handle_master_toggle_changed(enabled);
    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::settings
