// SPDX-License-Identifier: GPL-3.0-or-later
#include "theme_compat.h"

#include "ui_fonts.h"

#include "lvgl/lvgl.h"
#include "lvgl/src/themes/lv_theme_private.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

// Static theme instance - persists for lifetime of app
static lv_theme_t helix_theme;
static lv_theme_t* default_theme_backup = nullptr;

// Additional styles not in StyleRole enum (widget-specific parts)
static lv_style_t dropdown_indicator_style;
static lv_style_t checkbox_text_style;
static lv_style_t checkbox_box_style;
static lv_style_t checkbox_indicator_style;
static lv_style_t switch_track_style;
static lv_style_t switch_indicator_style;
static lv_style_t switch_knob_style;
static lv_style_t slider_track_style;
static lv_style_t slider_indicator_style;
static lv_style_t slider_knob_style;
static lv_style_t slider_disabled_style;
static lv_color_t dropdown_accent_color;
static bool extra_styles_initialized = false;

// Convert theme_palette_t to ThemePalette
static ThemePalette convert_palette(const theme_palette_t* p, int border_radius, int border_width,
                                    int border_opacity) {
    ThemePalette palette;
    palette.screen_bg = p->screen_bg;
    palette.overlay_bg = p->overlay_bg;
    palette.card_bg = p->card_bg;
    palette.elevated_bg = p->elevated_bg;
    palette.border = p->border;
    palette.text = p->text;
    palette.text_muted = p->text_muted;
    palette.text_subtle = p->text_subtle;
    palette.primary = p->primary;
    palette.secondary = p->secondary;
    palette.tertiary = p->tertiary;
    palette.info = p->info;
    palette.success = p->success;
    palette.warning = p->warning;
    palette.danger = p->danger;
    palette.focus = p->focus;
    palette.border_radius = border_radius;
    palette.border_width = border_width;
    palette.border_opacity = border_opacity;
    return palette;
}

// Initialize the extra widget-specific styles
static void init_extra_styles(const theme_palette_t* palette, int border_radius) {
    if (extra_styles_initialized)
        return;

    dropdown_accent_color = palette->secondary;

    // Dropdown indicator - MDI font for chevron
    lv_style_init(&dropdown_indicator_style);
    lv_style_set_text_font(&dropdown_indicator_style, &mdi_icons_24);

    // Checkbox styles
    lv_style_init(&checkbox_text_style);
    lv_style_set_text_color(&checkbox_text_style, palette->text);

    lv_style_init(&checkbox_box_style);
    lv_style_set_bg_color(&checkbox_box_style, palette->elevated_bg);
    lv_style_set_bg_opa(&checkbox_box_style, LV_OPA_COVER);
    lv_style_set_border_color(&checkbox_box_style, palette->border);
    lv_style_set_border_width(&checkbox_box_style, 2);
    lv_style_set_radius(&checkbox_box_style, 4);

    lv_style_init(&checkbox_indicator_style);
    lv_style_set_text_font(&checkbox_indicator_style, &mdi_icons_16);
    lv_style_set_text_color(&checkbox_indicator_style, palette->primary);

    // Switch styles
    lv_style_init(&switch_track_style);
    lv_style_set_bg_color(&switch_track_style, palette->border);
    lv_style_set_bg_opa(&switch_track_style, LV_OPA_COVER);

    lv_style_init(&switch_indicator_style);
    lv_style_set_bg_color(&switch_indicator_style, palette->secondary);
    lv_style_set_bg_opa(&switch_indicator_style, LV_OPA_COVER);

    lv_style_init(&switch_knob_style);
    lv_style_set_bg_color(&switch_knob_style, palette->primary);
    lv_style_set_bg_opa(&switch_knob_style, LV_OPA_COVER);

    // Slider styles
    lv_style_init(&slider_track_style);
    lv_style_set_bg_color(&slider_track_style, palette->border);
    lv_style_set_bg_opa(&slider_track_style, LV_OPA_COVER);
    lv_style_set_radius(&slider_track_style, border_radius);

    lv_style_init(&slider_indicator_style);
    lv_style_set_bg_color(&slider_indicator_style, palette->primary);
    lv_style_set_bg_opa(&slider_indicator_style, LV_OPA_COVER);

    lv_style_init(&slider_knob_style);
    lv_style_set_bg_color(&slider_knob_style, palette->primary);
    lv_style_set_bg_opa(&slider_knob_style, LV_OPA_COVER);
    lv_style_set_border_color(&slider_knob_style, palette->border);
    lv_style_set_border_width(&slider_knob_style, 1);
    lv_style_set_shadow_width(&slider_knob_style, 4);
    lv_style_set_shadow_color(&slider_knob_style, lv_color_black());
    lv_style_set_shadow_opa(&slider_knob_style, LV_OPA_30);

    lv_style_init(&slider_disabled_style);
    lv_style_set_opa(&slider_disabled_style, LV_OPA_50);

    extra_styles_initialized = true;
}

