// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_panel_advanced.h"

#include "ui_nav.h"
#include "ui_panel_calibration_zoffset.h"
#include "ui_panel_history_dashboard.h"
#include "ui_panel_screws_tilt.h"
#include "ui_toast.h"

#include "moonraker_api.h"
#include "moonraker_client.h"
#include "printer_capabilities.h"
#include "printer_state.h"

#include <spdlog/spdlog.h>

// Forward declarations
MoonrakerClient* get_moonraker_client();
ZOffsetCalibrationPanel& get_global_zoffset_cal_panel();
ScrewsTiltPanel& get_global_screws_tilt_panel();

// Global instance (singleton pattern matching SettingsPanel)
static std::unique_ptr<AdvancedPanel> g_advanced_panel;

AdvancedPanel& get_global_advanced_panel() {
    // Should be initialized by main.cpp before use
    if (!g_advanced_panel) {
        spdlog::error("get_global_advanced_panel() called before initialization!");
        throw std::runtime_error("AdvancedPanel not initialized");
    }
    return *g_advanced_panel;
}

// Called by main.cpp to initialize the global instance
void init_global_advanced_panel(PrinterState& printer_state, MoonrakerAPI* api) {
    g_advanced_panel = std::make_unique<AdvancedPanel>(printer_state, api);
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

AdvancedPanel::AdvancedPanel(PrinterState& printer_state, MoonrakerAPI* api)
    : PanelBase(printer_state, api) {
    spdlog::trace("[{}] Constructor", get_name());
}

// ============================================================================
// PANELBASE IMPLEMENTATION
// ============================================================================

void AdvancedPanel::init_subjects() {
    // No local subjects - capability subjects are owned by PrinterState
    subjects_initialized_ = true;
}

void AdvancedPanel::setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    // Call base class to store panel_ and parent_screen_
    PanelBase::setup(panel, parent_screen);

    if (!panel_) {
        spdlog::error("[{}] NULL panel", get_name());
        return;
    }

    setup_action_handlers();
    spdlog::info("[{}] Setup complete", get_name());
}

void AdvancedPanel::on_activate() {
    spdlog::debug("[{}] Activated", get_name());
}

// ============================================================================
// SETUP HELPERS
// ============================================================================

