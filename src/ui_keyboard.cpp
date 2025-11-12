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

#include "ui_keyboard.h"
#include "config.h"
#include <spdlog/spdlog.h>

// Global keyboard instance
static lv_obj_t* g_keyboard = NULL;
static bool g_number_row_enabled = false;
static bool g_number_row_user_preference = true;  // User's saved preference
static lv_obj_t* g_context_textarea = NULL;       // Currently focused textarea

//=============================================================================
// IMPROVED KEYBOARD LAYOUTS
//=============================================================================

// Macro for keyboard buttons with popover support (C++ requires explicit cast)
#define LV_KEYBOARD_CTRL_BUTTON_FLAGS (LV_BUTTONMATRIX_CTRL_NO_REPEAT | LV_BUTTONMATRIX_CTRL_CLICK_TRIG | LV_BUTTONMATRIX_CTRL_CHECKED)
#define LV_KB_BTN(width) static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_POPOVER | (width))

// Lowercase with number row (Android-style)
static const char * const kb_map_lc_numrow[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
    "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_NEW_LINE, "\n",
    "ABC", "z", "x", "c", "v", "b", "n", "m", ".", ",", "1#", "\n",
    LV_SYMBOL_KEYBOARD, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t kb_ctrl_lc_numrow[] = {
    // Row 1: Numbers 1-0 + backspace
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 7),
    // Row 2: q-p
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    // Row 3: a-l + enter
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 8),
    // Row 4: shift + z-m + period/comma + special
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5),
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5),
    // Row 5: keyboard/left/space/right/ok
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 2),
    static_cast<lv_buttonmatrix_ctrl_t>(6),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 2),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2)
};

// Uppercase with number row
static const char * const kb_map_uc_numrow[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
    "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
    "A", "S", "D", "F", "G", "H", "J", "K", "L", LV_SYMBOL_NEW_LINE, "\n",
    "abc", "Z", "X", "C", "V", "B", "N", "M", ".", ",", "1#", "\n",
    LV_SYMBOL_KEYBOARD, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
};

static const lv_buttonmatrix_ctrl_t kb_ctrl_uc_numrow[] = {
    // Row 1: Numbers 1-0 + backspace
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 7),
    // Row 2: Q-P
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    // Row 3: A-L + enter
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 8),
    // Row 4: shift + Z-M + period/comma + special
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5),
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 5),
    // Row 5: keyboard/left/space/right/ok
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 2),
    static_cast<lv_buttonmatrix_ctrl_t>(6),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 2),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2)
};

// Improved numeric keyboard with PERIOD (critical for IPs and decimals)
static const char * const kb_map_num_improved[] = {
    "1", "2", "3", LV_SYMBOL_KEYBOARD, "\n",
    "4", "5", "6", LV_SYMBOL_OK, "\n",
    "7", "8", "9", LV_SYMBOL_BACKSPACE, "\n",
    "+/-", "0", ".", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, ""
};

static const lv_buttonmatrix_ctrl_t kb_ctrl_num_improved[] = {
    LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_KEYBOARD_CTRL_BUTTON_FLAGS | 2),
    LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    static_cast<lv_buttonmatrix_ctrl_t>(2),
    LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 1),
    static_cast<lv_buttonmatrix_ctrl_t>(LV_BUTTONMATRIX_CTRL_CHECKED | 1)
};

/**
 * @brief Textarea focus event callback - handles auto show/hide with context-aware behavior
 */
static void textarea_focus_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* textarea = lv_event_get_target_obj(e);

    if (code == LV_EVENT_FOCUSED) {
        spdlog::debug("[Keyboard] Textarea focused: {}", (void*)textarea);

        // Check if this is a password field (stored in user_data by register_textarea_ex)
        bool is_password = (lv_obj_get_user_data(textarea) == (void*)1);

        if (is_password && !g_number_row_user_preference) {
            // Temporarily enable number row for password fields (even if user disabled it)
            spdlog::debug("[Keyboard] Auto-enabling number row for password field");
            g_number_row_enabled = true;
            lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER,
                               kb_map_lc_numrow, kb_ctrl_lc_numrow);
            lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_UPPER,
                               kb_map_uc_numrow, kb_ctrl_uc_numrow);
        }

        g_context_textarea = textarea;
        ui_keyboard_show(textarea);
    }
    else if (code == LV_EVENT_DEFOCUSED) {
        spdlog::debug("[Keyboard] Textarea defocused: {}", (void*)textarea);

        // Restore user preference when leaving password field
        bool was_password = (lv_obj_get_user_data(textarea) == (void*)1);
        if (was_password && !g_number_row_user_preference && g_number_row_enabled) {
            spdlog::debug("[Keyboard] Restoring number row to user preference");
            g_number_row_enabled = g_number_row_user_preference;
            lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER, NULL, NULL);
            lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_UPPER, NULL, NULL);
        }

        g_context_textarea = NULL;
        ui_keyboard_hide();
    }
}

/**
 * @brief Keyboard event callback - handles ready/cancel events
 */
