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

#include "ui_component_header_bar.h"
#include "ui_utils.h"
#include "ui_theme.h"
#include <stdio.h>
#include <vector>

// ============================================================================
// LIFECYCLE & RESPONSIVE BEHAVIOR
// ============================================================================

// Track all header_bar instances for resize handling
static std::vector<lv_obj_t*> header_instances;

// Event handler for DELETE event (cleanup)
static void header_bar_delete_cb(lv_event_t* e) {
    lv_obj_t* header = (lv_obj_t*)lv_event_get_target(e);

    // Remove from tracking list
    auto it = std::find(header_instances.begin(), header_instances.end(), header);
    if (it != header_instances.end()) {
        header_instances.erase(it);
        printf("[HeaderBar] Removed from tracking (%zu remain)\n", header_instances.size());
    }
}

// Global resize callback (called by app resize handler system)
static void on_app_resize() {
    for (lv_obj_t* header : header_instances) {
        if (!header) continue;

        lv_obj_t* screen = lv_obj_get_screen(header);
        if (!screen) continue;

        lv_coord_t header_height = ui_get_responsive_header_height(lv_obj_get_height(screen));
        lv_obj_set_height(header, header_height);
    }

    if (!header_instances.empty()) {
        printf("[HeaderBar] Updated %zu header heights on resize\n", header_instances.size());
    }
}

// ============================================================================
// HEADER BAR API
// ============================================================================

bool ui_header_bar_show_right_button(lv_obj_t* header_bar_widget) {
    if (!header_bar_widget) {
        return false;
    }

    lv_obj_t* button = lv_obj_find_by_name(header_bar_widget, "right_button");
    if (button) {
        lv_obj_remove_flag(button, LV_OBJ_FLAG_HIDDEN);
        return true;
    }

    return false;
}

bool ui_header_bar_hide_right_button(lv_obj_t* header_bar_widget) {
    if (!header_bar_widget) {
        return false;
    }

    lv_obj_t* button = lv_obj_find_by_name(header_bar_widget, "right_button");
    if (button) {
        lv_obj_add_flag(button, LV_OBJ_FLAG_HIDDEN);
        return true;
    }

    return false;
}

bool ui_header_bar_set_right_button_text(lv_obj_t* header_bar_widget, const char* text) {
    if (!header_bar_widget || !text) {
        return false;
    }

    lv_obj_t* button = lv_obj_find_by_name(header_bar_widget, "right_button");
    if (!button) {
        return false;
    }

    // Find the label child of the button
    lv_obj_t* label = lv_obj_find_by_name(button, "right_button_label");
    if (label) {
        lv_label_set_text(label, text);
        return true;
    }

    return false;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ui_component_header_bar_init() {
    // Register global resize callback for all header_bar instances
    ui_resize_handler_register(on_app_resize);

    printf("[HeaderBar] Component system initialized\n");
}

void ui_component_header_bar_setup(lv_obj_t* header, lv_obj_t* screen) {
    if (!header || !screen) {
        return;
    }

    // Attach DELETE event for cleanup
    lv_obj_add_event_cb(header, header_bar_delete_cb, LV_EVENT_DELETE, nullptr);

    // Track this instance for global resize handling
    header_instances.push_back(header);

    // Apply responsive height immediately
    lv_coord_t header_height = ui_get_responsive_header_height(lv_obj_get_height(screen));
    lv_obj_set_height(header, header_height);

    printf("[HeaderBar] Setup complete: height=%dpx\n", header_height);
}
