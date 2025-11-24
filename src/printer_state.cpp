// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "printer_state.h"

#include <cstring>

PrinterState::PrinterState() {
    // Initialize string buffers
    std::memset(print_filename_buf_, 0, sizeof(print_filename_buf_));
    std::memset(print_state_buf_, 0, sizeof(print_state_buf_));
    std::memset(homed_axes_buf_, 0, sizeof(homed_axes_buf_));
    std::memset(printer_connection_message_buf_, 0, sizeof(printer_connection_message_buf_));

    // Set default values
    std::strcpy(print_state_buf_, "standby");
    std::strcpy(homed_axes_buf_, "");
    std::strcpy(printer_connection_message_buf_, "Disconnected");
}

PrinterState::~PrinterState() {}

void PrinterState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterState] reset_for_testing: subjects not initialized, nothing to reset");
        return;  // Nothing to reset
    }

    spdlog::info("[PrinterState] reset_for_testing: Deinitializing subjects to clear observers");
    // Deinitialize all subjects to clear observers
    lv_subject_deinit(&extruder_temp_);
    lv_subject_deinit(&extruder_target_);
    lv_subject_deinit(&bed_temp_);
    lv_subject_deinit(&bed_target_);
    lv_subject_deinit(&print_progress_);
    lv_subject_deinit(&print_filename_);
    lv_subject_deinit(&print_state_);
    lv_subject_deinit(&position_x_);
    lv_subject_deinit(&position_y_);
    lv_subject_deinit(&position_z_);
    lv_subject_deinit(&homed_axes_);
    lv_subject_deinit(&speed_factor_);
    lv_subject_deinit(&flow_factor_);
    lv_subject_deinit(&fan_speed_);
    lv_subject_deinit(&printer_connection_state_);
    lv_subject_deinit(&printer_connection_message_);
    lv_subject_deinit(&network_status_);

    subjects_initialized_ = false;
}

