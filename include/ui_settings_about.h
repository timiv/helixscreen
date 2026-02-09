// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_settings_about.h
 * @brief About overlay - version info, update controls, system component versions
 *
 * Shows Current Version (with 7-tap secret), Check for Updates, Install Update,
 * Update Channel, Klipper/Moonraker/OS versions, Print Hours, and MCU rows.
 *
 * @pattern Overlay (lazy init)
 * @threading Main thread only
 *
 * @see DisplaySettingsOverlay for pattern reference
 */

#pragma once

#include "lvgl/lvgl.h"
#include "overlay_base.h"

namespace helix::settings {

/**
 * @class AboutOverlay
 * @brief Overlay for version info, updates, and system component versions
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::settings::get_about_overlay();
 * overlay.show(parent_screen);
 * @endcode
 */
class AboutOverlay : public OverlayBase {
  public:
    AboutOverlay();
    ~AboutOverlay() override;

    //
    // === OverlayBase Interface ===
    //

    void init_subjects() override;
    void register_callbacks() override;

    const char* get_name() const override {
        return "About";
    }

    void on_activate() override;

    lv_obj_t* create(lv_obj_t* parent) override;

    /**
     * @brief Show the overlay
     *
     * Lazy-creates overlay, initializes widgets, pushes onto nav stack.
     *
     * @param parent_screen The parent screen for overlay creation
     */
    void show(lv_obj_t* parent_screen);

    bool is_created() const {
        return overlay_root_ != nullptr;
    }

  private:
    //
    // === Internal Methods ===
    //

    void bind_version_subjects();
    void populate_mcu_rows();

    //
    // === State ===
    //

    lv_observer_t* klipper_version_observer_ = nullptr;
    lv_observer_t* moonraker_version_observer_ = nullptr;
    lv_observer_t* os_version_observer_ = nullptr;
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton AboutOverlay
 */
AboutOverlay& get_about_overlay();

} // namespace helix::settings
