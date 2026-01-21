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

#include "filament_sensor_types.h"
#include "lvgl/lvgl.h"
#include "overlay_base.h"

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
class FilamentSensorSettingsOverlay : public OverlayBase {
  public:
    /**
     * @brief Default constructor
     */
    FilamentSensorSettingsOverlay();

    /**
     * @brief Destructor
     */
    ~FilamentSensorSettingsOverlay() override;

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
    void register_callbacks() override;

    //
    // === UI Creation ===
    //

    /**
     * @brief Create the overlay UI (called lazily)
     *
     * @param parent Parent widget to attach overlay to (usually screen)
     * @return Root object of overlay, or nullptr on failure
     */
    lv_obj_t* create(lv_obj_t* parent) override;

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
     * @brief Get human-readable overlay name
     * @return "Filament Sensors"
     */
    const char* get_name() const override {
        return "Filament Sensors";
    }

    /**
     * @brief Initialize subjects (none needed for this overlay)
     */
    void init_subjects() override {}

    /**
     * @brief Called when overlay becomes visible
     *
     * Calls OverlayBase::on_activate() then populates sensor list.
     */
    void on_activate() override;

    /**
     * @brief Called when overlay is being hidden
     *
     * Calls OverlayBase::on_deactivate().
     */
    void on_deactivate() override;

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

    /**
     * @brief Get sensors excluding AMS/multi-material types
     * @return Vector of standalone sensors only
     */
    [[nodiscard]] std::vector<helix::FilamentSensorConfig> get_standalone_sensors() const;

    //
    // === State ===
    //
    // Note: overlay_root_ and parent_screen_ are inherited from OverlayBase

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