static void keyboard_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        spdlog::info("[Keyboard] OK pressed - input confirmed");
        ui_keyboard_hide();
    }
    else if (code == LV_EVENT_CANCEL) {
        spdlog::info("[Keyboard] Cancel pressed");
        ui_keyboard_hide();
    }
}

void ui_keyboard_init(lv_obj_t* parent)
{
    if (g_keyboard != NULL) {
        spdlog::warn("[Keyboard] Already initialized, skipping");
        return;
    }

    spdlog::info("[Keyboard] Initializing global keyboard");

    // Create keyboard at bottom of screen
    g_keyboard = lv_keyboard_create(parent);

    // Set initial mode (lowercase text)
    lv_keyboard_set_mode(g_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);

    // Enable pop-overs (iOS/Android-style key feedback)
    lv_keyboard_set_popovers(g_keyboard, true);

    // Apply improved numeric keyboard layout (adds period key for IPs/decimals)
    lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_NUMBER,
                       kb_map_num_improved, kb_ctrl_num_improved);

    // Load number row preference from config (defaults to true)
    Config* config = Config::get_instance();
    g_number_row_user_preference = config->get("/keyboard_number_row", true);
    g_number_row_enabled = g_number_row_user_preference;

    if (g_number_row_enabled) {
        lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER,
                           kb_map_lc_numrow, kb_ctrl_lc_numrow);
        lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_UPPER,
                           kb_map_uc_numrow, kb_ctrl_uc_numrow);
        spdlog::info("[Keyboard] Number row enabled (from config)");
    } else {
        spdlog::info("[Keyboard] Number row disabled (from config)");
    }

    // Apply styling - theme handles colors, set opacity for transparency
    lv_obj_set_style_bg_opa(g_keyboard, LV_OPA_80, LV_PART_MAIN);  // 80% opacity = 20% transparent
    lv_obj_set_style_bg_opa(g_keyboard, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_radius(g_keyboard, 8, LV_PART_ITEMS);  // Rounded key corners

    // Position at bottom-middle (default)
    lv_obj_align(g_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Start hidden
    lv_obj_add_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);

    // Add event handlers for ready and cancel events
    lv_obj_add_event_cb(g_keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(g_keyboard, keyboard_event_cb, LV_EVENT_CANCEL, NULL);

    spdlog::info("[Keyboard] Initialization complete");
}

void ui_keyboard_register_textarea(lv_obj_t* textarea)
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    if (textarea == NULL) {
        spdlog::error("[Keyboard] Cannot register NULL textarea");
        return;
    }

    spdlog::debug("[Keyboard] Registering textarea: {}", (void*)textarea);

    // Add event handler to catch focus/defocus events (not ALL events to avoid cleanup issues)
    lv_obj_add_event_cb(textarea, textarea_focus_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(textarea, textarea_focus_event_cb, LV_EVENT_DEFOCUSED, NULL);

    // Add textarea to default input group for physical keyboard input
    lv_group_t* default_group = lv_group_get_default();
    if (default_group) {
        lv_group_add_obj(default_group, textarea);
        spdlog::debug("[Keyboard] Added textarea to input group for physical keyboard");
    }
}

void ui_keyboard_show(lv_obj_t* textarea)
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    // Safety: if keyboard's parent is NULL, we're in cleanup - bail out
    if (lv_obj_get_parent(g_keyboard) == NULL) {
        spdlog::debug("[Keyboard] Skipping show - keyboard is being cleaned up");
        return;
    }

    // Safety: check if screen is valid before layout operations
    lv_obj_t* screen = lv_screen_active();
    if (screen == NULL || lv_obj_get_parent(screen) == NULL) {
        spdlog::debug("[Keyboard] Skipping show - screen is being cleaned up");
        return;
    }

    spdlog::debug("[Keyboard] Showing keyboard for textarea: {}", (void*)textarea);

    // Assign textarea to keyboard (standard LVGL API)
    lv_keyboard_set_textarea(g_keyboard, textarea);

    // Show keyboard
    lv_obj_remove_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);

    // Force layout update to get accurate positions
    lv_obj_update_layout(screen);

    if (!textarea) {
        return;
    }

    // Get absolute coordinates
    lv_area_t kb_coords, ta_coords;
    lv_obj_get_coords(g_keyboard, &kb_coords);
    lv_obj_get_coords(textarea, &ta_coords);

    int32_t kb_top = kb_coords.y1;
    int32_t ta_bottom = ta_coords.y2;

    // Add padding above textarea (20px breathing room)
    const int32_t padding = 20;
    int32_t desired_bottom = kb_top - padding;

    // Calculate if we need to shift the screen up
    if (ta_bottom > desired_bottom) {
        int32_t shift_up = ta_bottom - desired_bottom;

        spdlog::debug("[Keyboard] Shifting screen UP by {} px (ta_bottom={}, kb_top={})",
                     shift_up, ta_bottom, kb_top);

        // Move all screen children (except keyboard) up with animation
        lv_obj_t* screen = lv_screen_active();
        uint32_t child_count = lv_obj_get_child_count(screen);

        for (uint32_t i = 0; i < child_count; i++) {
            lv_obj_t* child = lv_obj_get_child(screen, i);
            if (child == g_keyboard) continue;

            int32_t current_y = lv_obj_get_y(child);
            int32_t target_y = current_y - shift_up;

            // Animate the Y position change (200ms, fast and smooth)
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, child);
            lv_anim_set_values(&a, current_y, target_y);
            lv_anim_set_time(&a, 200);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            lv_anim_start(&a);
        }
    } else {
        spdlog::debug("[Keyboard] Textarea already visible (ta_bottom={}, kb_top={})",
                     ta_bottom, kb_top);
    }
}

