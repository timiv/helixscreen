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

#include "ui_panel_controls_temp.h"
#include "ui_component_keypad.h"
#include "ui_component_header_bar.h"
#include "ui_utils.h"
#include "ui_nav.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>

// Temperature subjects (reactive data binding)
static lv_subject_t nozzle_current_subject;
static lv_subject_t nozzle_target_subject;
static lv_subject_t bed_current_subject;
static lv_subject_t bed_target_subject;
static lv_subject_t nozzle_display_subject;
static lv_subject_t bed_display_subject;

// Subject storage buffers
static char nozzle_current_buf[16];
static char nozzle_target_buf[16];
static char bed_current_buf[16];
static char bed_target_buf[16];
static char nozzle_display_buf[32];
static char bed_display_buf[32];

// Current temperature state
static int nozzle_current = 25;
static int nozzle_target = 0;
static int bed_current = 25;
static int bed_target = 0;

// Temperature limits (can be updated from Moonraker heater config)
static int nozzle_min_temp = 0;
static int nozzle_max_temp = 500;  // Safe default for most hotends
static int bed_min_temp = 0;
static int bed_max_temp = 150;     // Safe default for most heatbeds

// Panel widgets
static lv_obj_t* nozzle_panel = nullptr;
static lv_obj_t* bed_panel = nullptr;
static lv_obj_t* parent_obj = nullptr;

// Forward declarations for callbacks
static void update_nozzle_display();
static void update_bed_display();

void ui_panel_controls_temp_init_subjects() {
    // Initialize temperature subjects with default values
    snprintf(nozzle_current_buf, sizeof(nozzle_current_buf), "%d°C", nozzle_current);
    snprintf(nozzle_target_buf, sizeof(nozzle_target_buf), "%d°C", nozzle_target);
    snprintf(bed_current_buf, sizeof(bed_current_buf), "%d°C", bed_current);
    snprintf(bed_target_buf, sizeof(bed_target_buf), "%d°C", bed_target);
    snprintf(nozzle_display_buf, sizeof(nozzle_display_buf), "%d / %d°C", nozzle_current, nozzle_target);
    snprintf(bed_display_buf, sizeof(bed_display_buf), "%d / %d°C", bed_current, bed_target);

    lv_subject_init_string(&nozzle_current_subject, nozzle_current_buf, nullptr, sizeof(nozzle_current_buf), nozzle_current_buf);
    lv_subject_init_string(&nozzle_target_subject, nozzle_target_buf, nullptr, sizeof(nozzle_target_buf), nozzle_target_buf);
    lv_subject_init_string(&bed_current_subject, bed_current_buf, nullptr, sizeof(bed_current_buf), bed_current_buf);
    lv_subject_init_string(&bed_target_subject, bed_target_buf, nullptr, sizeof(bed_target_buf), bed_target_buf);
    lv_subject_init_string(&nozzle_display_subject, nozzle_display_buf, nullptr, sizeof(nozzle_display_buf), nozzle_display_buf);
    lv_subject_init_string(&bed_display_subject, bed_display_buf, nullptr, sizeof(bed_display_buf), bed_display_buf);

    // Register subjects with XML system (global scope)
    lv_xml_register_subject(NULL, "nozzle_current_temp", &nozzle_current_subject);
    lv_xml_register_subject(NULL, "nozzle_target_temp", &nozzle_target_subject);
    lv_xml_register_subject(NULL, "bed_current_temp", &bed_current_subject);
    lv_xml_register_subject(NULL, "bed_target_temp", &bed_target_subject);
    lv_xml_register_subject(NULL, "nozzle_temp_display", &nozzle_display_subject);
    lv_xml_register_subject(NULL, "bed_temp_display", &bed_display_subject);

    printf("[Temp] Subjects initialized: nozzle=%d/%d°C, bed=%d/%d°C\n",
           nozzle_current, nozzle_target, bed_current, bed_target);
}

// Update nozzle display text
static void update_nozzle_display() {
    snprintf(nozzle_display_buf, sizeof(nozzle_display_buf), "%d / %d°C", nozzle_current, nozzle_target);
    lv_subject_copy_string(&nozzle_display_subject, nozzle_display_buf);
}

// Update bed display text
static void update_bed_display() {
    snprintf(bed_display_buf, sizeof(bed_display_buf), "%d / %d°C", bed_current, bed_target);
    lv_subject_copy_string(&bed_display_subject, bed_display_buf);
}

// ============================================================================
// NOZZLE TEMPERATURE PANEL
// ============================================================================