// HelixScreen theme apply callback - applies styles based on widget type
static void helix_theme_apply(lv_theme_t* theme, lv_obj_t* obj) {
    // First apply LVGL default theme
    if (default_theme_backup && default_theme_backup->apply_cb) {
        default_theme_backup->apply_cb(default_theme_backup, obj);
    }

    auto& tm = ThemeManager::instance();

    // Global disabled state
    lv_obj_add_style(obj, tm.get_style(StyleRole::Disabled), LV_PART_MAIN | LV_STATE_DISABLED);

    // Plain lv_obj containers get transparent background (layout containers)
    if (lv_obj_check_type(obj, &lv_obj_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::ObjBase), LV_PART_MAIN);
    }

#if LV_USE_BUTTON
    if (lv_obj_check_type(obj, &lv_button_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::Button), LV_PART_MAIN);
        lv_obj_add_style(obj, tm.get_style(StyleRole::Pressed), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, tm.get_style(StyleRole::Focused), LV_STATE_FOCUSED);
    }
#endif

#if LV_USE_TEXTAREA
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::InputBg), LV_PART_MAIN);
        lv_obj_add_style(obj, tm.get_style(StyleRole::Focused), LV_STATE_FOCUSED);
    }
#endif

#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::InputBg), LV_PART_MAIN);
        lv_obj_add_style(obj, &dropdown_indicator_style, LV_PART_INDICATOR);
        lv_obj_add_style(obj, tm.get_style(StyleRole::Focused), LV_STATE_FOCUSED);
    }
    if (lv_obj_check_type(obj, &lv_dropdownlist_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::InputBg), LV_PART_MAIN);

        // Compute contrast text for dropdown accent
        uint8_t lum = lv_color_luminance(dropdown_accent_color);
        lv_color_t selected_text = (lum > 140) ? lv_color_black() : lv_color_white();

        lv_obj_set_style_bg_color(obj, dropdown_accent_color, LV_PART_SELECTED);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_SELECTED);
        lv_obj_set_style_text_color(obj, selected_text, LV_PART_SELECTED);
        lv_obj_set_style_bg_color(obj, dropdown_accent_color, LV_PART_SELECTED | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(obj, selected_text, LV_PART_SELECTED | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(obj, dropdown_accent_color, LV_PART_SELECTED | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(obj, selected_text, LV_PART_SELECTED | LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(obj, dropdown_accent_color,
                                  LV_PART_SELECTED | LV_STATE_CHECKED | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER,
                                LV_PART_SELECTED | LV_STATE_CHECKED | LV_STATE_PRESSED);
        lv_obj_set_style_text_color(obj, selected_text,
                                    LV_PART_SELECTED | LV_STATE_CHECKED | LV_STATE_PRESSED);
    }
#endif

#if LV_USE_ROLLER
    if (lv_obj_check_type(obj, &lv_roller_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::InputBg), LV_PART_MAIN);
    }
#endif

#if LV_USE_SPINBOX
    if (lv_obj_check_type(obj, &lv_spinbox_class)) {
        lv_obj_add_style(obj, tm.get_style(StyleRole::InputBg), LV_PART_MAIN);
    }
#endif

#if LV_USE_CHECKBOX
    if (lv_obj_check_type(obj, &lv_checkbox_class)) {
        lv_obj_add_style(obj, &checkbox_text_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &checkbox_box_style, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &checkbox_indicator_style, LV_PART_INDICATOR | LV_STATE_CHECKED);
    }
#endif

#if LV_USE_SWITCH
    if (lv_obj_check_type(obj, &lv_switch_class)) {
        lv_obj_add_style(obj, &switch_track_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &switch_indicator_style, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_style(obj, &switch_knob_style, LV_PART_KNOB);
        lv_obj_add_style(obj, tm.get_style(StyleRole::Focused), LV_STATE_FOCUSED);
    }
#endif

#if LV_USE_SLIDER
    if (lv_obj_check_type(obj, &lv_slider_class)) {
        lv_obj_add_style(obj, &slider_track_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &slider_indicator_style, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &slider_knob_style, LV_PART_KNOB);
        lv_obj_add_style(obj, &slider_disabled_style, LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_add_style(obj, &slider_disabled_style, LV_PART_INDICATOR | LV_STATE_DISABLED);
        lv_obj_add_style(obj, &slider_disabled_style, LV_PART_KNOB | LV_STATE_DISABLED);
    }
#endif
}

// Theme lifecycle functions
lv_theme_t* theme_core_init(lv_display_t* display, const theme_palette_t* palette, bool is_dark,
                            const lv_font_t* base_font, int32_t border_radius, int32_t border_width,
                            int32_t border_opacity) {
    // Convert palette and initialize ThemeManager
    ThemePalette dark_pal = convert_palette(palette, border_radius, border_width, border_opacity);
    ThemePalette light_pal = dark_pal; // Same palette for both modes initially

    auto& tm = ThemeManager::instance();
    tm.set_palettes(light_pal, dark_pal);
    tm.init();
    tm.set_dark_mode(is_dark);

    // Initialize widget-specific styles not in StyleRole enum
    init_extra_styles(palette, border_radius);

    // Create LVGL default theme as base (we'll layer on top)
    default_theme_backup =
        lv_theme_default_init(display, palette->primary, palette->secondary, is_dark, base_font);

    // Initialize our custom theme
    lv_theme_set_apply_cb(&helix_theme, helix_theme_apply);
    helix_theme.font_small = base_font;
    helix_theme.font_normal = base_font;
    helix_theme.font_large = base_font;
    helix_theme.color_primary = palette->primary;
    helix_theme.color_secondary = palette->secondary;

    spdlog::debug("[ThemeCompat] Initialized HelixScreen theme via ThemeManager");
    return &helix_theme;
}

void theme_core_update_colors(bool is_dark, const theme_palette_t* palette,
                              int32_t border_opacity) {
    auto& tm = ThemeManager::instance();
    const auto& current = tm.current_palette();

    // Convert and update both palettes
    ThemePalette new_pal =
        convert_palette(palette, current.border_radius, current.border_width, border_opacity);

    if (is_dark) {
        // Update dark palette
        ThemePalette light_pal = new_pal; // Keep symmetric for now
        tm.set_palettes(light_pal, new_pal);
    } else {
        // Update light palette
        ThemePalette dark_pal = new_pal;
        tm.set_palettes(new_pal, dark_pal);
    }

    tm.set_dark_mode(is_dark);
    spdlog::debug("[ThemeCompat] Updated colors, dark_mode={}", is_dark);
}

void theme_core_preview_colors(bool is_dark, const theme_palette_t* palette, int32_t border_radius,
                               int32_t border_opacity) {
    auto& tm = ThemeManager::instance();
    const auto& current = tm.current_palette();

    ThemePalette preview_pal =
        convert_palette(palette, border_radius, current.border_width, border_opacity);
    (void)is_dark; // Preview uses the palette directly

    tm.preview_palette(preview_pal);
    spdlog::debug("[ThemeCompat] Previewing colors");
}

// Helper macro to reduce boilerplate
#define STYLE_GETTER(name, role)                                                                   \
    lv_style_t* theme_core_get_##name##_style(void) {                                              \
        return ThemeManager::instance().get_style(StyleRole::role);                                \
    }

// Base styles
STYLE_GETTER(card, Card)
STYLE_GETTER(dialog, Dialog)
STYLE_GETTER(obj_base, ObjBase)
STYLE_GETTER(input_bg, InputBg)
STYLE_GETTER(disabled, Disabled)
STYLE_GETTER(pressed, Pressed)
STYLE_GETTER(focus_ring, Focused)

// Text styles
STYLE_GETTER(text, TextPrimary)
STYLE_GETTER(text_muted, TextMuted)
STYLE_GETTER(text_subtle, TextSubtle)

// Icon styles
STYLE_GETTER(icon_text, IconText)
STYLE_GETTER(icon_muted, TextMuted) // Maps to TextMuted, same color
STYLE_GETTER(icon_primary, IconPrimary)
STYLE_GETTER(icon_secondary, IconSecondary)
STYLE_GETTER(icon_tertiary, IconTertiary)
STYLE_GETTER(icon_info, IconInfo)
STYLE_GETTER(icon_success, IconSuccess)
STYLE_GETTER(icon_warning, IconWarning)
STYLE_GETTER(icon_danger, IconDanger)

// Button styles
STYLE_GETTER(button, Button)
STYLE_GETTER(button_primary, ButtonPrimary)
STYLE_GETTER(button_secondary, ButtonSecondary)
STYLE_GETTER(button_tertiary, ButtonTertiary)
STYLE_GETTER(button_danger, ButtonDanger)
STYLE_GETTER(button_ghost, ButtonGhost)
STYLE_GETTER(button_success, ButtonSuccess)
STYLE_GETTER(button_warning, ButtonWarning)

// Severity styles
STYLE_GETTER(severity_info, SeverityInfo)
STYLE_GETTER(severity_success, SeveritySuccess)
STYLE_GETTER(severity_warning, SeverityWarning)
STYLE_GETTER(severity_danger, SeverityDanger)

// Widget styles
STYLE_GETTER(dropdown, Dropdown)
STYLE_GETTER(checkbox, Checkbox)
STYLE_GETTER(switch, Switch)
STYLE_GETTER(slider, Slider)
STYLE_GETTER(spinner, Spinner)
STYLE_GETTER(arc, Arc)

#undef STYLE_GETTER

// Color helpers
lv_color_t theme_core_get_text_for_dark_bg(void) {
    // Light text for dark backgrounds
    return lv_color_hex(0xECEFF4);
}

lv_color_t theme_core_get_text_for_light_bg(void) {
    // Dark text for light backgrounds
    return lv_color_hex(0x2E3440);
}

lv_color_t theme_core_get_contrast_text_color(lv_color_t bg_color) {
    // Compute luminance using standard formula: L = (299*R + 587*G + 114*B) / 1000
    int luminance = (299 * bg_color.red + 587 * bg_color.green + 114 * bg_color.blue) / 1000;

    // Dark bg (L < 128): use light text; Light bg (L >= 128): use dark text
    if (luminance < 128) {
        return theme_core_get_text_for_dark_bg();
    } else {
        return theme_core_get_text_for_light_bg();
    }
}
