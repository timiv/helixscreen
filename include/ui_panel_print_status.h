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

#ifndef UI_PANEL_PRINT_STATUS_H
#define UI_PANEL_PRINT_STATUS_H

#include "lvgl/lvgl.h"

// Print state enum
typedef enum {
    PRINT_STATE_IDLE,
    PRINT_STATE_PRINTING,
    PRINT_STATE_PAUSED,
    PRINT_STATE_COMPLETE,
    PRINT_STATE_CANCELLED,
    PRINT_STATE_ERROR
} print_state_t;

// Initialize subjects for print status panel
void ui_panel_print_status_init_subjects();

// Setup event handlers after panel creation
void ui_panel_print_status_setup(lv_obj_t* panel, lv_obj_t* parent_screen);

// API to update print state (from WebSocket or mock simulation)
void ui_panel_print_status_set_filename(const char* filename);
void ui_panel_print_status_set_progress(int percent);  // 0-100
void ui_panel_print_status_set_layer(int current, int total);
void ui_panel_print_status_set_times(int elapsed_seconds, int remaining_seconds);
void ui_panel_print_status_set_temperatures(int nozzle_current, int nozzle_target,
                                            int bed_current, int bed_target);
void ui_panel_print_status_set_speeds(int speed_percent, int flow_percent);
void ui_panel_print_status_set_state(print_state_t state);

// Mock print simulation (for testing without real printer)
void ui_panel_print_status_start_mock_print(const char* filename, int total_layers, int duration_seconds);
void ui_panel_print_status_stop_mock_print();
void ui_panel_print_status_tick_mock_print();  // Call periodically to advance simulation

// Query current state
print_state_t ui_panel_print_status_get_state();
int ui_panel_print_status_get_progress();

#endif // UI_PANEL_PRINT_STATUS_H
