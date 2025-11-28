// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_panel_print_status.h"

#include "ui_error_reporting.h"
#include "ui_event_safety.h"
#include "ui_nav.h"
#include "ui_panel_common.h"
#include "ui_subject_registry.h"
#include "ui_utils.h"

#include "app_globals.h"
#include "config.h"
#include "moonraker_api.h"
#include "printer_state.h"
#include "wizard_config_paths.h"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstring>
#include <memory>

// Global instance for legacy API and resize callback
static std::unique_ptr<PrintStatusPanel> g_print_status_panel;

// Helper to get or create the global instance
PrintStatusPanel& get_global_print_status_panel() {
    if (!g_print_status_panel) {
        g_print_status_panel = std::make_unique<PrintStatusPanel>(get_printer_state(), nullptr);
    }
    return *g_print_status_panel;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

PrintStatusPanel::PrintStatusPanel(PrinterState& printer_state, MoonrakerAPI* api)
    : PanelBase(printer_state, api) {
    // Buffers are initialized with default values in header

    // Subscribe to PrinterState temperature subjects
    extruder_temp_observer_ = lv_subject_add_observer(printer_state_.get_extruder_temp_subject(),
                                                      extruder_temp_observer_cb, this);
    extruder_target_observer_ = lv_subject_add_observer(
        printer_state_.get_extruder_target_subject(), extruder_target_observer_cb, this);
    bed_temp_observer_ =
        lv_subject_add_observer(printer_state_.get_bed_temp_subject(), bed_temp_observer_cb, this);
    bed_target_observer_ = lv_subject_add_observer(printer_state_.get_bed_target_subject(),
                                                   bed_target_observer_cb, this);

    // Subscribe to print progress and state
    print_progress_observer_ = lv_subject_add_observer(printer_state_.get_print_progress_subject(),
                                                       print_progress_observer_cb, this);
    print_state_observer_ = lv_subject_add_observer(printer_state_.get_print_state_subject(),
                                                    print_state_observer_cb, this);
    print_filename_observer_ = lv_subject_add_observer(printer_state_.get_print_filename_subject(),
                                                       print_filename_observer_cb, this);

    // Subscribe to speed/flow factors
    speed_factor_observer_ = lv_subject_add_observer(printer_state_.get_speed_factor_subject(),
                                                     speed_factor_observer_cb, this);
    flow_factor_observer_ = lv_subject_add_observer(printer_state_.get_flow_factor_subject(),
                                                    flow_factor_observer_cb, this);

    spdlog::debug("[{}] Subscribed to PrinterState subjects (temps, progress, state, speeds)",
                  get_name());

    // Load configured LED from wizard settings
    Config* config = Config::get_instance();
    if (config) {
        configured_led_ = config->get<std::string>(WizardConfigPaths::LED_STRIP, "");
        if (!configured_led_.empty()) {
            // Subscribe to LED state changes from PrinterState
            led_state_observer_ = lv_subject_add_observer(printer_state_.get_led_state_subject(),
                                                          led_state_observer_cb, this);
            spdlog::debug("[{}] Configured LED: {} (observing state)", get_name(), configured_led_);
        }
    }
}

PrintStatusPanel::~PrintStatusPanel() {
    // Note: Do NOT call ui_resize_handler_unregister here!
    // During static destruction order, the resize handler may already be destroyed.
    // The resize handler uses a weak reference pattern - if the panel is gone,
    // it simply won't call the callback.
    resize_registered_ = false;

    // RAII cleanup: remove PrinterState observers
    if (extruder_temp_observer_) {
        lv_observer_remove(extruder_temp_observer_);
        extruder_temp_observer_ = nullptr;
    }
    if (extruder_target_observer_) {
        lv_observer_remove(extruder_target_observer_);
        extruder_target_observer_ = nullptr;
    }
    if (bed_temp_observer_) {
        lv_observer_remove(bed_temp_observer_);
        bed_temp_observer_ = nullptr;
    }
    if (bed_target_observer_) {
        lv_observer_remove(bed_target_observer_);
        bed_target_observer_ = nullptr;
    }
    if (print_progress_observer_) {
        lv_observer_remove(print_progress_observer_);
        print_progress_observer_ = nullptr;
    }
    if (print_state_observer_) {
        lv_observer_remove(print_state_observer_);
        print_state_observer_ = nullptr;
    }
    if (print_filename_observer_) {
        lv_observer_remove(print_filename_observer_);
        print_filename_observer_ = nullptr;
    }
    if (speed_factor_observer_) {
        lv_observer_remove(speed_factor_observer_);
        speed_factor_observer_ = nullptr;
    }
    if (flow_factor_observer_) {
        lv_observer_remove(flow_factor_observer_);
        flow_factor_observer_ = nullptr;
    }
    if (led_state_observer_) {
        lv_observer_remove(led_state_observer_);
        led_state_observer_ = nullptr;
    }
}

// ============================================================================
// PANELBASE IMPLEMENTATION
// ============================================================================

void PrintStatusPanel::init_subjects() {
    if (subjects_initialized_) {
        spdlog::warn("[{}] init_subjects() called twice - ignoring", get_name());
        return;
    }

    // Initialize all 10 subjects with default values
    UI_SUBJECT_INIT_AND_REGISTER_STRING(filename_subject_, filename_buf_, "No print active",
                                        "print_filename");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(progress_text_subject_, progress_text_buf_, "0%",
                                        "print_progress_text");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(layer_text_subject_, layer_text_buf_, "Layer 0 / 0",
                                        "print_layer_text");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(elapsed_subject_, elapsed_buf_, "0h 00m", "print_elapsed");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(remaining_subject_, remaining_buf_, "0h 00m",
                                        "print_remaining");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(nozzle_temp_subject_, nozzle_temp_buf_, "0 / 0°C",
                                        "nozzle_temp_text");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(bed_temp_subject_, bed_temp_buf_, "0 / 0°C",
                                        "bed_temp_text");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(speed_subject_, speed_buf_, "100%", "print_speed_text");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(flow_subject_, flow_buf_, "100%", "print_flow_text");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(pause_button_subject_, pause_button_buf_, "Pause",
                                        "pause_button_text");

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized (10 subjects)", get_name());
}

void PrintStatusPanel::setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    // Call base class to store panel_ and parent_screen_
    PanelBase::setup(panel, parent_screen);

    if (!panel_) {
        spdlog::error("[{}] NULL panel", get_name());
        return;
    }

    spdlog::info("[{}] Setting up panel...", get_name());

    // Calculate width to fill remaining space after navigation bar
    lv_coord_t screen_width = lv_obj_get_width(parent_screen_);
    lv_coord_t nav_width = screen_width / 10;
    lv_obj_set_width(panel_, screen_width - nav_width);

    // Use standard overlay panel setup for header/content/back button
    ui_overlay_panel_setup_standard(panel_, parent_screen_, "overlay_header", "overlay_content");

    lv_obj_t* overlay_content = lv_obj_find_by_name(panel_, "overlay_content");
    if (!overlay_content) {
        spdlog::error("[{}] overlay_content not found!", get_name());
        return;
    }

    // Force layout calculation
    lv_obj_update_layout(panel_);

    // Register resize callback
    ui_resize_handler_register(on_resize_static);
    resize_registered_ = true;

    // Wire up event handlers
    spdlog::debug("[{}] Wiring event handlers...", get_name());

    // Nozzle temperature card
    lv_obj_t* nozzle_card = lv_obj_find_by_name(overlay_content, "nozzle_temp_card");
    if (nozzle_card) {
        lv_obj_add_event_cb(nozzle_card, on_nozzle_card_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Nozzle temp card", get_name());
    } else {
        spdlog::error("[{}]   ✗ Nozzle temp card NOT FOUND", get_name());
    }

    // Bed temperature card
    lv_obj_t* bed_card = lv_obj_find_by_name(overlay_content, "bed_temp_card");
    if (bed_card) {
        lv_obj_add_event_cb(bed_card, on_bed_card_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Bed temp card", get_name());
    } else {
        spdlog::error("[{}]   ✗ Bed temp card NOT FOUND", get_name());
    }

    // Light button
    lv_obj_t* light_btn = lv_obj_find_by_name(overlay_content, "btn_light");
    if (light_btn) {
        lv_obj_add_event_cb(light_btn, on_light_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Light button", get_name());
    } else {
        spdlog::error("[{}]   ✗ Light button NOT FOUND", get_name());
    }

    // Pause button
    lv_obj_t* pause_btn = lv_obj_find_by_name(overlay_content, "btn_pause");
    if (pause_btn) {
        lv_obj_add_event_cb(pause_btn, on_pause_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Pause button", get_name());
    } else {
        spdlog::error("[{}]   ✗ Pause button NOT FOUND", get_name());
    }

    // Tune button
    lv_obj_t* tune_btn = lv_obj_find_by_name(overlay_content, "btn_tune");
    if (tune_btn) {
        lv_obj_add_event_cb(tune_btn, on_tune_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Tune button", get_name());
    } else {
        spdlog::error("[{}]   ✗ Tune button NOT FOUND", get_name());
    }

    // Cancel button
    lv_obj_t* cancel_btn = lv_obj_find_by_name(overlay_content, "btn_cancel");
    if (cancel_btn) {
        lv_obj_add_event_cb(cancel_btn, on_cancel_clicked, LV_EVENT_CLICKED, this);
        spdlog::debug("[{}]   ✓ Cancel button", get_name());
    } else {
        spdlog::error("[{}]   ✗ Cancel button NOT FOUND", get_name());
    }

    // Progress bar widget
    progress_bar_ = lv_obj_find_by_name(overlay_content, "print_progress");
    if (progress_bar_) {
        lv_bar_set_range(progress_bar_, 0, 100);
        lv_bar_set_value(progress_bar_, 0, LV_ANIM_OFF);
        spdlog::debug("[{}]   ✓ Progress bar", get_name());
    } else {
        spdlog::error("[{}]   ✗ Progress bar NOT FOUND", get_name());
    }

    spdlog::info("[{}] Setup complete!", get_name());
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

void PrintStatusPanel::format_time(int seconds, char* buf, size_t buf_size) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    std::snprintf(buf, buf_size, "%dh %02dm", hours, minutes);
}

void PrintStatusPanel::update_all_displays() {
    // Progress text
    std::snprintf(progress_text_buf_, sizeof(progress_text_buf_), "%d%%", current_progress_);
    lv_subject_copy_string(&progress_text_subject_, progress_text_buf_);

    // Layer text
    std::snprintf(layer_text_buf_, sizeof(layer_text_buf_), "Layer %d / %d", current_layer_,
                  total_layers_);
    lv_subject_copy_string(&layer_text_subject_, layer_text_buf_);

    // Time displays
    format_time(elapsed_seconds_, elapsed_buf_, sizeof(elapsed_buf_));
    lv_subject_copy_string(&elapsed_subject_, elapsed_buf_);

    format_time(remaining_seconds_, remaining_buf_, sizeof(remaining_buf_));
    lv_subject_copy_string(&remaining_subject_, remaining_buf_);

    // Temperatures
    std::snprintf(nozzle_temp_buf_, sizeof(nozzle_temp_buf_), "%d / %d°C", nozzle_current_,
                  nozzle_target_);
    lv_subject_copy_string(&nozzle_temp_subject_, nozzle_temp_buf_);

    std::snprintf(bed_temp_buf_, sizeof(bed_temp_buf_), "%d / %d°C", bed_current_, bed_target_);
    lv_subject_copy_string(&bed_temp_subject_, bed_temp_buf_);

    // Speeds
    std::snprintf(speed_buf_, sizeof(speed_buf_), "%d%%", speed_percent_);
    lv_subject_copy_string(&speed_subject_, speed_buf_);

    std::snprintf(flow_buf_, sizeof(flow_buf_), "%d%%", flow_percent_);
    lv_subject_copy_string(&flow_subject_, flow_buf_);

    // Update progress bar widget directly
    if (progress_bar_) {
        lv_bar_set_value(progress_bar_, current_progress_, LV_ANIM_OFF);
    }

    // Update pause button text based on state
    if (current_state_ == PrintState::Paused) {
        std::snprintf(pause_button_buf_, sizeof(pause_button_buf_), "Resume");
    } else {
        std::snprintf(pause_button_buf_, sizeof(pause_button_buf_), "Pause");
    }
    lv_subject_copy_string(&pause_button_subject_, pause_button_buf_);
}

// ============================================================================
// INSTANCE HANDLERS
// ============================================================================

void PrintStatusPanel::handle_nozzle_card_click() {
    spdlog::debug("[{}] Nozzle temp card clicked", get_name());
    // TODO: Show nozzle temperature adjustment panel
}

void PrintStatusPanel::handle_bed_card_click() {
    spdlog::debug("[{}] Bed temp card clicked", get_name());
    // TODO: Show bed temperature adjustment panel
}

void PrintStatusPanel::handle_light_button() {
    spdlog::info("[{}] Light button clicked", get_name());

    // Check if LED is configured
    if (configured_led_.empty()) {
        spdlog::warn("[{}] Light toggle called but no LED configured", get_name());
        return;
    }

    // Toggle to opposite of current state
    bool new_state = !led_on_;

    // Send command to Moonraker
    if (api_) {
        if (new_state) {
            api_->set_led_on(
                configured_led_,
                [this]() {
                    spdlog::info("[{}] LED turned ON - waiting for state update", get_name());
                },
                [](const MoonrakerError& err) {
                    spdlog::error("Failed to turn LED on: {}", err.message);
                    NOTIFY_ERROR("Failed to turn light on: {}", err.user_message());
                });
        } else {
            api_->set_led_off(
                configured_led_,
                [this]() {
                    spdlog::info("[{}] LED turned OFF - waiting for state update", get_name());
                },
                [](const MoonrakerError& err) {
                    spdlog::error("Failed to turn LED off: {}", err.message);
                    NOTIFY_ERROR("Failed to turn light off: {}", err.user_message());
                });
        }
    } else {
        spdlog::warn("[{}] API not available - cannot control LED", get_name());
        NOTIFY_ERROR("Cannot control light: printer not connected");
    }
}

void PrintStatusPanel::handle_pause_button() {
    if (current_state_ == PrintState::Printing) {
        spdlog::info("[{}] Pausing print...", get_name());

        if (api_) {
            api_->pause_print(
                [this]() {
                    spdlog::info("[{}] Pause command sent successfully", get_name());
                    // State will update via PrinterState observer when Moonraker confirms
                },
                [](const MoonrakerError& err) {
                    spdlog::error("Failed to pause print: {}", err.message);
                    NOTIFY_ERROR("Failed to pause print: {}", err.user_message());
                });
        } else {
            // Fall back to local state change for mock mode
            spdlog::warn("[{}] API not available - using local state change", get_name());
            set_state(PrintState::Paused);
        }
    } else if (current_state_ == PrintState::Paused) {
        spdlog::info("[{}] Resuming print...", get_name());

        if (api_) {
            api_->resume_print(
                [this]() {
                    spdlog::info("[{}] Resume command sent successfully", get_name());
                    // State will update via PrinterState observer when Moonraker confirms
                },
                [](const MoonrakerError& err) {
                    spdlog::error("Failed to resume print: {}", err.message);
                    NOTIFY_ERROR("Failed to resume print: {}", err.user_message());
                });
        } else {
            // Fall back to local state change for mock mode
            spdlog::warn("[{}] API not available - using local state change", get_name());
            set_state(PrintState::Printing);
        }
    }
}

void PrintStatusPanel::handle_tune_button() {
    spdlog::info("[{}] Tune button clicked (not yet implemented)", get_name());
    // TODO: Open tuning overlay with speed/flow/temp adjustments
}

void PrintStatusPanel::handle_cancel_button() {
    spdlog::info("[{}] Cancel button clicked", get_name());

    // TODO: Add confirmation dialog before canceling

    if (api_) {
        api_->cancel_print(
            [this]() {
                spdlog::info("[{}] Cancel command sent successfully", get_name());
                // State will update via PrinterState observer when Moonraker confirms
                stop_mock_print(); // Stop any mock simulation
            },
            [](const MoonrakerError& err) {
                spdlog::error("Failed to cancel print: {}", err.message);
                NOTIFY_ERROR("Failed to cancel print: {}", err.user_message());
            });
    } else {
        // Fall back to local state change for mock mode
        spdlog::warn("[{}] API not available - using local state change", get_name());
        set_state(PrintState::Cancelled);
        stop_mock_print();
    }
}

void PrintStatusPanel::handle_resize() {
    spdlog::debug("[{}] Handling resize event", get_name());
}

// ============================================================================
// STATIC TRAMPOLINES
// ============================================================================

void PrintStatusPanel::on_nozzle_card_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrintStatusPanel] on_nozzle_card_clicked");
    auto* self = static_cast<PrintStatusPanel*>(lv_event_get_user_data(e));
    if (self) {
        self->handle_nozzle_card_click();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrintStatusPanel::on_bed_card_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrintStatusPanel] on_bed_card_clicked");
    auto* self = static_cast<PrintStatusPanel*>(lv_event_get_user_data(e));
    if (self) {
        self->handle_bed_card_click();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrintStatusPanel::on_light_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrintStatusPanel] on_light_clicked");
    auto* self = static_cast<PrintStatusPanel*>(lv_event_get_user_data(e));
    if (self) {
        self->handle_light_button();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrintStatusPanel::on_pause_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrintStatusPanel] on_pause_clicked");
    auto* self = static_cast<PrintStatusPanel*>(lv_event_get_user_data(e));
    if (self) {
        self->handle_pause_button();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrintStatusPanel::on_tune_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrintStatusPanel] on_tune_clicked");
    auto* self = static_cast<PrintStatusPanel*>(lv_event_get_user_data(e));
    if (self) {
        self->handle_tune_button();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrintStatusPanel::on_cancel_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrintStatusPanel] on_cancel_clicked");
    auto* self = static_cast<PrintStatusPanel*>(lv_event_get_user_data(e));
    if (self) {
        self->handle_cancel_button();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrintStatusPanel::on_resize_static() {
    // Use global instance for resize callback (registered without user_data)
    if (g_print_status_panel) {
        g_print_status_panel->handle_resize();
    }
}

// ============================================================================
// PRINTERSTATE OBSERVER CALLBACKS
// ============================================================================

void PrintStatusPanel::extruder_temp_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    (void)subject;
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_temperature_changed();
    }
}

void PrintStatusPanel::extruder_target_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    (void)subject;
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_temperature_changed();
    }
}

void PrintStatusPanel::bed_temp_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    (void)subject;
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_temperature_changed();
    }
}

