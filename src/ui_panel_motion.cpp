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

#include "ui_panel_motion.h"
#include "ui_component_header_bar.h"
#include "ui_nav.h"
#include "ui_utils.h"
#include "ui_theme.h"
#include <stdio.h>
#include <string.h>

// Position subjects (reactive data binding)
static lv_subject_t pos_x_subject;
static lv_subject_t pos_y_subject;
static lv_subject_t pos_z_subject;

// Subject storage buffers
static char pos_x_buf[32];
static char pos_y_buf[32];
static char pos_z_buf[32];

// Current state
static jog_distance_t current_distance = JOG_DIST_1MM;
static float current_x = 0.0f;
static float current_y = 0.0f;
static float current_z = 0.0f;

// Panel widgets (accessed by name)
static lv_obj_t* motion_panel = nullptr;
static lv_obj_t* parent_obj = nullptr;

// Distance button widgets
static lv_obj_t* dist_buttons[4] = {nullptr};

// Distance values in mm
static const float distance_values[] = {0.1f, 1.0f, 10.0f, 100.0f};

void ui_panel_motion_init_subjects() {
    // Initialize position subjects with default placeholder values
    snprintf(pos_x_buf, sizeof(pos_x_buf), "X:    --  mm");
    snprintf(pos_y_buf, sizeof(pos_y_buf), "Y:    --  mm");
    snprintf(pos_z_buf, sizeof(pos_z_buf), "Z:    --  mm");

    lv_subject_init_string(&pos_x_subject, pos_x_buf, nullptr, sizeof(pos_x_buf), pos_x_buf);
    lv_subject_init_string(&pos_y_subject, pos_y_buf, nullptr, sizeof(pos_y_buf), pos_y_buf);
    lv_subject_init_string(&pos_z_subject, pos_z_buf, nullptr, sizeof(pos_z_buf), pos_z_buf);

    // Register subjects with XML system (using NULL parent = global scope)
    lv_xml_register_subject(NULL, "motion_pos_x", &pos_x_subject);
    lv_xml_register_subject(NULL, "motion_pos_y", &pos_y_subject);
    lv_xml_register_subject(NULL, "motion_pos_z", &pos_z_subject);

    printf("[Motion] Subjects initialized: X/Y/Z position displays\n");
}

// Helper: Update distance button styling
static void update_distance_buttons() {
    for (int i = 0; i < 4; i++) {
        if (dist_buttons[i]) {
            lv_color_t bg_color = (i == current_distance) ?
                lv_color_hex(0xcc3333) :  // Active: softer red
                lv_color_hex(0x505050);   // Inactive: lighter gray for visibility
            lv_obj_set_style_bg_color(dist_buttons[i], bg_color, 0);
        }
    }
}

