// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_emergency_stop.h"

#include "ui_notification.h"
#include "ui_toast.h"

#include <spdlog/spdlog.h>

// Panels where the E-Stop button should be visible during active prints
const std::unordered_set<std::string> EmergencyStopOverlay::VISIBLE_PANELS = {
    "home_panel", "print_status_panel", "controls_panel", "motion_panel"};

EmergencyStopOverlay& EmergencyStopOverlay::instance() {
    static EmergencyStopOverlay instance;
    return instance;
}

void EmergencyStopOverlay::init(PrinterState& printer_state, MoonrakerAPI* api) {
    printer_state_ = &printer_state;
    api_ = api;
    spdlog::debug("[EmergencyStop] Initialized with dependencies");
}

void EmergencyStopOverlay::set_require_confirmation(bool require) {
    require_confirmation_ = require;
    spdlog::debug("[EmergencyStop] Confirmation requirement set to: {}", require);
}

void EmergencyStopOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // Initialize visibility subject (default hidden)
    lv_subject_init_int(&estop_visible_, 0);
    lv_xml_register_subject(nullptr, "estop_visible", &estop_visible_);

    // Register click callbacks for XML event binding
    lv_xml_register_event_cb(nullptr, "emergency_stop_clicked", emergency_stop_clicked);
    lv_xml_register_event_cb(nullptr, "estop_dialog_cancel_clicked", estop_dialog_cancel_clicked);
    lv_xml_register_event_cb(nullptr, "estop_dialog_confirm_clicked", estop_dialog_confirm_clicked);
    lv_xml_register_event_cb(nullptr, "recovery_restart_klipper_clicked", recovery_restart_klipper_clicked);
    lv_xml_register_event_cb(nullptr, "recovery_firmware_restart_clicked", recovery_firmware_restart_clicked);
    lv_xml_register_event_cb(nullptr, "recovery_dismiss_clicked", recovery_dismiss_clicked);

    // Advanced panel button callbacks (reuse same logic)
    lv_xml_register_event_cb(nullptr, "advanced_estop_clicked", advanced_estop_clicked);
    lv_xml_register_event_cb(nullptr, "advanced_restart_klipper_clicked", advanced_restart_klipper_clicked);
    lv_xml_register_event_cb(nullptr, "advanced_firmware_restart_clicked", advanced_firmware_restart_clicked);

    subjects_initialized_ = true;
    spdlog::debug("[EmergencyStop] Subjects initialized");
}

void EmergencyStopOverlay::create() {
    if (!printer_state_ || !api_) {
        spdlog::error("[EmergencyStop] Cannot create: dependencies not initialized");
        return;
    }

    if (!subjects_initialized_) {
        spdlog::error("[EmergencyStop] Cannot create: subjects not initialized");
        return;
    }

    // Create the floating button on the active screen
    lv_obj_t* screen = lv_screen_active();
    button_ = static_cast<lv_obj_t*>(lv_xml_create(screen, "emergency_stop_button", nullptr));

    if (!button_) {
        spdlog::error("[EmergencyStop] Failed to create emergency_stop_button widget");
        return;
    }

    // Ensure button is on top of all other content
    lv_obj_move_foreground(button_);

    // Subscribe to print state changes for automatic visibility updates
    print_state_observer_ =
        ObserverGuard(printer_state_->get_print_state_enum_subject(), print_state_observer_cb, this);

    // Subscribe to klippy state changes for recovery dialog auto-popup
    klippy_state_observer_ =
        ObserverGuard(printer_state_->get_klippy_state_subject(), klippy_state_observer_cb, this);

    // Initial visibility update
    update_visibility();

    spdlog::info("[EmergencyStop] Created floating E-Stop button");
}

void EmergencyStopOverlay::on_panel_changed(const std::string& panel_name) {
    current_panel_ = panel_name;
    update_visibility();
    spdlog::trace("[EmergencyStop] Panel changed to: {}", panel_name);
}

void EmergencyStopOverlay::update_visibility() {
    if (!printer_state_) {
        return;
    }

    // Check if current panel should show E-Stop
    bool on_relevant_panel = VISIBLE_PANELS.count(current_panel_) > 0;

    // Check if print is active (PRINTING or PAUSED)
    PrintJobState state = printer_state_->get_print_job_state();
    bool is_printing = (state == PrintJobState::PRINTING || state == PrintJobState::PAUSED);

    // Show button only if both conditions are met
    bool should_show = is_printing && on_relevant_panel;

    int new_value = should_show ? 1 : 0;
    int current_value = lv_subject_get_int(&estop_visible_);

    if (new_value != current_value) {
        lv_subject_set_int(&estop_visible_, new_value);
        spdlog::debug("[EmergencyStop] Visibility changed: {} (panel={}, state={})", should_show,
                      current_panel_, static_cast<int>(state));
    }
}

