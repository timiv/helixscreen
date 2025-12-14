// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "calibration_types.h" // For InputShaperResult
#include "lvgl/lvgl.h"

#include <array>
#include <string>
#include <vector>

class MoonrakerClient;
class MoonrakerAPI;

/**
 * @file ui_panel_input_shaper.h
 * @brief Input Shaper calibration panel for resonance compensation tuning
 *
 * Interactive panel that guides users through SHAPER_CALIBRATE workflow.
 * Allows measuring resonance on X/Y axes, viewing recommendations, and
 * applying optimal shaper settings to reduce ringing/ghosting.
 *
 * ## State Machine:
 * - IDLE: Shows instructions and buttons to start calibration
 * - MEASURING: Calibration running, show spinner and cancel button
 * - RESULTS: Display recommendations, Apply/Dismiss buttons
 * - ERROR: Something went wrong, retry option
 *
 * ## Klipper Commands Used:
 * - MEASURE_AXES_NOISE: Check accelerometer noise level
 * - SHAPER_CALIBRATE AXIS=X/Y: Run resonance test
 * - SET_INPUT_SHAPER: Apply recommended settings
 * - SAVE_CONFIG: Save settings permanently (restarts Klipper)
 *
 * ## Usage:
 * ```cpp
 * InputShaperPanel& panel = get_global_input_shaper_panel();
 * panel.setup(lv_obj, parent_screen, moonraker_client, moonraker_api);
 * ui_nav_push_overlay(lv_obj);
 * ```
 */
class InputShaperPanel {
  public:
    /**
     * @brief Panel state machine states
     */
    enum class State {
        IDLE,      ///< Ready to start, showing instructions
        MEASURING, ///< SHAPER_CALIBRATE or MEASURE_AXES_NOISE running
        RESULTS,   ///< Showing calibration recommendations
        ERROR      ///< Error occurred
    };

    /**
     * @brief Parsed shaper result for display
     */
    struct ShaperFit {
        std::string type;            ///< Shaper type (zv, mzv, ei, 2hump_ei, 3hump_ei)
        float frequency = 0.0f;      ///< Recommended frequency in Hz
        float vibrations = 0.0f;     ///< Remaining vibrations percentage
        float smoothing = 0.0f;      ///< Smoothing value (lower is better)
        bool is_recommended = false; ///< True if this is the recommended shaper
    };

    InputShaperPanel() = default;
    ~InputShaperPanel();

    /**
     * @brief Setup the panel with event handlers
     *
     * @param panel Root panel object from lv_xml_create()
     * @param parent_screen Parent screen (for navigation)
     * @param client Moonraker client (kept for potential future use)
     * @param api Moonraker API for G-code execution
     */
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen, MoonrakerClient* client,
               MoonrakerAPI* api);

    /**
     * @brief Get current panel state
     * @return Current State
     */
    [[nodiscard]] State get_state() const {
        return state_;
    }

    //
    // === Event Handlers (public for XML event_cb callbacks) ===
    //

    void handle_calibrate_x_clicked();
    void handle_calibrate_y_clicked();
    void handle_measure_noise_clicked();
    void handle_cancel_clicked();
    void handle_apply_clicked();
    void handle_close_clicked();
    void handle_retry_clicked();
    void handle_save_config_clicked();
    void handle_help_clicked();

    /**
     * @brief Initialize subjects for reactive XML bindings
     *
     * Must be called BEFORE XML creation (from register_callbacks).
     * Subject bindings are resolved at XML parse time.
     */
    void init_subjects();

  private:
    bool subjects_initialized_ = false;

    // State management
    State state_ = State::IDLE;
    void set_state(State new_state);

    // Calibration commands
    void start_calibration(char axis);
    void measure_noise();
    void cancel_calibration();
    void apply_recommendation();
    void save_configuration();

    // Result callbacks (from MoonrakerAPI)
    void on_calibration_result(const InputShaperResult& result);
    void on_calibration_error(const std::string& message);

    // UI update helpers
    void populate_results();
    void clear_results();
    void update_status_label(const std::string& text);
    void process_pending_responses(); // Unused - for interface compatibility

    // Widget references
    lv_obj_t* panel_ = nullptr;
    lv_obj_t* parent_screen_ = nullptr;
    MoonrakerClient* client_ = nullptr;
    MoonrakerAPI* api_ = nullptr;

    // Display elements
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* error_message_ = nullptr;
    lv_obj_t* recommendation_label_ = nullptr;

    // Subjects for reactive bindings
    static constexpr size_t MAX_SHAPERS = 5;
    static constexpr size_t SHAPER_TYPE_BUF_SIZE = 16;
    static constexpr size_t SHAPER_VALUE_BUF_SIZE = 16;

    std::array<lv_subject_t, MAX_SHAPERS> shaper_visible_subjects_;
    std::array<lv_subject_t, MAX_SHAPERS> shaper_type_subjects_;
    std::array<lv_subject_t, MAX_SHAPERS> shaper_freq_subjects_;
    std::array<lv_subject_t, MAX_SHAPERS> shaper_vib_subjects_;

    // Fixed char arrays for string subjects
    char shaper_type_bufs_[MAX_SHAPERS][SHAPER_TYPE_BUF_SIZE] = {};
    char shaper_freq_bufs_[MAX_SHAPERS][SHAPER_VALUE_BUF_SIZE] = {};
    char shaper_vib_bufs_[MAX_SHAPERS][SHAPER_VALUE_BUF_SIZE] = {};

    // Results data
    char current_axis_ = 'X';
    std::vector<ShaperFit> shaper_results_;
    std::string recommended_type_;
    float recommended_freq_ = 0.0f;
};

// Global instance accessor
InputShaperPanel& get_global_input_shaper_panel();

/**
 * @brief Register XML event callbacks for input shaper panel
 *
 * Call this once at startup before creating any input_shaper_panel XML.
 * Registers callbacks for all button events (calibrate_x, calibrate_y, etc.).
 */
void ui_panel_input_shaper_register_callbacks();

/**
 * @brief Initialize row click callback for opening from Advanced panel
 *
 * Must be called during app initialization before XML creation.
 * Registers "on_input_shaper_row_clicked" callback.
 */
void init_input_shaper_row_handler();
