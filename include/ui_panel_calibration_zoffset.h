// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_observer_guard.h"

#include "lvgl/lvgl.h"
#include "overlay_base.h"
#include "subject_managed_panel.h"

#include <string>

class MoonrakerClient;
class PrinterState;

/**
 * @file ui_panel_calibration_zoffset.h
 * @brief Z-Offset calibration panel using PROBE_CALIBRATE workflow
 *
 * Interactive panel that guides the user through the paper test calibration
 * process. Uses Klipper's PROBE_CALIBRATE, TESTZ, ACCEPT, and ABORT commands.
 *
 * ## State Machine:
 * - IDLE: Shows instructions and Start button
 * - PROBING: Waiting for PROBE_CALIBRATE to complete (homes + probes)
 * - ADJUSTING: User adjusts Z with paper test (+/- buttons)
 * - SAVING: ACCEPT was pressed, saving config (Klipper restarts)
 * - COMPLETE: Calibration successful
 * - ERROR: Something went wrong
 *
 * ## Usage:
 * ```cpp
 * auto& overlay = get_global_zoffset_cal_panel();
 * if (!overlay.get_root()) {
 *     overlay.init_subjects();
 *     overlay.set_client(get_moonraker_client());
 *     overlay.create(parent_screen);
 * }
 * overlay.show();
 * ```
 */
class ZOffsetCalibrationPanel : public OverlayBase {
  public:
    /**
     * @brief Calibration state machine states
     */
    enum class State {
        IDLE,      ///< Ready to start, showing instructions
        PROBING,   ///< PROBE_CALIBRATE running
        ADJUSTING, ///< Interactive Z adjustment phase
        SAVING,    ///< ACCEPT sent, waiting for SAVE_CONFIG
        COMPLETE,  ///< Calibration finished successfully
        ERROR      ///< Error occurred
    };

    ZOffsetCalibrationPanel();
    ~ZOffsetCalibrationPanel() override;

    //
    // === OverlayBase Interface ===
    //

    /**
     * @brief Initialize LVGL subjects for XML data binding
     *
     * Call once at startup before any panel instances are created.
     * Registers the zoffset_cal_state subject and all XML event callbacks.
     */
    void init_subjects() override;

    /**
     * @brief Create overlay UI from XML
     *
     * @param parent Parent screen widget to attach overlay to
     * @return Root object of overlay, or nullptr on failure
     */
    lv_obj_t* create(lv_obj_t* parent) override;

    /**
     * @brief Get human-readable overlay name
     * @return "Z-Offset Calibration"
     */
    const char* get_name() const override {
        return "Z-Offset Calibration";
    }

    /**
     * @brief Called when overlay becomes visible
     *
     * Resets state to IDLE.
     */
    void on_activate() override;

    /**
     * @brief Called when overlay is being hidden
     *
     * Aborts calibration if in progress.
     */
    void on_deactivate() override;

    /**
     * @brief Clean up resources for async-safe destruction
     */
    void cleanup() override;

    //
    // === Public API ===
    //

    /**
     * @brief Show overlay panel
     *
     * Pushes overlay onto navigation stack and registers with NavigationManager.
     * on_activate() will be called automatically after animation completes.
     */
    void show();

    /**
     * @brief Set the Moonraker client for G-code commands
     *
     * @param client MoonrakerClient for sending commands
     */
    void set_client(MoonrakerClient* client) {
        client_ = client;
    }

    /**
     * @brief Get current calibration state
     * @return Current State
     */
    State get_state() const {
        return state_;
    }

    /**
     * @brief Update Z position display (called from external state updates)
     * @param z_position Current Z position from printer state
     */
    void update_z_position(float z_position);

    /**
     * @brief Handle calibration completion/error from Moonraker
     * @param success true if calibration completed successfully
     * @param message Status message
     */
    void on_calibration_result(bool success, const std::string& message);

    // Static trampolines for XML event_cb (must be public for registration)
    static void on_start_clicked(lv_event_t* e);
    static void on_z_adjust(lv_event_t* e);
    static void on_accept_clicked(lv_event_t* e);
    static void on_abort_clicked(lv_event_t* e);
    static void on_done_clicked(lv_event_t* e);
    static void on_retry_clicked(lv_event_t* e);

  private:
    // Client reference
    // Note: overlay_root_ inherited from OverlayBase
    lv_obj_t* parent_screen_ = nullptr;
    MoonrakerClient* client_ = nullptr;

    // State management
    State state_ = State::IDLE;
    void set_state(State new_state);

    // UI setup (called by create())
    void setup_widgets();

    // Gcode command helpers
    void send_probe_calibrate();
    void send_testz(float delta);
    void send_accept();
    void send_abort();

    // Event handlers
    void handle_start_clicked();
    void handle_z_adjust(float delta);
    void handle_accept_clicked();
    void handle_abort_clicked();
    void handle_done_clicked();
    void handle_retry_clicked();

    // Interactive elements
    lv_obj_t* z_position_display_ = nullptr;
    lv_obj_t* final_offset_label_ = nullptr;
    lv_obj_t* error_message_ = nullptr;

    // Current Z position during calibration
    float current_z_ = 0.0f;
    float final_offset_ = 0.0f;

    // Subject manager for automatic cleanup
    SubjectManager subjects_;

    // Observer guards for manual_probe state changes (RAII cleanup)
    ObserverGuard manual_probe_active_observer_;
    ObserverGuard manual_probe_z_observer_;
};

// Global instance accessor
ZOffsetCalibrationPanel& get_global_zoffset_cal_panel();

// Destroy the global instance (call during shutdown)
void destroy_zoffset_cal_panel();

/**
 * @brief Initialize row click callback for opening from Advanced panel
 *
 * Must be called during app initialization before XML creation.
 * Registers "on_zoffset_row_clicked" callback.
 */
void init_zoffset_row_handler();

/**
 * @brief Initialize event callbacks for the Z-Offset calibration panel
 *
 * Must be called during app initialization before XML creation.
 * Registers all button click callbacks used by calibration_zoffset_panel.xml.
 */
void init_zoffset_event_callbacks();