// Event handler: Back button
static void back_button_cb(lv_event_t* e) {
    (void)e;  // Unused parameter

    // Use navigation history to go back to previous panel
    if (!ui_nav_go_back()) {
        // Fallback: If navigation history is empty, manually hide and show controls launcher
        if (motion_panel) {
            lv_obj_add_flag(motion_panel, LV_OBJ_FLAG_HIDDEN);
        }

        if (parent_obj) {
            lv_obj_t* controls_launcher = lv_obj_find_by_name(parent_obj, "controls_panel");
            if (controls_launcher) {
                lv_obj_clear_flag(controls_launcher, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Event handler: Distance selector buttons
static void distance_button_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

    // Find which button was clicked
    for (int i = 0; i < 4; i++) {
        if (btn == dist_buttons[i]) {
            current_distance = (jog_distance_t)i;
            printf("[Motion] Distance selected: %.1fmm\n", distance_values[i]);
            update_distance_buttons();
            return;
        }
    }
}

// Event handler: Jog buttons (8 directions + center home)
static void jog_button_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* name = lv_obj_get_name(btn);

    if (!name) return;

    float dist = distance_values[current_distance];

    // Map button name to jog direction
    if (strcmp(name, "jog_n") == 0) {
        ui_panel_motion_jog(JOG_DIR_N, dist);
    } else if (strcmp(name, "jog_s") == 0) {
        ui_panel_motion_jog(JOG_DIR_S, dist);
    } else if (strcmp(name, "jog_e") == 0) {
        ui_panel_motion_jog(JOG_DIR_E, dist);
    } else if (strcmp(name, "jog_w") == 0) {
        ui_panel_motion_jog(JOG_DIR_W, dist);
    } else if (strcmp(name, "jog_ne") == 0) {
        ui_panel_motion_jog(JOG_DIR_NE, dist);
    } else if (strcmp(name, "jog_nw") == 0) {
        ui_panel_motion_jog(JOG_DIR_NW, dist);
    } else if (strcmp(name, "jog_se") == 0) {
        ui_panel_motion_jog(JOG_DIR_SE, dist);
    } else if (strcmp(name, "jog_sw") == 0) {
        ui_panel_motion_jog(JOG_DIR_SW, dist);
    } else if (strcmp(name, "jog_home_xy") == 0) {
        ui_panel_motion_home('A');  // Home X and Y
    }
}

// Event handler: Z-axis buttons
static void z_button_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* name = lv_obj_get_name(btn);

    printf("[Motion] Z button callback fired! Button name: '%s'\n", name ? name : "(null)");

    if (!name) {
        printf("[Motion]   ERROR: Button has no name!\n");
        return;
    }

    if (strcmp(name, "z_up_10") == 0) {
        ui_panel_motion_set_position(current_x, current_y, current_z + 10.0f);
        printf("[Motion] Z jog: +10mm (now %.1fmm)\n", current_z);
    } else if (strcmp(name, "z_up_1") == 0) {
        ui_panel_motion_set_position(current_x, current_y, current_z + 1.0f);
        printf("[Motion] Z jog: +1mm (now %.1fmm)\n", current_z);
    } else if (strcmp(name, "z_down_1") == 0) {
        ui_panel_motion_set_position(current_x, current_y, current_z - 1.0f);
        printf("[Motion] Z jog: -1mm (now %.1fmm)\n", current_z);
    } else if (strcmp(name, "z_down_10") == 0) {
        ui_panel_motion_set_position(current_x, current_y, current_z - 10.0f);
        printf("[Motion] Z jog: -10mm (now %.1fmm)\n", current_z);
    } else {
        printf("[Motion]   ERROR: Unknown button name: '%s'\n", name);
    }
}

// Event handler: Home buttons
static void home_button_cb(lv_event_t* e) {
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
    const char* name = lv_obj_get_name(btn);

    if (!name) return;

    if (strcmp(name, "home_all") == 0) {
        ui_panel_motion_home('A');
    } else if (strcmp(name, "home_x") == 0) {
        ui_panel_motion_home('X');
    } else if (strcmp(name, "home_y") == 0) {
        ui_panel_motion_home('Y');
    } else if (strcmp(name, "home_z") == 0) {
        ui_panel_motion_home('Z');
    }
}

// Resize callback for responsive padding
static void on_resize() {
    if (!motion_panel || !parent_obj) {
        return;
    }

    lv_obj_t* motion_content = lv_obj_find_by_name(motion_panel, "motion_content");
    if (motion_content) {
        lv_coord_t vertical_padding = ui_get_header_content_padding(lv_obj_get_height(parent_obj));
        // Set vertical padding (top/bottom) responsively, keep horizontal at medium (12px)
        lv_obj_set_style_pad_top(motion_content, vertical_padding, 0);
        lv_obj_set_style_pad_bottom(motion_content, vertical_padding, 0);
        lv_obj_set_style_pad_left(motion_content, UI_PADDING_MEDIUM, 0);
        lv_obj_set_style_pad_right(motion_content, UI_PADDING_MEDIUM, 0);
    }
}

void ui_panel_motion_setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    motion_panel = panel;
    parent_obj = parent_screen;

    printf("[Motion] Setting up event handlers...\n");

    // Setup header for responsive height
    lv_obj_t* motion_header = lv_obj_find_by_name(panel, "motion_header");
    if (motion_header) {
        ui_component_header_bar_setup(motion_header, parent_screen);
    }

    // Set responsive padding for content area
    lv_obj_t* motion_content = lv_obj_find_by_name(panel, "motion_content");
    if (motion_content) {
        lv_coord_t vertical_padding = ui_get_header_content_padding(lv_obj_get_height(parent_screen));
        // Set vertical padding (top/bottom) responsively, keep horizontal at medium (12px)
        lv_obj_set_style_pad_top(motion_content, vertical_padding, 0);
        lv_obj_set_style_pad_bottom(motion_content, vertical_padding, 0);
        lv_obj_set_style_pad_left(motion_content, UI_PADDING_MEDIUM, 0);
        lv_obj_set_style_pad_right(motion_content, UI_PADDING_MEDIUM, 0);
        printf("[Motion]   ✓ Content padding: top/bottom=%dpx, left/right=%dpx (responsive)\n",
               vertical_padding, UI_PADDING_MEDIUM);
    }

    // Register resize callback
    ui_resize_handler_register(on_resize);

    // Back button
    lv_obj_t* back_btn = lv_obj_find_by_name(panel, "back_button");
    if (back_btn) {
        lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, nullptr);
    }

    // Distance selector buttons
    const char* dist_names[] = {"dist_0_1", "dist_1", "dist_10", "dist_100"};
    for (int i = 0; i < 4; i++) {
        dist_buttons[i] = lv_obj_find_by_name(panel, dist_names[i]);
        if (dist_buttons[i]) {
            lv_obj_add_event_cb(dist_buttons[i], distance_button_cb, LV_EVENT_CLICKED, nullptr);
        }
    }
    update_distance_buttons();
    printf("[Motion]   ✓ Distance selector (4 buttons)\n");

    // Jog pad buttons (8 directions + center home)
    const char* jog_names[] = {"jog_n", "jog_s", "jog_e", "jog_w",
                               "jog_ne", "jog_nw", "jog_se", "jog_sw", "jog_home_xy"};
    for (const char* name : jog_names) {
        lv_obj_t* btn = lv_obj_find_by_name(panel, name);
        if (btn) {
            lv_obj_add_event_cb(btn, jog_button_cb, LV_EVENT_CLICKED, nullptr);
        }
    }
    printf("[Motion]   ✓ Jog pad (9 buttons)\n");

    // Z-axis buttons
    const char* z_names[] = {"z_up_10", "z_up_1", "z_down_1", "z_down_10"};
    int z_found = 0;
    for (const char* name : z_names) {
        lv_obj_t* btn = lv_obj_find_by_name(panel, name);
        if (btn) {
            printf("[Motion]     Found '%s' at %p\n", name, (void*)btn);
            lv_obj_add_event_cb(btn, z_button_cb, LV_EVENT_CLICKED, nullptr);
            printf("[Motion]     Event handler attached successfully\n");
            z_found++;
        } else {
            printf("[Motion]   ✗ Z button '%s' NOT FOUND!\n", name);
        }
    }
    printf("[Motion]   ✓ Z-axis controls (%d/4 buttons found)\n", z_found);

    // Home buttons
    const char* home_names[] = {"home_all", "home_x", "home_y", "home_z"};
    for (const char* name : home_names) {
        lv_obj_t* btn = lv_obj_find_by_name(panel, name);
        if (btn) {
            lv_obj_add_event_cb(btn, home_button_cb, LV_EVENT_CLICKED, nullptr);
        }
    }
    printf("[Motion]   ✓ Home buttons (4 buttons)\n");

    printf("[Motion] Setup complete!\n");
}