void ui_keyboard_hide()
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    // Safety: if keyboard's parent is NULL, we're in cleanup - bail out
    if (lv_obj_get_parent(g_keyboard) == NULL) {
        spdlog::debug("[Keyboard] Skipping hide - keyboard is being cleaned up");
        return;
    }

    // Safety: check if screen is valid before layout operations
    lv_obj_t* screen = lv_screen_active();
    if (screen == NULL || lv_obj_get_parent(screen) == NULL) {
        spdlog::debug("[Keyboard] Skipping hide - screen is being cleaned up");
        return;
    }

    spdlog::debug("[Keyboard] Hiding keyboard");

    // Clear keyboard assignment
    lv_keyboard_set_textarea(g_keyboard, NULL);

    // Hide keyboard
    lv_obj_add_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);

    // Move all screen children (except keyboard) back to y=0 with animation
    uint32_t child_count = lv_obj_get_child_count(screen);

    spdlog::debug("[Keyboard] Restoring screen children to y=0");

    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t* child = lv_obj_get_child(screen, i);
        if (child == g_keyboard) continue;

        int32_t current_y = lv_obj_get_y(child);
        if (current_y != 0) {
            // Animate back to y=0 (200ms, fast and smooth)
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, child);
            lv_anim_set_values(&a, current_y, 0);
            lv_anim_set_time(&a, 200);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
            lv_anim_start(&a);
        }
    }
}

bool ui_keyboard_is_visible()
{
    if (g_keyboard == NULL) {
        return false;
    }

    return !lv_obj_has_flag(g_keyboard, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t* ui_keyboard_get_instance()
{
    return g_keyboard;
}

void ui_keyboard_set_mode(lv_keyboard_mode_t mode)
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    spdlog::debug("[Keyboard] Setting mode: {}", (int)mode);
    lv_keyboard_set_mode(g_keyboard, mode);
}

void ui_keyboard_set_position(lv_align_t align, int32_t x_ofs, int32_t y_ofs)
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    spdlog::debug("[Keyboard] Setting position: align={}, x={}, y={}",
                  (int)align, x_ofs, y_ofs);
    lv_obj_align(g_keyboard, align, x_ofs, y_ofs);
}

void ui_keyboard_set_number_row(bool enable)
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    if (g_number_row_enabled == enable) {
        return;  // No change needed
    }

    g_number_row_enabled = enable;
    g_number_row_user_preference = enable;  // Update user preference
    spdlog::info("[Keyboard] Number row {} (user preference)", enable ? "enabled" : "disabled");

    // Apply custom layouts for text modes with number row
    if (enable) {
        // Set custom maps with number row for text modes
        lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER,
                           kb_map_lc_numrow, kb_ctrl_lc_numrow);
        lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_UPPER,
                           kb_map_uc_numrow, kb_ctrl_uc_numrow);
    } else {
        // Restore default LVGL layouts (pass NULL to reset)
        lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER, NULL, NULL);
        lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_TEXT_UPPER, NULL, NULL);
    }

    // Always apply improved numeric keyboard (with period key)
    lv_keyboard_set_map(g_keyboard, LV_KEYBOARD_MODE_NUMBER,
                       kb_map_num_improved, kb_ctrl_num_improved);

    // Save preference to config
    Config* config = Config::get_instance();
    config->set("/keyboard_number_row", enable);
    config->save();

    // Refresh display if currently visible
    lv_keyboard_mode_t current_mode = lv_keyboard_get_mode(g_keyboard);
    lv_keyboard_set_mode(g_keyboard, current_mode);
}

bool ui_keyboard_get_number_row()
{
    return g_number_row_enabled;
}

void ui_keyboard_register_textarea_ex(lv_obj_t* textarea, bool is_password)
{
    if (g_keyboard == NULL) {
        spdlog::error("[Keyboard] Not initialized - call ui_keyboard_init() first");
        return;
    }

    if (textarea == NULL) {
        spdlog::error("[Keyboard] Cannot register NULL textarea");
        return;
    }

    spdlog::debug("[Keyboard] Registering textarea: {} (password: {})",
                  (void*)textarea, is_password);

    // Store password flag in user data (lv_obj doesn't have built-in password type tracking)
    lv_obj_set_user_data(textarea, is_password ? (void*)1 : (void*)0);

    // Use standard registration which adds focus/defocus handlers
    ui_keyboard_register_textarea(textarea);
}
