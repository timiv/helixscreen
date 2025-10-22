/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of GuppyScreen.
 *
 * GuppyScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GuppyScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GuppyScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui_utils.h"
#include <cstdio>
#include <ctime>
#include <vector>

std::string format_print_time(int minutes) {
    char buf[32];
    if (minutes < 60) {
        snprintf(buf, sizeof(buf), "%dm", minutes);
    } else {
        int hours = minutes / 60;
        int mins = minutes % 60;
        if (mins == 0) {
            snprintf(buf, sizeof(buf), "%dh", hours);
        } else {
            snprintf(buf, sizeof(buf), "%dh%dm", hours, mins);
        }
    }
    return std::string(buf);
}

std::string format_filament_weight(float grams) {
    char buf[32];
    if (grams < 1.0f) {
        snprintf(buf, sizeof(buf), "%.1fg", grams);
    } else if (grams < 10.0f) {
        snprintf(buf, sizeof(buf), "%.1fg", grams);
    } else {
        snprintf(buf, sizeof(buf), "%.0fg", grams);
    }
    return std::string(buf);
}

std::string format_file_size(size_t bytes) {
    char buf[32];
    if (bytes < 1024) {
        snprintf(buf, sizeof(buf), "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        double kb = bytes / 1024.0;
        snprintf(buf, sizeof(buf), "%.1f KB", kb);
    } else if (bytes < 1024 * 1024 * 1024) {
        double mb = bytes / (1024.0 * 1024.0);
        snprintf(buf, sizeof(buf), "%.1f MB", mb);
    } else {
        double gb = bytes / (1024.0 * 1024.0 * 1024.0);
        snprintf(buf, sizeof(buf), "%.2f GB", gb);
    }
    return std::string(buf);
}

std::string format_modified_date(time_t timestamp) {
    char buf[64];
    struct tm* timeinfo = localtime(&timestamp);
    if (timeinfo) {
        // Format: "Jan 15 14:30"
        strftime(buf, sizeof(buf), "%b %d %H:%M", timeinfo);
    } else {
        snprintf(buf, sizeof(buf), "Unknown");
    }
    return std::string(buf);
}

// ============================================================================
// Header Bar Component Helpers
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
// App-level Resize Handling
// ============================================================================

// Storage for registered resize callbacks
static std::vector<ui_resize_callback_t> resize_callbacks;

// Debounce timer and period
static lv_timer_t* resize_debounce_timer = nullptr;
static constexpr uint32_t RESIZE_DEBOUNCE_MS = 250;

// Debounce timer callback - invokes all registered callbacks
static void resize_timer_cb(lv_timer_t* timer) {
    (void)timer;

    LV_LOG_USER("Resize debounce complete, calling %zu registered callbacks", resize_callbacks.size());

    // Call all registered callbacks
    for (auto callback : resize_callbacks) {
        if (callback) {
            callback();
        }
    }

    // Delete one-shot timer
    if (resize_debounce_timer) {
        lv_timer_delete(resize_debounce_timer);
        resize_debounce_timer = nullptr;
    }
}

// Screen resize event handler - starts/resets debounce timer
static void resize_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SIZE_CHANGED) {
        lv_obj_t* screen = (lv_obj_t*)lv_event_get_target(e);
        lv_coord_t width = lv_obj_get_width(screen);
        lv_coord_t height = lv_obj_get_height(screen);

        LV_LOG_USER("Screen size changed to %dx%d, resetting debounce timer", width, height);

        // Reset or create debounce timer
        if (resize_debounce_timer) {
            lv_timer_reset(resize_debounce_timer);
        } else {
            resize_debounce_timer = lv_timer_create(resize_timer_cb, RESIZE_DEBOUNCE_MS, nullptr);
            lv_timer_set_repeat_count(resize_debounce_timer, 1);  // One-shot
        }
    }
}

void ui_resize_handler_init(lv_obj_t* screen) {
    if (!screen) {
        LV_LOG_ERROR("Cannot init resize handler: screen is null");
        return;
    }

    // Add SIZE_CHANGED event listener to screen
    lv_obj_add_event_cb(screen, resize_event_cb, LV_EVENT_SIZE_CHANGED, nullptr);

    LV_LOG_USER("Resize handler initialized on screen");
}

void ui_resize_handler_register(ui_resize_callback_t callback) {
    if (!callback) {
        LV_LOG_WARN("Attempted to register null resize callback");
        return;
    }

    resize_callbacks.push_back(callback);
    LV_LOG_USER("Registered resize callback (%zu total)", resize_callbacks.size());
}
