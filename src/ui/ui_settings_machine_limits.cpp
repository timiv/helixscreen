// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_settings_machine_limits.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"
#include "ui_toast.h"
#include "ui_update_queue.h"

#include "moonraker_api.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::settings {

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

static std::unique_ptr<MachineLimitsOverlay> g_machine_limits_overlay;

MachineLimitsOverlay& get_machine_limits_overlay() {
    if (!g_machine_limits_overlay) {
        g_machine_limits_overlay = std::make_unique<MachineLimitsOverlay>();
        StaticPanelRegistry::instance().register_destroy(
            "MachineLimitsOverlay", []() { g_machine_limits_overlay.reset(); });
    }
    return *g_machine_limits_overlay;
}

void init_machine_limits_overlay(MoonrakerAPI* api) {
    auto& overlay = get_machine_limits_overlay();
    overlay.set_api(api);
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

MachineLimitsOverlay::MachineLimitsOverlay() {
    spdlog::trace("[{}] Constructor", get_name());
}

MachineLimitsOverlay::~MachineLimitsOverlay() {
    deinit_subjects();
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void MachineLimitsOverlay::set_api(MoonrakerAPI* api) {
    api_ = api;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void MachineLimitsOverlay::init_subjects() {
    if (subjects_initialized_) {
        spdlog::warn("[{}] init_subjects() called twice - ignoring", get_name());
        return;
    }

    // Initialize display subjects for XML binding
    UI_MANAGED_SUBJECT_STRING(max_velocity_display_subject_, velocity_buf_, "-- mm/s",
                              "max_velocity_display", subjects_);

    UI_MANAGED_SUBJECT_STRING(max_accel_display_subject_, accel_buf_, "-- mm/s²",
                              "max_accel_display", subjects_);

    UI_MANAGED_SUBJECT_STRING(accel_to_decel_display_subject_, a2d_buf_, "-- mm/s²",
                              "accel_to_decel_display", subjects_);

    UI_MANAGED_SUBJECT_STRING(square_corner_velocity_display_subject_, scv_buf_, "-- mm/s",
                              "square_corner_velocity_display", subjects_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void MachineLimitsOverlay::register_callbacks() {
    // Register slider change callbacks
    lv_xml_register_event_cb(nullptr, "on_max_velocity_changed", on_velocity_changed);
    lv_xml_register_event_cb(nullptr, "on_max_accel_changed", on_accel_changed);
    lv_xml_register_event_cb(nullptr, "on_accel_to_decel_changed", on_a2d_changed);
    lv_xml_register_event_cb(nullptr, "on_square_corner_velocity_changed", on_scv_changed);

    // Register button callbacks
    lv_xml_register_event_cb(nullptr, "on_limits_reset", on_reset);
    lv_xml_register_event_cb(nullptr, "on_limits_apply", on_apply);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

void MachineLimitsOverlay::deinit_subjects() {
    // SubjectManager handles cleanup automatically via RAII
    subjects_initialized_ = false;
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* MachineLimitsOverlay::create(lv_obj_t* parent) {
    if (!parent) {
        spdlog::error("[{}] NULL parent", get_name());
        return nullptr;
    }

    // Create overlay from XML
    overlay_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "machine_limits_overlay", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Initially hidden
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);
    spdlog::info("[{}] Overlay created", get_name());

    return overlay_;
}

void MachineLimitsOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    // Ensure subjects and callbacks are initialized
    if (!subjects_initialized_) {
        init_subjects();
        register_callbacks();
    }

    // Create overlay on first access (lazy initialization)
    if (!overlay_ && parent_screen) {
        create(parent_screen);
    }

    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay", get_name());
        ui_toast_show(ToastSeverity::ERROR, "Failed to load overlay", 2000);
        return;
    }

    // Query current limits from printer and show
    query_and_show(parent_screen);
}

void MachineLimitsOverlay::query_and_show(lv_obj_t* /*parent_screen*/) {
    if (api_) {
        api_->get_machine_limits(
            [this](const MachineLimits& limits) {
                // Capture limits by value and defer to main thread for LVGL calls
                ui_queue_update([this, limits]() {
                    spdlog::info("[{}] Got machine limits: vel={}, accel={}, a2d={}, scv={}",
                                 get_name(), limits.max_velocity, limits.max_accel,
                                 limits.max_accel_to_decel, limits.square_corner_velocity);

                    // Store both current and original for reset
                    current_limits_ = limits;
                    original_limits_ = limits;

                    // Update display and sliders
                    update_display();
                    update_sliders();

                    // Update read-only Z values
                    if (overlay_) {
                        lv_obj_t* z_vel_row = lv_obj_find_by_name(overlay_, "row_max_z_velocity");
                        if (z_vel_row) {
                            lv_obj_t* value = lv_obj_find_by_name(z_vel_row, "value");
                            if (value) {
                                char buf[32];
                                snprintf(buf, sizeof(buf), "%.0f mm/s", limits.max_z_velocity);
                                lv_label_set_text(value, buf);
                            }
                        }
                        lv_obj_t* z_accel_row = lv_obj_find_by_name(overlay_, "row_max_z_accel");
                        if (z_accel_row) {
                            lv_obj_t* value = lv_obj_find_by_name(z_accel_row, "value");
                            if (value) {
                                char buf[32];
                                snprintf(buf, sizeof(buf), "%.0f mm/s²", limits.max_z_accel);
                                lv_label_set_text(value, buf);
                            }
                        }
                    }

                    // Push overlay onto navigation history and show it
                    if (overlay_) {
                        ui_nav_push_overlay(overlay_);
                    }
                });
            },
            [this](const MoonrakerError& err) {
                // Capture error by value and defer to main thread for LVGL calls
                ui_queue_update([this, err]() {
                    spdlog::error("[{}] Failed to get machine limits: {}", get_name(), err.message);
                    ui_toast_show(ToastSeverity::ERROR, "Failed to get limits", 2000);
                });
            });
    } else {
        // No API - show overlay with default values
        spdlog::warn("[{}] No API available, showing defaults", get_name());
        if (overlay_) {
            ui_nav_push_overlay(overlay_);
        }
    }
}

// ============================================================================
// DISPLAY UPDATES
// ============================================================================

void MachineLimitsOverlay::update_display() {
    // Update max velocity display
    snprintf(velocity_buf_, sizeof(velocity_buf_), "%.0f mm/s", current_limits_.max_velocity);
    lv_subject_copy_string(&max_velocity_display_subject_, velocity_buf_);

    // Update max accel display
    snprintf(accel_buf_, sizeof(accel_buf_), "%.0f mm/s²", current_limits_.max_accel);
    lv_subject_copy_string(&max_accel_display_subject_, accel_buf_);

    // Update accel to decel display
    snprintf(a2d_buf_, sizeof(a2d_buf_), "%.0f mm/s²", current_limits_.max_accel_to_decel);
    lv_subject_copy_string(&accel_to_decel_display_subject_, a2d_buf_);

    // Update square corner velocity display
    snprintf(scv_buf_, sizeof(scv_buf_), "%.0f mm/s", current_limits_.square_corner_velocity);
    lv_subject_copy_string(&square_corner_velocity_display_subject_, scv_buf_);
}

void MachineLimitsOverlay::update_sliders() {
    if (!overlay_) {
        return;
    }

    // Update max velocity slider
    lv_obj_t* vel_slider = lv_obj_find_by_name(overlay_, "max_velocity_slider");
    if (vel_slider) {
        lv_slider_set_value(vel_slider, static_cast<int>(current_limits_.max_velocity),
                            LV_ANIM_OFF);
    }

    // Update max accel slider
    lv_obj_t* accel_slider = lv_obj_find_by_name(overlay_, "max_accel_slider");
    if (accel_slider) {
        lv_slider_set_value(accel_slider, static_cast<int>(current_limits_.max_accel), LV_ANIM_OFF);
    }

    // Update accel to decel slider
    lv_obj_t* a2d_slider = lv_obj_find_by_name(overlay_, "accel_to_decel_slider");
    if (a2d_slider) {
        lv_slider_set_value(a2d_slider, static_cast<int>(current_limits_.max_accel_to_decel),
                            LV_ANIM_OFF);
    }

    // Update square corner velocity slider
    lv_obj_t* scv_slider = lv_obj_find_by_name(overlay_, "square_corner_velocity_slider");
    if (scv_slider) {
        lv_slider_set_value(scv_slider, static_cast<int>(current_limits_.square_corner_velocity),
                            LV_ANIM_OFF);
    }
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void MachineLimitsOverlay::handle_velocity_changed(int value) {
    current_limits_.max_velocity = static_cast<double>(value);
    snprintf(velocity_buf_, sizeof(velocity_buf_), "%d mm/s", value);
    lv_subject_copy_string(&max_velocity_display_subject_, velocity_buf_);
}

void MachineLimitsOverlay::handle_accel_changed(int value) {
    current_limits_.max_accel = static_cast<double>(value);
    snprintf(accel_buf_, sizeof(accel_buf_), "%d mm/s²", value);
    lv_subject_copy_string(&max_accel_display_subject_, accel_buf_);
}

void MachineLimitsOverlay::handle_a2d_changed(int value) {
    current_limits_.max_accel_to_decel = static_cast<double>(value);
    snprintf(a2d_buf_, sizeof(a2d_buf_), "%d mm/s²", value);
    lv_subject_copy_string(&accel_to_decel_display_subject_, a2d_buf_);
}

void MachineLimitsOverlay::handle_scv_changed(int value) {
    current_limits_.square_corner_velocity = static_cast<double>(value);
    snprintf(scv_buf_, sizeof(scv_buf_), "%d mm/s", value);
    lv_subject_copy_string(&square_corner_velocity_display_subject_, scv_buf_);
}

void MachineLimitsOverlay::handle_reset() {
    spdlog::info("[{}] Resetting limits to original values", get_name());
    current_limits_ = original_limits_;
    update_display();
    update_sliders();
}

void MachineLimitsOverlay::handle_apply() {
    spdlog::info("[{}] Applying machine limits: vel={}, accel={}, a2d={}, scv={}", get_name(),
                 current_limits_.max_velocity, current_limits_.max_accel,
                 current_limits_.max_accel_to_decel, current_limits_.square_corner_velocity);

    if (!api_) {
        ui_toast_show(ToastSeverity::ERROR, "No printer connection", 2000);
        return;
    }

    api_->set_machine_limits(
        current_limits_,
        [this]() {
            // Defer to main thread for LVGL calls
            ui_queue_update([this]() {
                spdlog::info("[{}] Machine limits applied successfully", get_name());
                ui_toast_show(ToastSeverity::SUCCESS, "Limits applied", 2000);
                // Update original to prevent reset from reverting
                original_limits_ = current_limits_;
            });
        },
        [this](const MoonrakerError& err) {
            // Capture error by value and defer to main thread for LVGL calls
            ui_queue_update([this, err]() {
                spdlog::error("[{}] Failed to apply machine limits: {}", get_name(), err.message);
                ui_toast_show(ToastSeverity::ERROR, "Failed to apply limits", 2000);
            });
        });
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void MachineLimitsOverlay::on_velocity_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[MachineLimitsOverlay] on_velocity_changed");
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int value = lv_slider_get_value(slider);
    get_machine_limits_overlay().handle_velocity_changed(value);
    LVGL_SAFE_EVENT_CB_END();
}

void MachineLimitsOverlay::on_accel_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[MachineLimitsOverlay] on_accel_changed");
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int value = lv_slider_get_value(slider);
    get_machine_limits_overlay().handle_accel_changed(value);
    LVGL_SAFE_EVENT_CB_END();
}

void MachineLimitsOverlay::on_a2d_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[MachineLimitsOverlay] on_a2d_changed");
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int value = lv_slider_get_value(slider);
    get_machine_limits_overlay().handle_a2d_changed(value);
    LVGL_SAFE_EVENT_CB_END();
}

void MachineLimitsOverlay::on_scv_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[MachineLimitsOverlay] on_scv_changed");
    auto* slider = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    int value = lv_slider_get_value(slider);
    get_machine_limits_overlay().handle_scv_changed(value);
    LVGL_SAFE_EVENT_CB_END();
}

void MachineLimitsOverlay::on_reset(lv_event_t* /*e*/) {
    LVGL_SAFE_EVENT_CB_BEGIN("[MachineLimitsOverlay] on_reset");
    get_machine_limits_overlay().handle_reset();
    LVGL_SAFE_EVENT_CB_END();
}

void MachineLimitsOverlay::on_apply(lv_event_t* /*e*/) {
    LVGL_SAFE_EVENT_CB_BEGIN("[MachineLimitsOverlay] on_apply");
    get_machine_limits_overlay().handle_apply();
    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::settings