void PrinterState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("Printer state subjects already initialized, skipping");
        return;
    }

    spdlog::debug("Initializing printer state subjects (register_xml={})", register_xml);

    // Temperature subjects (integer, degrees Celsius)
    lv_subject_init_int(&extruder_temp_, 0);
    lv_subject_init_int(&extruder_target_, 0);
    lv_subject_init_int(&bed_temp_, 0);
    lv_subject_init_int(&bed_target_, 0);

    // Print progress subjects
    lv_subject_init_int(&print_progress_, 0);
    lv_subject_init_string(&print_filename_, print_filename_buf_, nullptr,
                           sizeof(print_filename_buf_), "");
    lv_subject_init_string(&print_state_, print_state_buf_, nullptr, sizeof(print_state_buf_),
                           "standby");

    // Motion subjects
    lv_subject_init_int(&position_x_, 0);
    lv_subject_init_int(&position_y_, 0);
    lv_subject_init_int(&position_z_, 0);
    lv_subject_init_string(&homed_axes_, homed_axes_buf_, nullptr, sizeof(homed_axes_buf_), "");

    // Speed/Flow subjects (percentages)
    lv_subject_init_int(&speed_factor_, 100);
    lv_subject_init_int(&flow_factor_, 100);
    lv_subject_init_int(&fan_speed_, 0);

    // Printer connection state subjects (Moonraker WebSocket)
    lv_subject_init_int(&printer_connection_state_, 0); // 0 = disconnected
    lv_subject_init_string(&printer_connection_message_, printer_connection_message_buf_, nullptr,
                           sizeof(printer_connection_message_buf_), "Disconnected");

    // Network connectivity subject (WiFi/Ethernet)
    // TODO: Get actual network status from EthernetManager/WiFiManager
    lv_subject_init_int(&network_status_, 2); // 0=DISCONNECTED, 1=CONNECTING, 2=CONNECTED (mock mode default)

    // Register all subjects with LVGL XML system (CRITICAL for XML bindings)
    if (register_xml) {
        spdlog::debug("[PrinterState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "extruder_temp", &extruder_temp_);
        lv_xml_register_subject(NULL, "extruder_target", &extruder_target_);
        lv_xml_register_subject(NULL, "bed_temp", &bed_temp_);
        lv_xml_register_subject(NULL, "bed_target", &bed_target_);
        lv_xml_register_subject(NULL, "print_progress", &print_progress_);
        lv_xml_register_subject(NULL, "print_filename", &print_filename_);
        lv_xml_register_subject(NULL, "print_state", &print_state_);
        lv_xml_register_subject(NULL, "position_x", &position_x_);
        lv_xml_register_subject(NULL, "position_y", &position_y_);
        lv_xml_register_subject(NULL, "position_z", &position_z_);
        lv_xml_register_subject(NULL, "homed_axes", &homed_axes_);
        lv_xml_register_subject(NULL, "speed_factor", &speed_factor_);
        lv_xml_register_subject(NULL, "flow_factor", &flow_factor_);
        lv_xml_register_subject(NULL, "fan_speed", &fan_speed_);
        lv_xml_register_subject(NULL, "printer_connection_state", &printer_connection_state_);
        lv_xml_register_subject(NULL, "printer_connection_message", &printer_connection_message_);
        lv_xml_register_subject(NULL, "network_status", &network_status_);
    } else {
        spdlog::debug("[PrinterState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("Printer state subjects initialized and registered successfully");
}

void PrinterState::update_from_notification(const json& notification) {
    std::lock_guard<std::mutex> lock(state_mutex_);

    // Moonraker notifications have structure:
    // {"method": "notify_status_update", "params": [{...printer state...}, eventtime]}

    if (!notification.contains("method") || !notification.contains("params")) {
        return;
    }

    std::string method = notification["method"].get<std::string>();
    if (method != "notify_status_update") {
        return;
    }

    // Extract printer state from params[0]
    auto params = notification["params"];
    if (params.is_array() && !params.empty()) {
        const auto& state = params[0];

        // Update extruder temperature
        if (state.contains("extruder")) {
            const auto& extruder = state["extruder"];

            if (extruder.contains("temperature")) {
                int temp = static_cast<int>(extruder["temperature"].get<double>());
                lv_subject_set_int(&extruder_temp_, temp);
            }

            if (extruder.contains("target")) {
                int target = static_cast<int>(extruder["target"].get<double>());
                lv_subject_set_int(&extruder_target_, target);
            }
        }

        // Update bed temperature
        if (state.contains("heater_bed")) {
            const auto& bed = state["heater_bed"];

            if (bed.contains("temperature")) {
                int temp = static_cast<int>(bed["temperature"].get<double>());
                lv_subject_set_int(&bed_temp_, temp);
            }

            if (bed.contains("target")) {
                int target = static_cast<int>(bed["target"].get<double>());
                lv_subject_set_int(&bed_target_, target);
            }
        }

        // Update print progress
        if (state.contains("virtual_sdcard")) {
            const auto& sdcard = state["virtual_sdcard"];

            if (sdcard.contains("progress")) {
                double progress = sdcard["progress"].get<double>();
                int progress_pct = static_cast<int>(progress * 100.0);
                lv_subject_set_int(&print_progress_, progress_pct);
            }
        }

        // Update print state
        if (state.contains("print_stats")) {
            const auto& stats = state["print_stats"];

            if (stats.contains("state")) {
                std::string state_str = stats["state"].get<std::string>();
                lv_subject_copy_string(&print_state_, state_str.c_str());
            }

            if (stats.contains("filename")) {
                std::string filename = stats["filename"].get<std::string>();
                lv_subject_copy_string(&print_filename_, filename.c_str());
            }
        }

        // Update toolhead position
        if (state.contains("toolhead")) {
            const auto& toolhead = state["toolhead"];

            if (toolhead.contains("position") && toolhead["position"].is_array()) {
                const auto& pos = toolhead["position"];
                if (pos.size() >= 3) {
                    lv_subject_set_int(&position_x_, static_cast<int>(pos[0].get<double>()));
                    lv_subject_set_int(&position_y_, static_cast<int>(pos[1].get<double>()));
                    lv_subject_set_int(&position_z_, static_cast<int>(pos[2].get<double>()));
                }
            }

            if (toolhead.contains("homed_axes")) {
                std::string axes = toolhead["homed_axes"].get<std::string>();
                lv_subject_copy_string(&homed_axes_, axes.c_str());
            }
        }

        // Update speed factor
        if (state.contains("gcode_move")) {
            const auto& gcode_move = state["gcode_move"];

            if (gcode_move.contains("speed_factor")) {
                double factor = gcode_move["speed_factor"].get<double>();
                int factor_pct = static_cast<int>(factor * 100.0);
                lv_subject_set_int(&speed_factor_, factor_pct);
            }

            if (gcode_move.contains("extrude_factor")) {
                double factor = gcode_move["extrude_factor"].get<double>();
                int factor_pct = static_cast<int>(factor * 100.0);
                lv_subject_set_int(&flow_factor_, factor_pct);
            }
        }

        // Update fan speed
        if (state.contains("fan")) {
            const auto& fan = state["fan"];

            if (fan.contains("speed")) {
                double speed = fan["speed"].get<double>();
                int speed_pct = static_cast<int>(speed * 100.0);
                lv_subject_set_int(&fan_speed_, speed_pct);
            }
        }

        // Cache full state for complex queries
        json_state_.merge_patch(state);
    }
}

json& PrinterState::get_json_state() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return json_state_;
}

void PrinterState::set_printer_connection_state(int state, const char* message) {
    spdlog::info("[PrinterState] Printer connection state changed: {} - {}", state, message);
    spdlog::debug("[PrinterState] Setting printer_connection_state_ subject (at {}) to value {}",
                  (void*)&printer_connection_state_, state);
    lv_subject_set_int(&printer_connection_state_, state);
    spdlog::debug("[PrinterState] Subject value now: {}", lv_subject_get_int(&printer_connection_state_));
    lv_subject_copy_string(&printer_connection_message_, message);
    spdlog::debug("[PrinterState] Printer connection state update complete, observers should be notified");
}

void PrinterState::set_network_status(int status) {
    spdlog::debug("[PrinterState] Network status changed: {}", status);
    lv_subject_set_int(&network_status_, status);
}