void EmergencyStopOverlay::handle_click() {
    spdlog::info("[EmergencyStop] Button clicked");

    if (require_confirmation_) {
        show_confirmation_dialog();
    } else {
        execute_emergency_stop();
    }
}

void EmergencyStopOverlay::execute_emergency_stop() {
    if (!api_) {
        spdlog::error("[EmergencyStop] Cannot execute: API not available");
        ui_toast_show(ToastSeverity::ERROR, "Emergency stop failed: not connected", 4000);
        return;
    }

    spdlog::warn("[EmergencyStop] Executing emergency stop (M112)!");

    api_->emergency_stop(
        []() {
            spdlog::info("[EmergencyStop] Emergency stop command sent successfully");
            ui_toast_show(ToastSeverity::WARNING, "Emergency stop activated", 5000);
        },
        [](const MoonrakerError& err) {
            spdlog::error("[EmergencyStop] Emergency stop failed: {}", err.message);
            ui_toast_show(ToastSeverity::ERROR,
                          ("Emergency stop failed: " + err.user_message()).c_str(), 5000);
        });
}

void EmergencyStopOverlay::show_confirmation_dialog() {
    // Don't show if already visible
    if (confirmation_dialog_) {
        spdlog::debug("[EmergencyStop] Confirmation dialog already visible");
        return;
    }

    spdlog::debug("[EmergencyStop] Showing confirmation dialog");

    // Create dialog on current screen
    lv_obj_t* screen = lv_screen_active();
    confirmation_dialog_ =
        static_cast<lv_obj_t*>(lv_xml_create(screen, "estop_confirmation_dialog", nullptr));

    if (!confirmation_dialog_) {
        spdlog::error("[EmergencyStop] Failed to create confirmation dialog, executing directly");
        execute_emergency_stop();
        return;
    }

    // Ensure dialog is on top of everything including the E-Stop button
    lv_obj_move_foreground(confirmation_dialog_);

    spdlog::info("[EmergencyStop] Confirmation dialog shown");
}

void EmergencyStopOverlay::dismiss_confirmation_dialog() {
    if (confirmation_dialog_) {
        lv_obj_delete(confirmation_dialog_);
        confirmation_dialog_ = nullptr;
        spdlog::debug("[EmergencyStop] Confirmation dialog dismissed");
    }
}

void EmergencyStopOverlay::show_recovery_dialog() {
    // Don't show if already visible
    if (recovery_dialog_) {
        spdlog::debug("[KlipperRecovery] Recovery dialog already visible");
        return;
    }

    spdlog::info("[KlipperRecovery] Showing recovery dialog (Klipper in SHUTDOWN state)");

    // Create dialog on current screen
    lv_obj_t* screen = lv_screen_active();
    recovery_dialog_ =
        static_cast<lv_obj_t*>(lv_xml_create(screen, "klipper_recovery_dialog", nullptr));

    if (!recovery_dialog_) {
        spdlog::error("[KlipperRecovery] Failed to create recovery dialog");
        return;
    }

    // Ensure dialog is on top of everything
    lv_obj_move_foreground(recovery_dialog_);
}

void EmergencyStopOverlay::dismiss_recovery_dialog() {
    if (recovery_dialog_) {
        lv_obj_delete(recovery_dialog_);
        recovery_dialog_ = nullptr;
        spdlog::debug("[KlipperRecovery] Recovery dialog dismissed");
    }
}

void EmergencyStopOverlay::restart_klipper() {
    if (!api_) {
        spdlog::error("[KlipperRecovery] Cannot restart: API not available");
        ui_toast_show(ToastSeverity::ERROR, "Restart failed: not connected", 4000);
        return;
    }

    spdlog::info("[KlipperRecovery] Restarting Klipper...");
    ui_toast_show(ToastSeverity::INFO, "Restarting Klipper...", 3000);

    api_->restart_klipper(
        []() {
            spdlog::info("[KlipperRecovery] Klipper restart command sent");
            // Toast will update when klippy_state changes to READY
        },
        [](const MoonrakerError& err) {
            spdlog::error("[KlipperRecovery] Klipper restart failed: {}", err.message);
            ui_toast_show(ToastSeverity::ERROR,
                          ("Restart failed: " + err.user_message()).c_str(), 5000);
        });
}

