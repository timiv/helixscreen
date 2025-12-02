// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl.h"
#include "moonraker_api.h"
#include "printer_state.h"
#include "ui_observer_guard.h"

#include <string>
#include <unordered_set>

/**
 * @brief Floating emergency stop button overlay
 *
 * Displays a prominent red emergency stop button during active prints
 * (PRINTING or PAUSED states) on relevant panels (home, print_status,
 * controls, motion). The button triggers an immediate M112 emergency
 * stop command via Moonraker.
 *
 * Features:
 * - Single-tap activation (default) or confirmation dialog (optional setting)
 * - Automatic visibility based on print state and current panel
 * - Z-ordered above all panel content
 * - Visual feedback via toast notifications
 *
 * Usage:
 *   // In main.cpp after LVGL and subjects initialized:
 *   EmergencyStopOverlay::instance().init(printer_state, api, settings);
 *   EmergencyStopOverlay::instance().create();
 *
 * @see KlipperRecoveryDialog for post-shutdown recovery flow
 */
class EmergencyStopOverlay {
  public:
    /**
     * @brief Get singleton instance
     * @return Reference to the global EmergencyStopOverlay instance
     */
    static EmergencyStopOverlay& instance();

    /**
     * @brief Initialize with dependencies
     *
     * Must be called before create(). Sets up references to printer state
     * and API for operation.
     *
     * @param printer_state Reference to PrinterState for print job state
     * @param api Pointer to MoonrakerAPI for emergency_stop() calls
     */
    void init(PrinterState& printer_state, MoonrakerAPI* api);

    /**
     * @brief Initialize subjects for XML binding
     *
     * Registers the estop_visible subject used by XML binding.
     * Must be called during subject initialization phase (before XML creation).
     */
    void init_subjects();

    /**
     * @brief Create the floating button widget
     *
     * Creates the emergency_stop_button XML component on the active screen.
     * The button floats above all panels at z-index top.
     *
     * Must be called after:
     * - init() with valid dependencies
     * - init_subjects() for XML binding
     * - XML components registered
     */
    void create();

    /**
     * @brief Notify overlay of panel change
     *
     * Called by navigation system when active panel changes.
     * Updates visibility based on whether current panel should show E-Stop.
     *
     * @param panel_name Name of the newly active panel (e.g., "home_panel")
     */
    void on_panel_changed(const std::string& panel_name);

    /**
     * @brief Force visibility update
     *
     * Recalculates and applies visibility based on current print state
     * and panel. Called automatically by state observers, but can be
     * called manually if needed.
     */
    void update_visibility();

    /**
     * @brief Set whether confirmation dialog is required
     *
     * When enabled, clicking E-Stop shows a confirmation dialog before
     * executing. When disabled (default), E-Stop executes immediately.
     *
     * @param require true to require confirmation, false for immediate action
     */
    void set_require_confirmation(bool require);

  private:
    EmergencyStopOverlay() = default;
    ~EmergencyStopOverlay() = default;

    // Non-copyable
    EmergencyStopOverlay(const EmergencyStopOverlay&) = delete;
    EmergencyStopOverlay& operator=(const EmergencyStopOverlay&) = delete;

    // Dependencies (set via init())
    PrinterState* printer_state_ = nullptr;
    MoonrakerAPI* api_ = nullptr;

    // Confirmation requirement (set via set_require_confirmation())
    bool require_confirmation_ = false;

    // Widget references
    lv_obj_t* button_ = nullptr;
    lv_obj_t* confirmation_dialog_ = nullptr;
    lv_obj_t* recovery_dialog_ = nullptr;

    // Visibility subject (1=visible, 0=hidden)
    lv_subject_t estop_visible_;
    bool subjects_initialized_ = false;

    // Current panel tracking
    std::string current_panel_;

    // Panels where E-Stop should be visible
    static const std::unordered_set<std::string> VISIBLE_PANELS;

    // State observers
    ObserverGuard print_state_observer_;
    ObserverGuard klippy_state_observer_;

    // Event handlers
    void handle_click();
    void execute_emergency_stop();
    void show_confirmation_dialog();
    void dismiss_confirmation_dialog();
    void show_recovery_dialog();
    void dismiss_recovery_dialog();
    void restart_klipper();
    void firmware_restart();

    // Static callbacks
    static void emergency_stop_clicked(lv_event_t* e);
    static void estop_dialog_cancel_clicked(lv_event_t* e);
    static void estop_dialog_confirm_clicked(lv_event_t* e);
    static void recovery_restart_klipper_clicked(lv_event_t* e);
    static void recovery_firmware_restart_clicked(lv_event_t* e);
    static void recovery_dismiss_clicked(lv_event_t* e);
    static void advanced_estop_clicked(lv_event_t* e);
    static void advanced_restart_klipper_clicked(lv_event_t* e);
    static void advanced_firmware_restart_clicked(lv_event_t* e);
    static void print_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject);
    static void klippy_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject);
};
