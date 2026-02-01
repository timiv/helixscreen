// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

/**
 * @file theme_compat.h
 * @brief Compatibility shims for legacy theme_core API
 *
 * These functions wrap the new ThemeManager API to provide backward
 * compatibility. They will be removed once all callers are migrated.
 */

#include <lvgl.h>

/**
 * @brief 16-color semantic palette for theme initialization
 */
typedef struct {
    lv_color_t screen_bg;   // 0: Main app background
    lv_color_t overlay_bg;  // 1: Sidebar/panel background
    lv_color_t card_bg;     // 2: Card surfaces
    lv_color_t elevated_bg; // 3: Elevated/control surfaces (buttons, inputs)
    lv_color_t border;      // 4: Borders and dividers
    lv_color_t text;        // 5: Primary text
    lv_color_t text_muted;  // 6: Secondary text
    lv_color_t text_subtle; // 7: Hint/tertiary text
    lv_color_t primary;     // 8: Primary accent
    lv_color_t secondary;   // 9: Secondary accent
    lv_color_t tertiary;    // 10: Tertiary accent
    lv_color_t info;        // 11: Info states
    lv_color_t success;     // 12: Success states
    lv_color_t warning;     // 13: Warning states
    lv_color_t danger;      // 14: Error/danger states
    lv_color_t focus;       // 15: Focus ring color
} theme_palette_t;

#ifdef __cplusplus
extern "C" {
#endif

// Theme lifecycle functions
lv_theme_t* theme_core_init(lv_display_t* display, const theme_palette_t* palette, bool is_dark,
                            const lv_font_t* base_font, int32_t border_radius, int32_t border_width,
                            int32_t border_opacity);
void theme_core_update_colors(bool is_dark, const theme_palette_t* palette, int32_t border_opacity);
void theme_core_preview_colors(bool is_dark, const theme_palette_t* palette, int32_t border_radius,
                               int32_t border_opacity);

// Style getters - map to ThemeManager::get_style(StyleRole::X)
lv_style_t* theme_core_get_card_style(void);
lv_style_t* theme_core_get_dialog_style(void);
lv_style_t* theme_core_get_obj_base_style(void);
lv_style_t* theme_core_get_input_bg_style(void);
lv_style_t* theme_core_get_disabled_style(void);
lv_style_t* theme_core_get_pressed_style(void);
lv_style_t* theme_core_get_focus_ring_style(void);

// Text styles
lv_style_t* theme_core_get_text_style(void);
lv_style_t* theme_core_get_text_muted_style(void);
lv_style_t* theme_core_get_text_subtle_style(void);

// Icon styles
lv_style_t* theme_core_get_icon_text_style(void);
lv_style_t* theme_core_get_icon_muted_style(void);
lv_style_t* theme_core_get_icon_primary_style(void);
lv_style_t* theme_core_get_icon_secondary_style(void);
lv_style_t* theme_core_get_icon_tertiary_style(void);
lv_style_t* theme_core_get_icon_info_style(void);
lv_style_t* theme_core_get_icon_success_style(void);
lv_style_t* theme_core_get_icon_warning_style(void);
lv_style_t* theme_core_get_icon_danger_style(void);

// Button styles
lv_style_t* theme_core_get_button_style(void);
lv_style_t* theme_core_get_button_primary_style(void);
lv_style_t* theme_core_get_button_secondary_style(void);
lv_style_t* theme_core_get_button_tertiary_style(void);
lv_style_t* theme_core_get_button_danger_style(void);
lv_style_t* theme_core_get_button_ghost_style(void);
lv_style_t* theme_core_get_button_success_style(void);
lv_style_t* theme_core_get_button_warning_style(void);

// Severity styles
lv_style_t* theme_core_get_severity_info_style(void);
lv_style_t* theme_core_get_severity_success_style(void);
lv_style_t* theme_core_get_severity_warning_style(void);
lv_style_t* theme_core_get_severity_danger_style(void);

// Widget styles
lv_style_t* theme_core_get_dropdown_style(void);
lv_style_t* theme_core_get_checkbox_style(void);
lv_style_t* theme_core_get_switch_style(void);
lv_style_t* theme_core_get_slider_style(void);
lv_style_t* theme_core_get_spinner_style(void);
lv_style_t* theme_core_get_arc_style(void);

// Color helpers for contrast text
lv_color_t theme_core_get_text_for_dark_bg(void);
lv_color_t theme_core_get_text_for_light_bg(void);
lv_color_t theme_core_get_contrast_text_color(lv_color_t bg_color);

#ifdef __cplusplus
}
#endif