void PrintStatusPanel::bed_target_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    (void)subject;
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_temperature_changed();
    }
}

void PrintStatusPanel::print_progress_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_print_progress_changed(lv_subject_get_int(subject));
    }
}

void PrintStatusPanel::print_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_print_state_changed(lv_subject_get_string(subject));
    }
}

void PrintStatusPanel::print_filename_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_print_filename_changed(lv_subject_get_string(subject));
    }
}

void PrintStatusPanel::speed_factor_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_speed_factor_changed(lv_subject_get_int(subject));
    }
}

void PrintStatusPanel::flow_factor_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_flow_factor_changed(lv_subject_get_int(subject));
    }
}

void PrintStatusPanel::led_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PrintStatusPanel*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_led_state_changed(lv_subject_get_int(subject));
    }
}

// ============================================================================
// OBSERVER INSTANCE METHODS
// ============================================================================

void PrintStatusPanel::on_temperature_changed() {
    // Read all temperature values from PrinterState subjects
    int extruder_temp = lv_subject_get_int(printer_state_.get_extruder_temp_subject());
    int extruder_target = lv_subject_get_int(printer_state_.get_extruder_target_subject());
    int bed_temp = lv_subject_get_int(printer_state_.get_bed_temp_subject());
    int bed_target = lv_subject_get_int(printer_state_.get_bed_target_subject());

    // Update internal state and display
    set_temperatures(extruder_temp, extruder_target, bed_temp, bed_target);

    spdlog::trace("[{}] Temperatures updated: nozzle {}/{}°C, bed {}/{}°C", get_name(),
                  extruder_temp, extruder_target, bed_temp, bed_target);
}