void EmergencyStopOverlay::firmware_restart() {
    if (!api_) {
        spdlog::error("[KlipperRecovery] Cannot firmware restart: API not available");
        ui_toast_show(ToastSeverity::ERROR, "Restart failed: not connected", 4000);
        return;
    }

    spdlog::info("[KlipperRecovery] Firmware restarting...");
    ui_toast_show(ToastSeverity::INFO, "Firmware restarting...", 3000);

    api_->restart_firmware(
        []() {
            spdlog::info("[KlipperRecovery] Firmware restart command sent");
            // Toast will update when klippy_state changes to READY
        },
        [](const MoonrakerError& err) {
            spdlog::error("[KlipperRecovery] Firmware restart failed: {}", err.message);
            ui_toast_show(ToastSeverity::ERROR,
                          ("Firmware restart failed: " + err.user_message()).c_str(), 5000);
        });
}

// Static callback trampolines
void EmergencyStopOverlay::emergency_stop_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    EmergencyStopOverlay::instance().handle_click();
}

void EmergencyStopOverlay::estop_dialog_cancel_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::debug("[EmergencyStop] Cancel clicked - aborting E-Stop");
    EmergencyStopOverlay::instance().dismiss_confirmation_dialog();
}

void EmergencyStopOverlay::estop_dialog_confirm_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::debug("[EmergencyStop] Confirm clicked - executing E-Stop");
    auto& instance = EmergencyStopOverlay::instance();
    instance.dismiss_confirmation_dialog();
    instance.execute_emergency_stop();
}

void EmergencyStopOverlay::recovery_restart_klipper_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::debug("[KlipperRecovery] Restart Klipper clicked");
    auto& instance = EmergencyStopOverlay::instance();
    instance.dismiss_recovery_dialog();
    instance.restart_klipper();
}

void EmergencyStopOverlay::recovery_firmware_restart_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::debug("[KlipperRecovery] Firmware Restart clicked");
    auto& instance = EmergencyStopOverlay::instance();
    instance.dismiss_recovery_dialog();
    instance.firmware_restart();
}

void EmergencyStopOverlay::recovery_dismiss_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::debug("[KlipperRecovery] Dismiss clicked");
    EmergencyStopOverlay::instance().dismiss_recovery_dialog();
}

// Advanced panel button callbacks
void EmergencyStopOverlay::advanced_estop_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::info("[Advanced] E-Stop clicked from Advanced panel");
    EmergencyStopOverlay::instance().handle_click();
}

void EmergencyStopOverlay::advanced_restart_klipper_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::info("[Advanced] Restart Klipper clicked from Advanced panel");
    EmergencyStopOverlay::instance().restart_klipper();
}

void EmergencyStopOverlay::advanced_firmware_restart_clicked(lv_event_t* e) {
    LV_UNUSED(e);
    spdlog::info("[Advanced] Firmware Restart clicked from Advanced panel");
    EmergencyStopOverlay::instance().firmware_restart();
}

// Static observer callbacks
void EmergencyStopOverlay::print_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    LV_UNUSED(subject);
    auto* self = static_cast<EmergencyStopOverlay*>(lv_observer_get_user_data(observer));
    if (self) {
        self->update_visibility();
    }
}

void EmergencyStopOverlay::klippy_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<EmergencyStopOverlay*>(lv_observer_get_user_data(observer));
    if (!self) {
        return;
    }

    int state = lv_subject_get_int(subject);
    auto klippy_state = static_cast<KlippyState>(state);

    if (klippy_state == KlippyState::SHUTDOWN) {
        // Auto-popup recovery dialog when Klipper enters SHUTDOWN state
        spdlog::info("[KlipperRecovery] Detected Klipper SHUTDOWN state, showing recovery dialog");
        self->show_recovery_dialog();
    } else if (klippy_state == KlippyState::READY) {
        // Auto-dismiss recovery dialog when Klipper is back to READY
        if (self->recovery_dialog_) {
            spdlog::info("[KlipperRecovery] Klipper is READY, dismissing recovery dialog");
            self->dismiss_recovery_dialog();
            ui_toast_show(ToastSeverity::SUCCESS, "Printer ready", 3000);
        }
    }
}
