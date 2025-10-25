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

#pragma once

#include "lvgl/lvgl.h"

/**
 * View mode for print select panel
 */
enum class PrintSelectViewMode {
    CARD = 0,  // Card grid view (default)
    LIST = 1   // List view with columns
};

/**
 * Sort column for list view
 */
enum class PrintSelectSortColumn {
    FILENAME,
    SIZE,
    MODIFIED,
    PRINT_TIME,
    FILAMENT
};

/**
 * Sort direction
 */
enum class PrintSelectSortDirection {
    ASCENDING,
    DESCENDING
};

/**
 * Initialize print select panel subjects
 * Call this BEFORE creating XML components
 */
void ui_panel_print_select_init_subjects();

/**
 * Setup print select panel after XML creation
 * Wires up event handlers and creates detail view
 * @param panel_root The root print select panel widget
 * @param parent_screen The parent screen (for creating overlays at correct z-level)
 */
void ui_panel_print_select_setup(lv_obj_t* panel_root, lv_obj_t* parent_screen);

/**
 * Populate print select panel with test print files
 * Automatically handles both card and list views
 * @param panel_root The root print select panel widget
 */
void ui_panel_print_select_populate_test_data(lv_obj_t* panel_root);

/**
 * Toggle between card and list view
 */
void ui_panel_print_select_toggle_view();

/**
 * Sort files by specified column
 * @param column Column to sort by
 */
void ui_panel_print_select_sort_by(PrintSelectSortColumn column);

/**
 * Show delete confirmation dialog for the selected file
 */
void ui_panel_print_select_show_delete_confirmation();

/**
 * Set the selected file data (updates reactive subjects)
 * @param filename The name of the file
 * @param thumbnail_src Path to thumbnail image
 * @param print_time Formatted print time string
 * @param filament_weight Formatted filament weight string
 */
void ui_panel_print_select_set_file(const char* filename, const char* thumbnail_src,
                                    const char* print_time, const char* filament_weight);

/**
 * Show the detail view overlay
 */
void ui_panel_print_select_show_detail_view();

/**
 * Hide the detail view overlay
 */
void ui_panel_print_select_hide_detail_view();

/**
 * Set the print status panel reference for launching prints
 * @param panel The print status panel widget
 */
void ui_panel_print_select_set_print_status_panel(lv_obj_t* panel);