void PrintStatusPanel::on_print_progress_changed(int progress) {
    // Update progress display without calling update_all_displays()
    // to avoid redundant updates when multiple subjects change
    current_progress_ = progress;
    if (current_progress_ < 0)
        current_progress_ = 0;
    if (current_progress_ > 100)
        current_progress_ = 100;

    // Update progress text
    std::snprintf(progress_text_buf_, sizeof(progress_text_buf_), "%d%%", current_progress_);
    lv_subject_copy_string(&progress_text_subject_, progress_text_buf_);

    // Update progress bar widget directly
    if (progress_bar_) {
        lv_bar_set_value(progress_bar_, current_progress_, LV_ANIM_OFF);
    }

    spdlog::trace("[{}] Progress updated: {}%", get_name(), current_progress_);
}

void PrintStatusPanel::on_print_state_changed(const char* state) {
    if (!state) {
        return;
    }

    // Map Moonraker state string to PrintState enum
    PrintState new_state = PrintState::Idle;

    if (std::strcmp(state, "standby") == 0) {
        new_state = PrintState::Idle;
    } else if (std::strcmp(state, "printing") == 0) {
        new_state = PrintState::Printing;
    } else if (std::strcmp(state, "paused") == 0) {
        new_state = PrintState::Paused;
    } else if (std::strcmp(state, "complete") == 0) {
        new_state = PrintState::Complete;
    } else if (std::strcmp(state, "error") == 0) {
        new_state = PrintState::Error;
    } else if (std::strcmp(state, "cancelled") == 0) {
        new_state = PrintState::Cancelled;
    } else {
        spdlog::warn("[{}] Unknown print state: {}", get_name(), state);
    }

    // Only update if state actually changed
    if (new_state != current_state_) {
        set_state(new_state);
        spdlog::info("[{}] Print state changed: {} -> {}", get_name(), state,
                     static_cast<int>(new_state));
    }
}

