// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_panel_base.h"

/**
 * @file ui_panel_advanced.h
 * @brief Advanced Panel - Hub for advanced printer tools and calibration
 *
 * The Advanced Panel serves as a navigation hub for advanced features including:
 * - Bed Leveling (auto mesh, manual screws, QGL, Z-tilt)
 * - Input Shaping (resonance testing, Klippain Shake&Tune)
 * - Spoolman (filament tracking and inventory)
 * - Machine Limits (velocity/acceleration configuration)
 * - Z-Offset Calibration
 * - Macro Browser (execute printer macros)
 * - Diagnostics (console, restart options)
 *
 * ## Architecture:
 *
 * This panel uses the hub pattern - it's a scrollable list of action rows that
 * navigate to dedicated overlay panels for each feature. The hub itself is
 * stateless; all feature logic lives in the sub-panels.
 *
 * ## Capability-Driven UI:
 *
 * Features are conditionally shown based on PrinterCapabilities:
 * - Input Shaping requires accelerometer
 * - Spoolman requires Spoolman service
 * - Z-Offset requires probe
 *
 * @see PanelBase for base class documentation
 * @see PrinterCapabilities for capability detection
 */
class AdvancedPanel : public PanelBase {
  public:
    /**
     * @brief Construct AdvancedPanel with injected dependencies
     *
     * @param printer_state Reference to PrinterState
     * @param api Pointer to MoonrakerAPI
     */
    AdvancedPanel(PrinterState& printer_state, MoonrakerAPI* api);

    ~AdvancedPanel() override = default;

    //
    // === PanelBase Implementation ===
    //

    /**
     * @brief Initialize capability-related subjects for XML binding
     *
     * Creates subjects for:
     * - printer_has_accelerometer
     * - printer_has_spoolman
     *
     * Note: printer_has_probe is already created by SettingsPanel.
     */
    void init_subjects() override;

    /**
     * @brief Setup the advanced panel hub with navigation handlers
     *
     * @param panel Root panel object from lv_xml_create()
     * @param parent_screen Parent screen for overlay creation
     */
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;

    const char* get_name() const override {
        return "Advanced Panel";
    }
    const char* get_xml_component_name() const override {
        return "advanced_panel";
    }

    //
    // === Lifecycle Hooks ===
    //

    /**
     * @brief Refresh capability flags when panel becomes visible
     *
     * Updates subjects based on current PrinterCapabilities.
     */
    void on_activate() override;

  private:
    //
    // === Widget References (Action Rows) ===
    //

    lv_obj_t* bed_leveling_row_ = nullptr;
    lv_obj_t* input_shaping_row_ = nullptr;
    lv_obj_t* z_offset_row_ = nullptr;
    lv_obj_t* machine_limits_row_ = nullptr;
    lv_obj_t* spoolman_row_ = nullptr;
    lv_obj_t* macros_row_ = nullptr;
    lv_obj_t* console_row_ = nullptr;
    lv_obj_t* print_history_row_ = nullptr;
    lv_obj_t* restart_row_ = nullptr;

    //
    // === Lazily-Created Overlay Panels ===
    //

    lv_obj_t* zoffset_cal_panel_ = nullptr;       // Reuses existing panel
    lv_obj_t* history_dashboard_panel_ = nullptr; // Print history dashboard
    lv_obj_t* screws_tilt_panel_ = nullptr;       // Screws tilt adjust panel

    //
    // === Setup Helpers ===
    //

    void setup_action_handlers();

    //
    // === Navigation Handlers ===
    //

    void handle_bed_leveling_clicked();
    void handle_input_shaping_clicked();
    void handle_z_offset_clicked();
    void handle_machine_limits_clicked();
    void handle_spoolman_clicked();
    void handle_macros_clicked();
    void handle_console_clicked();
    void handle_print_history_clicked();
    void handle_restart_clicked();

    //
    // === Static Event Trampolines ===
    //

    static void on_bed_leveling_clicked(lv_event_t* e);
    static void on_input_shaping_clicked(lv_event_t* e);
    static void on_z_offset_clicked(lv_event_t* e);
    static void on_machine_limits_clicked(lv_event_t* e);
    static void on_spoolman_clicked(lv_event_t* e);
    static void on_macros_clicked(lv_event_t* e);
    static void on_console_clicked(lv_event_t* e);
    static void on_print_history_clicked(lv_event_t* e);
    static void on_restart_clicked(lv_event_t* e);
};

/**
 * @brief Global instance accessor
 *
 * Returns reference to singleton AdvancedPanel used by main.cpp.
 */
AdvancedPanel& get_global_advanced_panel();

/**
 * @brief Initialize the global AdvancedPanel instance
 *
 * Must be called by main.cpp before accessing get_global_advanced_panel().
 *
 * @param printer_state Reference to PrinterState
 * @param api Pointer to MoonrakerAPI
 */
void init_global_advanced_panel(PrinterState& printer_state, MoonrakerAPI* api);