void ui_panel_motion_set_position(float x, float y, float z) {
    current_x = x;
    current_y = y;
    current_z = z;

    // Update subjects (will automatically update bound UI elements)
    snprintf(pos_x_buf, sizeof(pos_x_buf), "X: %6.1f mm", x);
    snprintf(pos_y_buf, sizeof(pos_y_buf), "Y: %6.1f mm", y);
    snprintf(pos_z_buf, sizeof(pos_z_buf), "Z: %6.1f mm", z);

    lv_subject_copy_string(&pos_x_subject, pos_x_buf);
    lv_subject_copy_string(&pos_y_subject, pos_y_buf);
    lv_subject_copy_string(&pos_z_subject, pos_z_buf);
}

jog_distance_t ui_panel_motion_get_distance() {
    return current_distance;
}

void ui_panel_motion_set_distance(jog_distance_t dist) {
    if (dist >= 0 && dist <= 3) {
        current_distance = dist;
        update_distance_buttons();
    }
}

void ui_panel_motion_jog(jog_direction_t direction, float distance_mm) {
    const char* dir_names[] = {"N(+Y)", "S(-Y)", "E(+X)", "W(-X)",
                               "NE(+X+Y)", "NW(-X+Y)", "SE(+X-Y)", "SW(-X-Y)"};

    printf("[Motion] Jog command: %s %.1fmm\n", dir_names[direction], distance_mm);

    // Mock position update (simulate jog movement)
    float dx = 0.0f, dy = 0.0f;

    switch (direction) {
        case JOG_DIR_N:  dy = distance_mm; break;
        case JOG_DIR_S:  dy = -distance_mm; break;
        case JOG_DIR_E:  dx = distance_mm; break;
        case JOG_DIR_W:  dx = -distance_mm; break;
        case JOG_DIR_NE: dx = distance_mm; dy = distance_mm; break;
        case JOG_DIR_NW: dx = -distance_mm; dy = distance_mm; break;
        case JOG_DIR_SE: dx = distance_mm; dy = -distance_mm; break;
        case JOG_DIR_SW: dx = -distance_mm; dy = -distance_mm; break;
    }

    ui_panel_motion_set_position(current_x + dx, current_y + dy, current_z);

    // TODO: Send actual G-code command via Moonraker API
    // Example: G0 X{new_x} Y{new_y} F{feedrate}
}

void ui_panel_motion_home(char axis) {
    printf("[Motion] Home command: %c axis\n", axis);

    // Mock position update (simulate homing)
    switch (axis) {
        case 'X': ui_panel_motion_set_position(0.0f, current_y, current_z); break;
        case 'Y': ui_panel_motion_set_position(current_x, 0.0f, current_z); break;
        case 'Z': ui_panel_motion_set_position(current_x, current_y, 0.0f); break;
        case 'A': ui_panel_motion_set_position(0.0f, 0.0f, 0.0f); break;  // All axes
    }

    // TODO: Send actual G-code command via Moonraker API
    // Example: G28 X (home X), G28 (home all)
}