void PrintStatusPanel::on_print_filename_changed(const char* filename) {
    if (filename && filename[0] != '\0') {
        set_filename(filename);
        spdlog::debug("[{}] Filename updated: {}", get_name(), filename);
    }
}

void PrintStatusPanel::on_speed_factor_changed(int speed) {
    speed_percent_ = speed;
    std::snprintf(speed_buf_, sizeof(speed_buf_), "%d%%", speed_percent_);
    lv_subject_copy_string(&speed_subject_, speed_buf_);

    spdlog::trace("[{}] Speed factor updated: {}%", get_name(), speed);
}

void PrintStatusPanel::on_flow_factor_changed(int flow) {
    flow_percent_ = flow;
    std::snprintf(flow_buf_, sizeof(flow_buf_), "%d%%", flow_percent_);
    lv_subject_copy_string(&flow_subject_, flow_buf_);

    spdlog::trace("[{}] Flow factor updated: {}%", get_name(), flow);
}

void PrintStatusPanel::on_led_state_changed(int state) {
    led_on_ = (state != 0);
    spdlog::debug("[{}] LED state changed: {} (from PrinterState)", get_name(),
                  led_on_ ? "ON" : "OFF");
}

// ============================================================================
// PUBLIC API
// ============================================================================

void PrintStatusPanel::set_filename(const char* filename) {
    std::snprintf(filename_buf_, sizeof(filename_buf_), "%s", filename);
    lv_subject_copy_string(&filename_subject_, filename_buf_);
}