void AdvancedPanel::setup_action_handlers() {
    // === Bed Leveling Row ===
    bed_leveling_row_ = lv_obj_find_by_name(panel_, "row_bed_leveling");
    if (bed_leveling_row_) {
        lv_obj_add_event_cb(bed_leveling_row_, on_bed_leveling_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Bed leveling action row", get_name());
    }

    // === Input Shaping Row ===
    input_shaping_row_ = lv_obj_find_by_name(panel_, "row_input_shaping");
    if (input_shaping_row_) {
        lv_obj_add_event_cb(input_shaping_row_, on_input_shaping_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Input shaping action row", get_name());
    }

    // === Z-Offset Row ===
    z_offset_row_ = lv_obj_find_by_name(panel_, "row_z_offset");
    if (z_offset_row_) {
        lv_obj_add_event_cb(z_offset_row_, on_z_offset_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Z-offset action row", get_name());
    }

    // === Machine Limits Row ===
    machine_limits_row_ = lv_obj_find_by_name(panel_, "row_machine_limits");
    if (machine_limits_row_) {
        lv_obj_add_event_cb(machine_limits_row_, on_machine_limits_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Machine limits action row", get_name());
    }

    // === Spoolman Row ===
    spoolman_row_ = lv_obj_find_by_name(panel_, "row_spoolman");
    if (spoolman_row_) {
        lv_obj_add_event_cb(spoolman_row_, on_spoolman_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Spoolman action row", get_name());
    }

    // === Macros Row ===
    macros_row_ = lv_obj_find_by_name(panel_, "row_macros");
    if (macros_row_) {
        lv_obj_add_event_cb(macros_row_, on_macros_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Macros action row", get_name());
    }

    // === Console Row ===
    console_row_ = lv_obj_find_by_name(panel_, "row_console");
    if (console_row_) {
        lv_obj_add_event_cb(console_row_, on_console_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Console action row", get_name());
    }

    // === Print History Row ===
    print_history_row_ = lv_obj_find_by_name(panel_, "row_print_history");
    if (print_history_row_) {
        lv_obj_add_event_cb(print_history_row_, on_print_history_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Print history action row", get_name());
    }

    // === Restart Row ===
    restart_row_ = lv_obj_find_by_name(panel_, "row_restart");
    if (restart_row_) {
        lv_obj_add_event_cb(restart_row_, on_restart_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Restart action row", get_name());
    }
}

// ============================================================================
// NAVIGATION HANDLERS
// ============================================================================

void AdvancedPanel::handle_bed_leveling_clicked() {
    spdlog::debug("[{}] Bed Leveling clicked - opening screws tilt panel", get_name());

    // Lazy-create the screws tilt panel
    if (!screws_tilt_panel_ && parent_screen_) {
        spdlog::debug("[{}] Creating screws tilt panel...", get_name());

        // Create from XML
        screws_tilt_panel_ =
            static_cast<lv_obj_t*>(lv_xml_create(parent_screen_, "screws_tilt_panel", nullptr));
        if (screws_tilt_panel_) {
            // Setup the panel class
            MoonrakerClient* client = get_moonraker_client();
            get_global_screws_tilt_panel().setup(screws_tilt_panel_, parent_screen_, client, api_);

            // Initially hidden
            lv_obj_add_flag(screws_tilt_panel_, LV_OBJ_FLAG_HIDDEN);
            spdlog::info("[{}] Screws tilt panel created", get_name());
        } else {
            spdlog::error("[{}] Failed to create screws tilt panel from XML", get_name());
            return;
        }
    }

    // Push screws tilt panel onto navigation and show it
    if (screws_tilt_panel_) {
        ui_nav_push_overlay(screws_tilt_panel_);
    }
}

void AdvancedPanel::handle_input_shaping_clicked() {
    spdlog::debug("[{}] Input Shaping clicked", get_name());

    MoonrakerClient* client = get_moonraker_client();
    if (client && client->capabilities().has_klippain_shaketune()) {
        ui_toast_show(ToastSeverity::INFO, "Input Shaping: Klippain Shake&Tune detected", 2000);
    } else {
        ui_toast_show(ToastSeverity::INFO, "Input Shaping: Coming soon", 2000);
    }
}

void AdvancedPanel::handle_z_offset_clicked() {
    spdlog::debug("[{}] Z-Offset clicked - opening calibration panel", get_name());

    // Reuse existing Z-Offset calibration panel from SettingsPanel
    if (!zoffset_cal_panel_ && parent_screen_) {
        spdlog::debug("[{}] Creating Z-Offset calibration panel...", get_name());

        // Create from XML
        zoffset_cal_panel_ = static_cast<lv_obj_t*>(
            lv_xml_create(parent_screen_, "calibration_zoffset_panel", nullptr));
        if (zoffset_cal_panel_) {
            // Setup event handlers (class-based API)
            MoonrakerClient* client = get_moonraker_client();
            get_global_zoffset_cal_panel().setup(zoffset_cal_panel_, parent_screen_, client);

            // Initially hidden
            lv_obj_add_flag(zoffset_cal_panel_, LV_OBJ_FLAG_HIDDEN);
            spdlog::info("[{}] Z-Offset calibration panel created", get_name());
        } else {
            spdlog::error("[{}] Failed to create Z-Offset panel from XML", get_name());
            return;
        }
    }

    // Push Z-Offset panel onto navigation history and show it
    if (zoffset_cal_panel_) {
        ui_nav_push_overlay(zoffset_cal_panel_);
    }
}

void AdvancedPanel::handle_machine_limits_clicked() {
    spdlog::debug("[{}] Machine Limits clicked", get_name());
    ui_toast_show(ToastSeverity::INFO, "Machine Limits: Coming soon", 2000);
}

void AdvancedPanel::handle_spoolman_clicked() {
    spdlog::debug("[{}] Spoolman clicked", get_name());
    ui_toast_show(ToastSeverity::INFO, "Spoolman: Coming soon", 2000);
}

void AdvancedPanel::handle_macros_clicked() {
    spdlog::debug("[{}] Macros clicked", get_name());

    MoonrakerClient* client = get_moonraker_client();
    if (client) {
        size_t macro_count = client->capabilities().macro_count();
        std::string msg = "Macros: " + std::to_string(macro_count) + " available";
        ui_toast_show(ToastSeverity::INFO, msg.c_str(), 2000);
    }
}

void AdvancedPanel::handle_console_clicked() {
    spdlog::debug("[{}] Console clicked", get_name());
    ui_toast_show(ToastSeverity::INFO, "Console: Coming soon", 2000);
}

void AdvancedPanel::handle_print_history_clicked() {
    spdlog::debug("[{}] Print History clicked - opening dashboard", get_name());

    // Lazy-create the history dashboard panel
    if (!history_dashboard_panel_ && parent_screen_) {
        spdlog::debug("[{}] Creating history dashboard panel...", get_name());

        // Create from XML
        history_dashboard_panel_ = static_cast<lv_obj_t*>(
            lv_xml_create(parent_screen_, "history_dashboard_panel", nullptr));
        if (history_dashboard_panel_) {
            // Setup the panel class
            get_global_history_dashboard_panel().setup(history_dashboard_panel_, parent_screen_);

            // Initially hidden
            lv_obj_add_flag(history_dashboard_panel_, LV_OBJ_FLAG_HIDDEN);
            spdlog::info("[{}] History dashboard panel created", get_name());
        } else {
            spdlog::error("[{}] Failed to create history dashboard panel from XML", get_name());
            return;
        }
    }

    // Push history dashboard onto navigation and show it
    if (history_dashboard_panel_) {
        ui_nav_push_overlay(history_dashboard_panel_);
        get_global_history_dashboard_panel().on_activate();
    }
}

void AdvancedPanel::handle_restart_clicked() {
    spdlog::debug("[{}] Restart clicked", get_name());
    ui_toast_show(ToastSeverity::INFO, "Restart: Coming soon", 2000);
}

// ============================================================================
// STATIC EVENT TRAMPOLINES
// ============================================================================

void AdvancedPanel::on_bed_leveling_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_bed_leveling_clicked();
}

void AdvancedPanel::on_input_shaping_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_input_shaping_clicked();
}

void AdvancedPanel::on_z_offset_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_z_offset_clicked();
}

void AdvancedPanel::on_machine_limits_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_machine_limits_clicked();
}

void AdvancedPanel::on_spoolman_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_spoolman_clicked();
}

void AdvancedPanel::on_macros_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_macros_clicked();
}

void AdvancedPanel::on_console_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_console_clicked();
}

void AdvancedPanel::on_print_history_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_print_history_clicked();
}

void AdvancedPanel::on_restart_clicked(lv_event_t* e) {
    auto* self = static_cast<AdvancedPanel*>(lv_event_get_user_data(e));
    if (self)
        self->handle_restart_clicked();
}