// Event handler: Back button (nozzle panel)
static void nozzle_back_button_cb(lv_event_t* e) {
    (void)e;

    // Use navigation history to go back to previous panel
    if (!ui_nav_go_back()) {
        // Fallback: If navigation history is empty, manually hide and show controls launcher
        if (nozzle_panel) {
            lv_obj_add_flag(nozzle_panel, LV_OBJ_FLAG_HIDDEN);
        }

        if (parent_obj) {
            lv_obj_t* controls_launcher = lv_obj_find_by_name(parent_obj, "controls_panel");
            if (controls_launcher) {
                lv_obj_clear_flag(controls_launcher, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Event handler: Confirm button (nozzle panel)
static void nozzle_confirm_button_cb(lv_event_t* e) {
    (void)e;
    printf("[Temp] Nozzle temperature confirmed: %d°C\n", nozzle_target);

    // TODO: Send command to printer (moonraker_set_nozzle_temp(nozzle_target))

    // Return to launcher
    nozzle_back_button_cb(e);
}

// Event handler: Nozzle preset buttons
static void nozzle_preset_button_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* name = lv_obj_get_name(btn);

    if (!name) return;

    int temp = 0;
    if (strcmp(name, "preset_off") == 0) {
        temp = 0;
    } else if (strcmp(name, "preset_pla") == 0) {
        temp = 210;
    } else if (strcmp(name, "preset_petg") == 0) {
        temp = 240;
    } else if (strcmp(name, "preset_abs") == 0) {
        temp = 250;
    }

    nozzle_target = temp;
    update_nozzle_display();
    printf("[Temp] Nozzle target set to %d°C via preset\n", temp);
}

// Callback for nozzle custom temperature input
static void nozzle_custom_callback(float value, void* user_data) {
    (void)user_data;
    nozzle_target = (int)value;
    update_nozzle_display();
    printf("[Temp] Nozzle target set to %d°C via custom input\n", nozzle_target);
}

// Event handler: Nozzle custom button
static void nozzle_custom_button_cb(lv_event_t* e) {
    (void)e;
    printf("[Temp] Opening keypad for nozzle custom temperature\n");

    ui_keypad_config_t config = {
        .initial_value = (float)nozzle_target,
        .min_value = 0.0f,
        .max_value = 350.0f,
        .title_label = "Nozzle Temp",
        .unit_label = "°C",
        .allow_decimal = false,
        .allow_negative = false,
        .callback = nozzle_custom_callback,
        .user_data = nullptr
    };

    ui_keypad_show(&config);
}

// Resize callback for responsive padding (nozzle panel)
static void nozzle_on_resize() {
    if (!nozzle_panel || !parent_obj) {
        return;
    }

    lv_obj_t* temp_content = lv_obj_find_by_name(nozzle_panel, "temp_content");
    if (temp_content) {
        lv_coord_t padding = ui_get_header_content_padding(lv_obj_get_height(parent_obj));
        lv_obj_set_style_pad_all(temp_content, padding, 0);
    }
}

void ui_panel_controls_temp_nozzle_setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    nozzle_panel = panel;
    parent_obj = parent_screen;

    printf("[Temp] Setting up nozzle panel event handlers...\n");

    // Setup header for responsive height
    lv_obj_t* nozzle_temp_header = lv_obj_find_by_name(panel, "nozzle_temp_header");
    if (nozzle_temp_header) {
        ui_component_header_bar_setup(nozzle_temp_header, parent_screen);
    }

    // Set responsive padding for content area
    lv_obj_t* temp_content = lv_obj_find_by_name(panel, "temp_content");
    if (temp_content) {
        lv_coord_t padding = ui_get_header_content_padding(lv_obj_get_height(parent_screen));
        lv_obj_set_style_pad_all(temp_content, padding, 0);
        printf("[Temp]   ✓ Content padding: %dpx (responsive)\n", padding);
    }

    // Register resize callback
    ui_resize_handler_register(nozzle_on_resize);

    // Back button
    lv_obj_t* back_btn = lv_obj_find_by_name(panel, "back_button");
    if (back_btn) {
        lv_obj_add_event_cb(back_btn, nozzle_back_button_cb, LV_EVENT_CLICKED, nullptr);
        printf("[Temp]   ✓ Back button\n");
    }

    // Show and wire confirm button
    lv_obj_t* header = lv_obj_find_by_name(panel, "nozzle_temp_header");
    if (header && ui_header_bar_show_right_button(header)) {
        lv_obj_t* confirm_btn = lv_obj_find_by_name(header, "right_button");
        if (confirm_btn) {
            lv_obj_add_event_cb(confirm_btn, nozzle_confirm_button_cb, LV_EVENT_CLICKED, nullptr);
            printf("[Temp]   ✓ Confirm button\n");
        }
    }

    // Preset buttons
    const char* preset_names[] = {"preset_off", "preset_pla", "preset_petg", "preset_abs"};
    for (const char* name : preset_names) {
        lv_obj_t* btn = lv_obj_find_by_name(panel, name);
        if (btn) {
            lv_obj_add_event_cb(btn, nozzle_preset_button_cb, LV_EVENT_CLICKED, nullptr);
        }
    }
    printf("[Temp]   ✓ Preset buttons (4)\n");

    // Custom button
    lv_obj_t* custom_btn = lv_obj_find_by_name(panel, "btn_custom");
    if (custom_btn) {
        lv_obj_add_event_cb(custom_btn, nozzle_custom_button_cb, LV_EVENT_CLICKED, nullptr);
        printf("[Temp]   ✓ Custom button\n");
    }

    printf("[Temp] Nozzle panel setup complete!\n");
}

// ============================================================================
// BED TEMPERATURE PANEL
// ============================================================================

// Event handler: Back button (bed panel)
static void bed_back_button_cb(lv_event_t* e) {
    (void)e;

    // Use navigation history to go back to previous panel
    if (!ui_nav_go_back()) {
        // Fallback: If navigation history is empty, manually hide and show controls launcher
        if (bed_panel) {
            lv_obj_add_flag(bed_panel, LV_OBJ_FLAG_HIDDEN);
        }

        if (parent_obj) {
            lv_obj_t* controls_launcher = lv_obj_find_by_name(parent_obj, "controls_panel");
            if (controls_launcher) {
                lv_obj_clear_flag(controls_launcher, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Event handler: Confirm button (bed panel)
static void bed_confirm_button_cb(lv_event_t* e) {
    (void)e;
    printf("[Temp] Bed temperature confirmed: %d°C\n", bed_target);

    // TODO: Send command to printer (moonraker_set_bed_temp(bed_target))

    // Return to launcher
    bed_back_button_cb(e);
}

// Event handler: Bed preset buttons
static void bed_preset_button_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* name = lv_obj_get_name(btn);

    if (!name) return;

    int temp = 0;
    if (strcmp(name, "preset_off") == 0) {
        temp = 0;
    } else if (strcmp(name, "preset_pla") == 0) {
        temp = 60;
    } else if (strcmp(name, "preset_petg") == 0) {
        temp = 80;
    } else if (strcmp(name, "preset_abs") == 0) {
        temp = 100;
    }

    bed_target = temp;
    update_bed_display();
    printf("[Temp] Bed target set to %d°C via preset\n", temp);
}

// Callback for bed custom temperature input
static void bed_custom_callback(float value, void* user_data) {
    (void)user_data;
    bed_target = (int)value;
    update_bed_display();
    printf("[Temp] Bed target set to %d°C via custom input\n", bed_target);
}

// Event handler: Bed custom button
static void bed_custom_button_cb(lv_event_t* e) {
    (void)e;
    printf("[Temp] Opening keypad for bed custom temperature\n");

    ui_keypad_config_t config = {
        .initial_value = (float)bed_target,
        .min_value = 0.0f,
        .max_value = 150.0f,
        .title_label = "Heat Bed Temp",
        .unit_label = "°C",
        .allow_decimal = false,
        .allow_negative = false,
        .callback = bed_custom_callback,
        .user_data = nullptr
    };

    ui_keypad_show(&config);
}

// Resize callback for responsive padding (bed panel)
static void bed_on_resize() {
    if (!bed_panel || !parent_obj) {
        return;
    }

    lv_obj_t* temp_content = lv_obj_find_by_name(bed_panel, "temp_content");
    if (temp_content) {
        lv_coord_t padding = ui_get_header_content_padding(lv_obj_get_height(parent_obj));
        lv_obj_set_style_pad_all(temp_content, padding, 0);
    }
}

void ui_panel_controls_temp_bed_setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    bed_panel = panel;
    parent_obj = parent_screen;

    printf("[Temp] Setting up bed panel event handlers...\n");

    // Setup header for responsive height
    lv_obj_t* bed_temp_header = lv_obj_find_by_name(panel, "bed_temp_header");
    if (bed_temp_header) {
        ui_component_header_bar_setup(bed_temp_header, parent_screen);
    }

    // Set responsive padding for content area
    lv_obj_t* temp_content = lv_obj_find_by_name(panel, "temp_content");
    if (temp_content) {
        lv_coord_t padding = ui_get_header_content_padding(lv_obj_get_height(parent_screen));
        lv_obj_set_style_pad_all(temp_content, padding, 0);
        printf("[Temp]   ✓ Content padding: %dpx (responsive)\n", padding);
    }

    // Register resize callback
    ui_resize_handler_register(bed_on_resize);

    // Back button
    lv_obj_t* back_btn = lv_obj_find_by_name(panel, "back_button");
    if (back_btn) {
        lv_obj_add_event_cb(back_btn, bed_back_button_cb, LV_EVENT_CLICKED, nullptr);
        printf("[Temp]   ✓ Back button\n");
    }

    // Show and wire confirm button
    lv_obj_t* header = lv_obj_find_by_name(panel, "bed_temp_header");
    if (header && ui_header_bar_show_right_button(header)) {
        lv_obj_t* confirm_btn = lv_obj_find_by_name(header, "right_button");
        if (confirm_btn) {
            lv_obj_add_event_cb(confirm_btn, bed_confirm_button_cb, LV_EVENT_CLICKED, nullptr);
            printf("[Temp]   ✓ Confirm button\n");
        }
    }

    // Preset buttons
    const char* preset_names[] = {"preset_off", "preset_pla", "preset_petg", "preset_abs"};
    for (const char* name : preset_names) {
        lv_obj_t* btn = lv_obj_find_by_name(panel, name);
        if (btn) {
            lv_obj_add_event_cb(btn, bed_preset_button_cb, LV_EVENT_CLICKED, nullptr);
        }
    }
    printf("[Temp]   ✓ Preset buttons (4)\n");

    // Custom button
    lv_obj_t* custom_btn = lv_obj_find_by_name(panel, "btn_custom");
    if (custom_btn) {
        lv_obj_add_event_cb(custom_btn, bed_custom_button_cb, LV_EVENT_CLICKED, nullptr);
        printf("[Temp]   ✓ Custom button\n");
    }

    printf("[Temp] Bed panel setup complete!\n");
}

// ============================================================================
// PUBLIC API
// ============================================================================

void ui_panel_controls_temp_set_nozzle(int current, int target) {
    // Validate temperature ranges using dynamic limits
    if (current < nozzle_min_temp || current > nozzle_max_temp) {
        printf("[Temp] WARNING: Invalid nozzle current temperature %d°C (valid: %d-%d°C), clamping\n",
               current, nozzle_min_temp, nozzle_max_temp);
        current = (current < nozzle_min_temp) ? nozzle_min_temp : nozzle_max_temp;
    }
    if (target < nozzle_min_temp || target > nozzle_max_temp) {
        printf("[Temp] WARNING: Invalid nozzle target temperature %d°C (valid: %d-%d°C), clamping\n",
               target, nozzle_min_temp, nozzle_max_temp);
        target = (target < nozzle_min_temp) ? nozzle_min_temp : nozzle_max_temp;
    }

    nozzle_current = current;
    nozzle_target = target;
    update_nozzle_display();
}

void ui_panel_controls_temp_set_bed(int current, int target) {
    // Validate temperature ranges using dynamic limits
    if (current < bed_min_temp || current > bed_max_temp) {
        printf("[Temp] WARNING: Invalid bed current temperature %d°C (valid: %d-%d°C), clamping\n",
               current, bed_min_temp, bed_max_temp);
        current = (current < bed_min_temp) ? bed_min_temp : bed_max_temp;
    }
    if (target < bed_min_temp || target > bed_max_temp) {
        printf("[Temp] WARNING: Invalid bed target temperature %d°C (valid: %d-%d°C), clamping\n",
               target, bed_min_temp, bed_max_temp);
        target = (target < bed_min_temp) ? bed_min_temp : bed_max_temp;
    }

    bed_current = current;
    bed_target = target;
    update_bed_display();
}

int ui_panel_controls_temp_get_nozzle_target() {
    return nozzle_target;
}

int ui_panel_controls_temp_get_bed_target() {
    return bed_target;
}

void ui_panel_controls_temp_set_nozzle_limits(int min_temp, int max_temp) {
    nozzle_min_temp = min_temp;
    nozzle_max_temp = max_temp;
    printf("[Temp] Nozzle temperature limits updated: %d-%d°C\n", min_temp, max_temp);
}

void ui_panel_controls_temp_set_bed_limits(int min_temp, int max_temp) {
    bed_min_temp = min_temp;
    bed_max_temp = max_temp;
    printf("[Temp] Bed temperature limits updated: %d-%d°C\n", min_temp, max_temp);
}
