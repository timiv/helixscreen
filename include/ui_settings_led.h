// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_settings_led.h
 * @brief LED Settings overlay - startup preference, auto state, macro devices
 *
 * This overlay allows users to configure:
 * - LED on at start preference
 * - Automatic LED state control (color changes based on printer state)
 * - View auto-detected macro LED devices
 *
 * @pattern Overlay (lazy init)
 * @threading Main thread only
 *
 * @see LedController for LED config persistence
 * @see LedAutoState for automatic state control
 */

#pragma once

#include "lvgl/lvgl.h"
#include "overlay_base.h"

#include <set>
#include <string>
#include <vector>

namespace helix::settings {

/**
 * @class LedSettingsOverlay
 * @brief Overlay for configuring LED-related settings
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::settings::get_led_settings_overlay();
 * overlay.show(parent_screen);
 * @endcode
 */
class LedSettingsOverlay : public OverlayBase {
  public:
    LedSettingsOverlay();
    ~LedSettingsOverlay() override;

    // Non-copyable
    LedSettingsOverlay(const LedSettingsOverlay&) = delete;
    LedSettingsOverlay& operator=(const LedSettingsOverlay&) = delete;

    //
    // === OverlayBase Interface ===
    //

    void init_subjects() override;
    void register_callbacks() override;

    const char* get_name() const override {
        return "LED Settings";
    }

    void on_activate() override;
    void on_deactivate() override;

    //
    // === UI Creation ===
    //

    lv_obj_t* create(lv_obj_t* parent) override;
    void show(lv_obj_t* parent_screen);

    bool is_created() const {
        return overlay_root_ != nullptr;
    }

    //
    // === Event Handlers (public for static callbacks) ===
    //

    void handle_led_on_at_start_changed(bool enabled);
    void handle_auto_state_changed(bool enabled);

  private:
    //
    // === Internal Methods ===
    //

    void init_led_on_at_start_toggle();
    void init_auto_state_toggle();
    void populate_macro_devices();
    void populate_led_chips();
    void handle_led_chip_clicked(const std::string& led_name);

    // LED chip selection state
    std::vector<std::string> discovered_leds_;
    std::set<std::string> selected_leds_;

    //
    // === Static Callbacks ===
    //

    static void on_led_on_at_start_changed(lv_event_t* e);
    static void on_auto_state_changed(lv_event_t* e);
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton LedSettingsOverlay
 */
LedSettingsOverlay& get_led_settings_overlay();

} // namespace helix::settings
