// SPDX-License-Identifier: GPL-3.0-or-later

#include "print_completion.h"

#include "ui_modal.h"
#include "ui_toast.h"

#include "app_globals.h"
#include "printer_state.h"
#include "settings_manager.h"

#include <spdlog/spdlog.h>

#include <lvgl.h>

namespace helix {

// Track previous state to detect transitions to terminal states
static PrintJobState prev_print_state = PrintJobState::STANDBY;

// Callback for print state changes - triggers completion notifications
static void on_print_state_changed_for_notification(lv_observer_t* observer,
                                                    lv_subject_t* subject) {
    (void)observer;
    auto current = static_cast<PrintJobState>(lv_subject_get_int(subject));

    // Check for transitions to terminal states (from active print states)
    bool was_active =
        (prev_print_state == PrintJobState::PRINTING || prev_print_state == PrintJobState::PAUSED);
    bool is_terminal = (current == PrintJobState::COMPLETE || current == PrintJobState::CANCELLED ||
                        current == PrintJobState::ERROR);

    if (was_active && is_terminal) {
        auto mode = SettingsManager::instance().get_completion_alert_mode();
        if (mode == CompletionAlertMode::OFF) {
            spdlog::debug("[PrintComplete] Notification disabled");
            prev_print_state = current;
            return;
        }

        // Get filename from PrinterState
        const char* filename =
            lv_subject_get_string(get_printer_state().get_print_filename_subject());
        const char* display_name = (filename && filename[0]) ? filename : "Unknown";

        // Determine message and severity based on state
        char message[128];
        ToastSeverity severity = ToastSeverity::SUCCESS;
        ui_modal_severity modal_severity = UI_MODAL_SEVERITY_INFO;

        switch (current) {
        case PrintJobState::COMPLETE:
            snprintf(message, sizeof(message), "Print complete: %s", display_name);
            severity = ToastSeverity::SUCCESS;
            modal_severity = UI_MODAL_SEVERITY_INFO;
            break;
        case PrintJobState::CANCELLED:
            snprintf(message, sizeof(message), "Print cancelled: %s", display_name);
            severity = ToastSeverity::WARNING;
            modal_severity = UI_MODAL_SEVERITY_WARNING;
            break;
        case PrintJobState::ERROR:
            snprintf(message, sizeof(message), "Print failed: %s", display_name);
            severity = ToastSeverity::ERROR;
            modal_severity = UI_MODAL_SEVERITY_ERROR;
            break;
        default:
            break;
        }

        spdlog::info("[PrintComplete] Showing notification: {} (mode={})", message,
                     static_cast<int>(mode));

        // Wake display if sleeping
        SettingsManager::instance().wake_display();

        if (mode == CompletionAlertMode::NOTIFICATION) {
            // Brief toast (5 seconds)
            ui_toast_show(severity, message, 5000);
        } else if (mode == CompletionAlertMode::ALERT) {
            // Modal dialog requiring user acknowledgment
            ui_modal_config_t config = {
                .position = {.use_alignment = true, .alignment = LV_ALIGN_CENTER},
                .backdrop_opa = 180,
                .keyboard = nullptr,
                .persistent = false,
                .on_close = nullptr};

            const char* title =
                (current == PrintJobState::COMPLETE)
                    ? "Print Complete"
                    : (current == PrintJobState::CANCELLED ? "Print Cancelled" : "Print Failed");
            const char* attrs[] = {"title", title, "message", message, nullptr};

            ui_modal_configure(modal_severity, false, "OK", nullptr);
            lv_obj_t* modal = ui_modal_show("modal_dialog", &config, attrs);
            if (modal) {
                // Wire up OK button to dismiss
                lv_obj_t* ok_btn = lv_obj_find_by_name(modal, "btn_primary");
                if (ok_btn) {
                    lv_obj_set_user_data(ok_btn, modal);
                    lv_obj_add_event_cb(
                        ok_btn,
                        [](lv_event_t* e) {
                            auto* btn = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
                            auto* dlg = static_cast<lv_obj_t*>(lv_obj_get_user_data(btn));
                            if (dlg) {
                                ui_modal_hide(dlg);
                            }
                        },
                        LV_EVENT_CLICKED, nullptr);
                }
            }
        }
    }

    prev_print_state = current;
}

ObserverGuard init_print_completion_observer() {
    spdlog::debug("Print completion observer registered");
    return ObserverGuard(get_printer_state().get_print_state_enum_subject(),
                         on_print_state_changed_for_notification, nullptr);
}

} // namespace helix
