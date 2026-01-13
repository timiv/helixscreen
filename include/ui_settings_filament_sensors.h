// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_settings_filament_sensors.h
 * @brief Filament Sensor Settings overlay - configure filament sensors and roles
 *
 * This overlay allows users to configure:
 * - Master enable/disable toggle for all sensor monitoring
 * - Per-sensor role assignment (None, Runout, Toolhead, Entry)
 * - Per-sensor enable/disable toggle
 *
 * Sensor discovery is handled by FilamentSensorManager. This overlay provides
 * the UI for user configuration only.
 *
 * @pattern Overlay (lazy init, dynamic row creation)
 * @threading Main thread only
 *
 * @see FilamentSensorManager for sensor discovery and state management
 * @see Config for persistence
 */

#pragma once

#include "lvgl/lvgl.h"

#include <string>
#include <vector>

namespace helix::settings {

/**
 * @class FilamentSensorSettingsOverlay
 * @brief Overlay for configuring filament sensor roles and enable states
 *
 * This overlay provides:
 * - Master toggle: Enable/disable all filament monitoring
 * - Sensor list: Dynamic rows for each discovered sensor
 * - Per-sensor role dropdown: None/Runout/Toolhead/Entry
 * - Per-sensor enable toggle
 *
 * ## State Management:
 *
 * Configuration is managed by FilamentSensorManager singleton.
 * Changes are immediately persisted via mgr.save_config().
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::settings::get_filament_sensor_settings_overlay();
 * overlay.show(parent_screen);  // Creates overlay if needed, populates list, shows
 * @endcode
 *
 * ## Note on Dynamic Row Creation:
 *
 * Unlike declarative XML callbacks, sensor rows are created dynamically at runtime
 * using lv_xml_create(). This requires using lv_obj_add_event_cb() for the
 * dropdown and toggle callbacks, which is an acceptable exception to the
 * declarative UI rule.
 */
class FilamentSensorSettingsOverlay {
  public:
    /**
     * @brief Default constructor
     */
    FilamentSensorSettingsOverlay();

    /**
     * @brief Destructor
     */
    ~FilamentSensorSettingsOverlay();

    // Non-copyable
    FilamentSensorSettingsOverlay(const FilamentSensorSettingsOverlay&) = delete;
    FilamentSensorSettingsOverlay& operator=(const FilamentSensorSettingsOverlay&) = delete;

    //
    // === Initialization ===
    //

    /**
     * @brief Register event callbacks with lv_xml system
     *
     * Registers callbacks for:
     * - on_filament_sensors_clicked (entry point from SettingsPanel)
     * - on_filament_master_toggle_changed (master enable toggle)
     */
    void register_callbacks();

    //
    // === UI Creation ===
    //

    /**
     * @brief Create the overlay UI (called lazily)
     *
     * @param parent Parent widget to attach overlay to (usually screen)
     * @return Root object of overlay, or nullptr on failure
     */
    lv_obj_t* create(lv_obj_t* parent);

    /**
     * @brief Show the overlay (populates sensor list first)
     *
     * This method:
     * 1. Ensures overlay is created
     * 2. Updates sensor count label
     * 3. Populates sensor list with current sensors
     * 4. Pushes overlay onto navigation stack
     *
     * @param parent_screen The parent screen for overlay creation
     */
    void show(lv_obj_t* parent_screen);

    //
    // === Accessors ===
    //

    /**
     * @brief Get root overlay widget
     * @return Root widget, or nullptr if not created
     */
    lv_obj_t* get_root() const {
        return overlay_;
    }

    /**
     * @brief Check if overlay has been created
     * @return true if create() was called successfully
     */
    bool is_created() const {
        return overlay_ != nullptr;
    }

    /**
     * @brief Get human-readable overlay name
     * @return "Filament Sensors"
     */
    const char* get_name() const {
        return "Filament Sensors";
    }

    //
    // === Event Handlers (public for static callbacks) ===
    //

    /**
     * @brief Handle master enable toggle change
     * @param enabled New master enable state
     */
    void handle_master_toggle_changed(bool enabled);

  private:
    //
    // === Internal Methods ===
    //

    /**
     * @brief Populate sensor list from FilamentSensorManager
     *
     * Creates a filament_sensor_row component for each discovered sensor.
     * Clears existing rows first to allow refresh.
     */
    void populate_sensor_list();

    /**
     * @brief Update sensor count label
     */
    void update_sensor_count_label();

    //
    // === State ===
    //

    lv_obj_t* overlay_{nullptr};
    lv_obj_t* parent_screen_{nullptr};

    //
    // === Static Callbacks ===
    //

    static void on_filament_master_toggle_changed(lv_event_t* e);
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton FilamentSensorSettingsOverlay
 */
FilamentSensorSettingsOverlay& get_filament_sensor_settings_overlay();

} // namespace helix::settings
