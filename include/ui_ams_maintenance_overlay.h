// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_maintenance_overlay.h
 * @brief AMS Maintenance sub-panel overlay
 *
 * This overlay provides maintenance actions for the AMS system:
 * - Home: Reset AMS to home position
 * - Recover: Attempt error recovery
 * - Abort: Cancel current operation
 *
 * @pattern Overlay (lazy init, singleton)
 * @threading Main thread only
 */

#pragma once

#include "overlay_base.h"

#include <lvgl/lvgl.h>

#include <memory>

namespace helix::ui {

/**
 * @class AmsMaintenanceOverlay
 * @brief Overlay for AMS maintenance actions
 *
 * This overlay provides buttons to perform maintenance operations:
 * - Home: Resets the AMS system to its home position
 * - Recover: Attempts to recover from error states
 * - Abort: Cancels the current operation
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::ui::get_ams_maintenance_overlay();
 * if (!overlay.are_subjects_initialized()) {
 *     overlay.init_subjects();
 *     overlay.register_callbacks();
 * }
 * overlay.show(parent_screen);
 * @endcode
 */
class AmsMaintenanceOverlay : public OverlayBase {
  public:
    /**
     * @brief Default constructor
     */
    AmsMaintenanceOverlay();

    /**
     * @brief Destructor
     */
    ~AmsMaintenanceOverlay() override;

    // Non-copyable
    AmsMaintenanceOverlay(const AmsMaintenanceOverlay&) = delete;
    AmsMaintenanceOverlay& operator=(const AmsMaintenanceOverlay&) = delete;

    //
    // === OverlayBase Interface ===
    //

    /**
     * @brief Initialize subjects for reactive binding
     *
     * Registers subjects for:
     * - ams_maintenance_status: Current action status text
     */
    void init_subjects() override;

    /**
     * @brief Register event callbacks with lv_xml system
     *
     * Registers callbacks for home, recover, and abort buttons.
     */
    void register_callbacks() override;

    /**
     * @brief Create the overlay UI (called lazily)
     *
     * @param parent Parent widget to attach overlay to (usually screen)
     * @return Root object of overlay, or nullptr on failure
     */
    lv_obj_t* create(lv_obj_t* parent) override;

    /**
     * @brief Get human-readable overlay name
     * @return "Maintenance"
     */
    const char* get_name() const override {
        return "Maintenance";
    }

    //
    // === Public API ===
    //

    /**
     * @brief Show the overlay
     *
     * This method:
     * 1. Ensures overlay is created (lazy init)
     * 2. Updates current status display
     * 3. Pushes overlay onto navigation stack
     *
     * @param parent_screen The parent screen for overlay creation
     */
    void show(lv_obj_t* parent_screen);

    /**
     * @brief Refresh status display
     *
     * Updates the status label with current AMS action state.
     */
    void refresh();

  private:
    //
    // === Internal Methods ===
    //

    /**
     * @brief Update status label from current backend state
     */
    void update_status();

    /**
     * @brief Convert AmsAction enum to human-readable string
     *
     * @param action The action to convert
     * @return Human-readable status string
     */
    static const char* action_to_string(int action);

    //
    // === Static Callbacks ===
    //

    /**
     * @brief Callback for Home button click
     *
     * Calls backend->reset() to home the AMS system.
     */
    static void on_home_clicked(lv_event_t* e);

    /**
     * @brief Callback for Recover button click
     *
     * Calls backend->recover() to attempt error recovery.
     */
    static void on_recover_clicked(lv_event_t* e);

    /**
     * @brief Callback for Abort button click
     *
     * Calls backend->cancel() to abort the current operation.
     */
    static void on_abort_clicked(lv_event_t* e);

    //
    // === State ===
    //

    /// Alias for overlay_root_ to match existing pattern
    lv_obj_t*& overlay_ = overlay_root_;

    /// Subject for status text display
    lv_subject_t status_subject_;

    /// Buffer for status text
    char status_buf_[64] = {};
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton AmsMaintenanceOverlay
 */
AmsMaintenanceOverlay& get_ams_maintenance_overlay();

} // namespace helix::ui
