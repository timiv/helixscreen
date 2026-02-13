// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_settings_led.cpp
 * @brief Implementation of LedSettingsOverlay
 */

#include "ui_settings_led.h"

#include "ui_event_safety.h"
#include "ui_led_chip_factory.h"
#include "ui_nav_manager.h"

#include "device_display_name.h"
#include "led/led_auto_state.h"
#include "led/led_controller.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::settings {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<LedSettingsOverlay> g_led_settings_overlay;

LedSettingsOverlay& get_led_settings_overlay() {
    if (!g_led_settings_overlay) {
        g_led_settings_overlay = std::make_unique<LedSettingsOverlay>();
        StaticPanelRegistry::instance().register_destroy("LedSettingsOverlay",
                                                         []() { g_led_settings_overlay.reset(); });
    }
    return *g_led_settings_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

LedSettingsOverlay::LedSettingsOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

LedSettingsOverlay::~LedSettingsOverlay() {
    spdlog::trace("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void LedSettingsOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // No dedicated subjects needed - toggles are synced imperatively in on_activate()
    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void LedSettingsOverlay::register_callbacks() {
    lv_xml_register_event_cb(nullptr, "on_led_on_at_start_changed", on_led_on_at_start_changed);
    lv_xml_register_event_cb(nullptr, "on_auto_state_changed", on_auto_state_changed);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* LedSettingsOverlay::create(lv_obj_t* parent) {
    if (overlay_root_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_root_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    overlay_root_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "led_settings_overlay", nullptr));
    if (!overlay_root_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_root_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_root_;
}

void LedSettingsOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    parent_screen_ = parent_screen;

    // Ensure subjects and callbacks are initialized
    if (!subjects_initialized_) {
        init_subjects();
        register_callbacks();
    }

    // Lazy create overlay
    if (!overlay_root_ && parent_screen_) {
        create(parent_screen_);
    }

    if (!overlay_root_) {
        spdlog::error("[{}] Cannot show - overlay not created", get_name());
        return;
    }

    // Register for lifecycle callbacks
    NavigationManager::instance().register_overlay_instance(overlay_root_, this);

    // Push onto navigation stack (on_activate will initialize widgets)
    ui_nav_push_overlay(overlay_root_);
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void LedSettingsOverlay::on_activate() {
    OverlayBase::on_activate();

    populate_led_chips();
    init_led_on_at_start_toggle();
    init_auto_state_toggle();
    populate_macro_devices();
}

void LedSettingsOverlay::on_deactivate() {
    OverlayBase::on_deactivate();
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void LedSettingsOverlay::init_led_on_at_start_toggle() {
    if (!overlay_root_)
        return;

    lv_obj_t* row = lv_obj_find_by_name(overlay_root_, "row_led_on_at_start");
    if (row) {
        lv_obj_t* toggle = lv_obj_find_by_name(row, "toggle");
        if (toggle) {
            bool enabled = helix::led::LedController::instance().get_led_on_at_start();
            if (enabled) {
                lv_obj_add_state(toggle, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(toggle, LV_STATE_CHECKED);
            }
            spdlog::trace("[{}]   LED on at start toggle: {}", get_name(), enabled ? "ON" : "OFF");
        }
    }
}

void LedSettingsOverlay::init_auto_state_toggle() {
    if (!overlay_root_)
        return;

    lv_obj_t* row = lv_obj_find_by_name(overlay_root_, "row_auto_state_enabled");
    if (row) {
        lv_obj_t* toggle = lv_obj_find_by_name(row, "toggle");
        if (toggle) {
            bool enabled = helix::led::LedAutoState::instance().is_enabled();
            if (enabled) {
                lv_obj_add_state(toggle, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(toggle, LV_STATE_CHECKED);
            }
            spdlog::trace("[{}]   Auto state toggle: {}", get_name(), enabled ? "ON" : "OFF");
        }
    }
}

void LedSettingsOverlay::populate_macro_devices() {
    if (!overlay_root_)
        return;

    lv_obj_t* container = lv_obj_find_by_name(overlay_root_, "macro_devices_container");
    if (!container)
        return;

    // Clear existing children
    lv_obj_clean(container);

    auto& led_ctrl = helix::led::LedController::instance();
    const auto& macros = led_ctrl.configured_macros();

    if (macros.empty()) {
        // Show "No macro devices detected" message
        const char* attrs[] = {
            "label",     "No macro devices detected",
            "label_tag", "No macro devices detected",
            "icon",      "code_braces",
            "value",     "—",
            nullptr,
        };
        lv_xml_create(container, "setting_info_row", attrs);
        spdlog::debug("[{}] No macro devices to display", get_name());
        return;
    }

    for (const auto& macro : macros) {
        std::string type_label;
        switch (macro.type) {
        case helix::led::MacroLedType::ON_OFF:
            type_label = "On/Off";
            break;
        case helix::led::MacroLedType::TOGGLE:
            type_label = "Toggle";
            break;
        case helix::led::MacroLedType::PRESET:
            type_label = "Preset";
            break;
        }

        const char* attrs[] = {
            "label", macro.display_name.c_str(), "icon",  "code_braces",
            "value", type_label.c_str(),         nullptr,
        };
        lv_xml_create(container, "setting_info_row", attrs);
    }

    spdlog::debug("[{}] Macro devices populated ({} macros)", get_name(), macros.size());
}

// ============================================================================
// LED CHIP SELECTION
// ============================================================================

void LedSettingsOverlay::populate_led_chips() {
    if (!overlay_root_)
        return;

    lv_obj_t* led_chip_row = lv_obj_find_by_name(overlay_root_, "row_led_select");
    if (!led_chip_row)
        return;

    lv_obj_t* chip_container = lv_obj_find_by_name(led_chip_row, "chip_container");
    if (!chip_container) {
        spdlog::warn("[{}] LED chip row found but no chip_container", get_name());
        return;
    }

    // Clear existing chips
    lv_obj_clean(chip_container);

    // Source LED list from LedController backends (native + WLED)
    auto& led_ctrl = helix::led::LedController::instance();
    discovered_leds_.clear();
    for (const auto& strip : led_ctrl.native().strips()) {
        discovered_leds_.push_back(strip.id);
    }
    for (const auto& strip : led_ctrl.wled().strips()) {
        discovered_leds_.push_back(strip.id);
    }

    // Load selected LEDs from LedController
    selected_leds_.clear();
    for (const auto& strip_id : led_ctrl.selected_strips()) {
        selected_leds_.insert(strip_id);
    }

    // Create chips for each discovered LED
    for (const auto& led : discovered_leds_) {
        bool selected = selected_leds_.count(led) > 0;
        std::string display_name = helix::get_display_name(led, helix::DeviceType::LED);

        helix::ui::create_led_chip(
            chip_container, led, display_name, selected,
            [this](const std::string& led_name) { handle_led_chip_clicked(led_name); });
    }

    spdlog::debug("[{}] LED chips populated ({} LEDs, {} selected)", get_name(),
                  discovered_leds_.size(), selected_leds_.size());
}

void LedSettingsOverlay::handle_led_chip_clicked(const std::string& led_name) {
    // Toggle selection
    if (selected_leds_.count(led_name) > 0) {
        selected_leds_.erase(led_name);
        spdlog::info("[{}] LED deselected: {}", get_name(), led_name);
    } else {
        selected_leds_.insert(led_name);
        spdlog::info("[{}] LED selected: {}", get_name(), led_name);
    }

    // Save via LedController
    std::vector<std::string> strips_vec(selected_leds_.begin(), selected_leds_.end());
    helix::led::LedController::instance().set_selected_strips(strips_vec);
    helix::led::LedController::instance().save_config();

    // Rebuild chips to update visual state
    populate_led_chips();
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void LedSettingsOverlay::handle_led_on_at_start_changed(bool enabled) {
    spdlog::info("[{}] LED on at start toggled: {}", get_name(), enabled ? "ON" : "OFF");
    helix::led::LedController::instance().set_led_on_at_start(enabled);
    helix::led::LedController::instance().save_config();
}

void LedSettingsOverlay::handle_auto_state_changed(bool enabled) {
    spdlog::info("[{}] Auto state toggled: {}", get_name(), enabled ? "ON" : "OFF");
    helix::led::LedAutoState::instance().set_enabled(enabled);
    helix::led::LedAutoState::instance().save_config();
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void LedSettingsOverlay::on_led_on_at_start_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[LedSettingsOverlay] on_led_on_at_start_changed");
    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    get_led_settings_overlay().handle_led_on_at_start_changed(enabled);
    LVGL_SAFE_EVENT_CB_END();
}

void LedSettingsOverlay::on_auto_state_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[LedSettingsOverlay] on_auto_state_changed");
    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);
    get_led_settings_overlay().handle_auto_state_changed(enabled);
    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::settings
