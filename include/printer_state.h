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

#ifndef PRINTER_STATE_H
#define PRINTER_STATE_H

#include "lvgl/lvgl.h"
#include "hv/json.hpp"  // libhv's nlohmann json (via cpputil/)
#include "spdlog/spdlog.h"

#include <string>
#include <mutex>

using json = nlohmann::json;

/**
 * @brief Printer state manager with LVGL 9 reactive subjects
 *
 * Implements hybrid architecture:
 * - LVGL subjects for UI-bound data (automatic reactive updates)
 * - JSON cache for complex data (file lists, capabilities, metadata)
 *
 * All subjects are thread-safe and automatically update bound UI widgets.
 */
class PrinterState {
public:
  PrinterState();
  ~PrinterState();

  /**
   * @brief Initialize all LVGL subjects
   *
   * MUST be called BEFORE creating XML components that bind to these subjects.
   */
  void init_subjects();

  /**
   * @brief Update state from Moonraker notification
   *
   * Extracts values from notify_status_update messages and updates subjects.
   * Also maintains JSON cache for complex data.
   *
   * @param notification Parsed JSON notification from Moonraker
   */
  void update_from_notification(const json& notification);

  /**
   * @brief Get raw JSON state for complex queries
   *
   * Thread-safe access to cached printer state.
   *
   * @return Reference to JSON state object
   */
  json& get_json_state();

  //
  // Subject accessors for XML binding
  //

  // Temperature subjects (integer, degrees Celsius)
  lv_subject_t* get_extruder_temp_subject() { return &extruder_temp_; }
  lv_subject_t* get_extruder_target_subject() { return &extruder_target_; }
  lv_subject_t* get_bed_temp_subject() { return &bed_temp_; }
  lv_subject_t* get_bed_target_subject() { return &bed_target_; }

  // Print progress subjects
  lv_subject_t* get_print_progress_subject() { return &print_progress_; }  // 0-100
  lv_subject_t* get_print_filename_subject() { return &print_filename_; }
  lv_subject_t* get_print_state_subject() { return &print_state_; }  // "standby", "printing", "paused", "complete"

  // Motion subjects
  lv_subject_t* get_position_x_subject() { return &position_x_; }
  lv_subject_t* get_position_y_subject() { return &position_y_; }
  lv_subject_t* get_position_z_subject() { return &position_z_; }
  lv_subject_t* get_homed_axes_subject() { return &homed_axes_; }  // "xyz", "xy", etc.

  // Speed/Flow subjects (percentages, 0-100)
  lv_subject_t* get_speed_factor_subject() { return &speed_factor_; }
  lv_subject_t* get_flow_factor_subject() { return &flow_factor_; }
  lv_subject_t* get_fan_speed_subject() { return &fan_speed_; }

  // Connection state subjects
  lv_subject_t* get_connection_state_subject() { return &connection_state_; }  // 0=disconnected, 1=connecting, 2=connected, 3=reconnecting, 4=failed
  lv_subject_t* get_connection_message_subject() { return &connection_message_; }  // Status message

  /**
   * @brief Set connection state
   *
   * Updates both connection_state and connection_message subjects.
   * Called by main.cpp WebSocket callbacks.
   *
   * @param state 0=disconnected, 1=connecting, 2=connected, 3=reconnecting, 4=failed
   * @param message Status message ("Connecting...", "Ready", "Disconnected", etc.)
   */
  void set_connection_state(int state, const char* message);

private:
  // Temperature subjects
  lv_subject_t extruder_temp_;
  lv_subject_t extruder_target_;
  lv_subject_t bed_temp_;
  lv_subject_t bed_target_;

  // Print progress subjects
  lv_subject_t print_progress_;      // Integer 0-100
  lv_subject_t print_filename_;      // String buffer
  lv_subject_t print_state_;         // String buffer

  // Motion subjects
  lv_subject_t position_x_;
  lv_subject_t position_y_;
  lv_subject_t position_z_;
  lv_subject_t homed_axes_;          // String buffer

  // Speed/Flow subjects
  lv_subject_t speed_factor_;
  lv_subject_t flow_factor_;
  lv_subject_t fan_speed_;

  // Connection state subjects
  lv_subject_t connection_state_;    // Integer: 0=disconnected, 1=connecting, 2=connected, 3=reconnecting, 4=failed
  lv_subject_t connection_message_;  // String buffer

  // String buffers for subject storage
  char print_filename_buf_[256];
  char print_state_buf_[32];
  char homed_axes_buf_[8];
  char connection_message_buf_[128];

  // JSON cache for complex data
  json json_state_;
  std::mutex state_mutex_;
};

#endif // PRINTER_STATE_H