void PrintStatusPanel::set_progress(int percent) {
    current_progress_ = percent;
    if (current_progress_ < 0)
        current_progress_ = 0;
    if (current_progress_ > 100)
        current_progress_ = 100;
    update_all_displays();
}

void PrintStatusPanel::set_layer(int current, int total) {
    current_layer_ = current;
    total_layers_ = total;
    update_all_displays();
}

void PrintStatusPanel::set_times(int elapsed_secs, int remaining_secs) {
    elapsed_seconds_ = elapsed_secs;
    remaining_seconds_ = remaining_secs;
    update_all_displays();
}

void PrintStatusPanel::set_temperatures(int nozzle_cur, int nozzle_tgt, int bed_cur, int bed_tgt) {
    nozzle_current_ = nozzle_cur;
    nozzle_target_ = nozzle_tgt;
    bed_current_ = bed_cur;
    bed_target_ = bed_tgt;
    update_all_displays();
}

void PrintStatusPanel::set_speeds(int speed_pct, int flow_pct) {
    speed_percent_ = speed_pct;
    flow_percent_ = flow_pct;
    update_all_displays();
}

void PrintStatusPanel::set_state(PrintState state) {
    current_state_ = state;
    update_all_displays();
    spdlog::debug("[{}] State changed to: {}", get_name(), static_cast<int>(state));
}

// ============================================================================
// MOCK PRINT SIMULATION
// ============================================================================

void PrintStatusPanel::start_mock_print(const char* filename, int layers, int duration_secs) {
    mock_active_ = true;
    mock_total_seconds_ = duration_secs;
    mock_elapsed_seconds_ = 0;
    mock_total_layers_ = layers;

    set_filename(filename);
    set_progress(0);
    set_layer(0, layers);
    set_times(0, duration_secs);
    set_temperatures(215, 215, 60, 60);
    set_speeds(100, 100);
    set_state(PrintState::Printing);

    spdlog::info("[{}] Mock print started: {} ({} layers, {} seconds)", get_name(), filename,
                 layers, duration_secs);
}

void PrintStatusPanel::stop_mock_print() {
    mock_active_ = false;
    spdlog::info("[{}] Mock print stopped", get_name());
}

void PrintStatusPanel::tick_mock_print() {
    if (!mock_active_)
        return;
    if (current_state_ != PrintState::Printing)
        return;
    if (mock_elapsed_seconds_ >= mock_total_seconds_) {
        // Print complete
        set_state(PrintState::Complete);
        stop_mock_print();
        spdlog::info("[{}] Mock print complete!", get_name());
        return;
    }

    // Advance simulation by 1 second
    mock_elapsed_seconds_++;

    // Calculate progress
    int progress = (mock_elapsed_seconds_ * 100) / mock_total_seconds_;
    int remaining = mock_total_seconds_ - mock_elapsed_seconds_;
    int layer = (mock_elapsed_seconds_ * mock_total_layers_) / mock_total_seconds_;

    // Update displays
    set_progress(progress);
    set_layer(layer, mock_total_layers_);
    set_times(mock_elapsed_seconds_, remaining);

    // Simulate temperature fluctuations (±2°C)
    int nozzle_var = (mock_elapsed_seconds_ % 4) - 2;
    int bed_var = (mock_elapsed_seconds_ % 6) - 3;
    set_temperatures(215 + nozzle_var, 215, 60 + bed_var, 60);
}

// Temporary wrapper for tick function (still called by main.cpp)
void ui_panel_print_status_tick_mock_print() {
    auto& panel = get_global_print_status_panel();
    panel.tick_mock_print();
}
