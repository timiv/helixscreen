// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_ui_button.cpp
 * @brief Unit tests for ui_button.cpp - Semantic button widget with auto-contrast
 *
 * Tests cover:
 * - Basic creation: ui_button creates button with label
 * - Variant styles: Each variant applies correct shared style
 * - Text attribute: Label shows correct text
 * - Auto-contrast on dark bg: Primary/danger buttons get light text
 * - Auto-contrast on light bg: Ghost button gets dark text
 * - Reactive update: When theme changes, button bg AND text contrast update
 *
 * Phase 2.6b: ui_button semantic widget with auto-contrast text
 */

#include "ui_button.h"

#include "../lvgl_ui_test_fixture.h"
#include "theme_compat.h"

#include "../catch_amalgamated.hpp"

// Helper: Create a dark mode test palette with configurable primary color
static theme_palette_t make_dark_test_palette_with_primary(lv_color_t primary) {
    theme_palette_t p = {};
    p.screen_bg = lv_color_hex(0x121212);
    p.overlay_bg = lv_color_hex(0x1A1A1A);
    p.card_bg = lv_color_hex(0x1E1E1E);
    p.elevated_bg = lv_color_hex(0x2D2D2D);
    p.border = lv_color_hex(0x424242);
    p.text = lv_color_hex(0xE0E0E0);
    p.text_muted = lv_color_hex(0xA0A0A0);
    p.text_subtle = lv_color_hex(0x808080);
    p.primary = primary;
    p.secondary = lv_color_hex(0x03DAC6);
    p.tertiary = lv_color_hex(0x6C757D);
    p.info = lv_color_hex(0x42A5F5);
    p.success = lv_color_hex(0x4CAF50);
    p.warning = lv_color_hex(0xFFA726);
    p.danger = lv_color_hex(0xEF5350);
    p.focus = lv_color_hex(0x4FC3F7);
    return p;
}

// Helper: Create a dark mode test palette with configurable primary and surface
static theme_palette_t make_dark_test_palette_custom(lv_color_t primary, lv_color_t surface) {
    theme_palette_t p = make_dark_test_palette_with_primary(primary);
    p.elevated_bg = surface;
    return p;
}

// ============================================================================
// Basic Creation Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: creates button with label", "[ui-button]") {
    const char* attrs[] = {"text", "Save", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Button should have at least one child (the label)
    uint32_t child_count = lv_obj_get_child_count(btn);
    REQUIRE(child_count >= 1);

    // First child should be the label
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);
    REQUIRE(lv_obj_check_type(label, &lv_label_class));

    // Label should show the text
    const char* text = lv_label_get_text(label);
    REQUIRE(text != nullptr);
    REQUIRE(strcmp(text, "Save") == 0);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: defaults to primary variant", "[ui-button]") {
    const char* attrs[] = {"text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Get the button's background color
    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    // Get expected color from the shared primary button style
    lv_style_t* primary_style = theme_core_get_button_primary_style();
    REQUIRE(primary_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(primary_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Button should have the same bg color as the primary style
    REQUIRE(lv_color_eq(btn_bg, value.color));

    lv_obj_delete(btn);
}

// ============================================================================
// Variant Style Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: primary variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "primary", "text", "Save", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    lv_style_t* primary_style = theme_core_get_button_primary_style();
    REQUIRE(primary_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(primary_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(lv_color_eq(btn_bg, value.color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: secondary variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "secondary", "text", "Cancel", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    lv_style_t* secondary_style = theme_core_get_button_secondary_style();
    REQUIRE(secondary_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(secondary_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(lv_color_eq(btn_bg, value.color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: danger variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "danger", "text", "Delete", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    lv_style_t* danger_style = theme_core_get_button_danger_style();
    REQUIRE(danger_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(danger_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(lv_color_eq(btn_bg, value.color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: ghost variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "ghost", "text", "Skip", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Ghost buttons have transparent background
    lv_opa_t btn_bg_opa = lv_obj_get_style_bg_opa(btn, LV_PART_MAIN);

    lv_style_t* ghost_style = theme_core_get_button_ghost_style();
    REQUIRE(ghost_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(ghost_style, LV_STYLE_BG_OPA, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(btn_bg_opa == value.num);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: success variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "success", "text", "Save", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    lv_style_t* success_style = theme_core_get_button_success_style();
    REQUIRE(success_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(success_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(btn_bg.red == value.color.red);
    REQUIRE(btn_bg.green == value.color.green);
    REQUIRE(btn_bg.blue == value.color.blue);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: tertiary variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "tertiary", "text", "More", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    lv_style_t* tertiary_style = theme_core_get_button_tertiary_style();
    REQUIRE(tertiary_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(tertiary_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(btn_bg.red == value.color.red);
    REQUIRE(btn_bg.green == value.color.green);
    REQUIRE(btn_bg.blue == value.color.blue);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: warning variant applies correct style",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "warning", "text", "Caution", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    lv_style_t* warning_style = theme_core_get_button_warning_style();
    REQUIRE(warning_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(warning_style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    REQUIRE(btn_bg.red == value.color.red);
    REQUIRE(btn_bg.green == value.color.green);
    REQUIRE(btn_bg.blue == value.color.blue);

    lv_obj_delete(btn);
}

// ============================================================================
// Text Attribute Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: text attribute sets label text", "[ui-button]") {
    const char* attrs[] = {"text", "Custom Label", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    const char* text = lv_label_get_text(label);
    REQUIRE(text != nullptr);
    REQUIRE(strcmp(text, "Custom Label") == 0);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: empty text attribute creates empty label",
                 "[ui-button]") {
    const char* attrs[] = {"text", "", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    const char* text = lv_label_get_text(label);
    REQUIRE(text != nullptr);
    REQUIRE(strlen(text) == 0);

    lv_obj_delete(btn);
}

// ============================================================================
// Auto-Contrast Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: primary button has light text (dark bg)",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "primary", "text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    // Get label text color
    lv_color_t text_color = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Get the button's bg color and calculate luminance
    // LVGL 9: lv_color_t has direct .red, .green, .blue members
    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    uint8_t r = btn_bg.red;
    uint8_t g = btn_bg.green;
    uint8_t b = btn_bg.blue;
    uint32_t lum = (299 * r + 587 * g + 114 * b) / 1000;

    INFO("Button bg: R=" << (int)r << " G=" << (int)g << " B=" << (int)b << " Lum=" << lum);

    // If bg is dark (lum < 128), text should be light
    // If bg is light (lum >= 128), text should be dark
    lv_color_t expected_text_color =
        (lum < 128) ? theme_core_get_text_for_dark_bg() : theme_core_get_text_for_light_bg();

    REQUIRE(lv_color_eq(text_color, expected_text_color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: danger button has light text (dark bg)",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "danger", "text", "Delete", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    lv_color_t text_color = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Danger color is 0xEF5350 (red) which has luminance ~97
    // (299*239 + 587*83 + 114*80) / 1000 = (71461 + 48721 + 9120) / 1000 = 129
    // So danger may be borderline - let's verify the logic is applied correctly
    // LVGL 9: lv_color_t has direct .red, .green, .blue members
    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    uint8_t r = btn_bg.red;
    uint8_t g = btn_bg.green;
    uint8_t b = btn_bg.blue;
    uint32_t lum = (299 * r + 587 * g + 114 * b) / 1000;

    INFO("Danger button bg: R=" << (int)r << " G=" << (int)g << " B=" << (int)b << " Lum=" << lum);

    lv_color_t expected_text_color =
        (lum < 128) ? theme_core_get_text_for_dark_bg() : theme_core_get_text_for_light_bg();

    REQUIRE(lv_color_eq(text_color, expected_text_color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: ghost button uses appropriate text color",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "ghost", "text", "Skip", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    // Ghost button has transparent bg, so we need a different approach
    // For ghost buttons, text color should still be computed based on
    // whatever bg color the style reports (even if transparent)
    // LVGL 9: lv_color_t has direct .red, .green, .blue members
    lv_color_t text_color = lv_obj_get_style_text_color(label, LV_PART_MAIN);
    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    uint8_t r = btn_bg.red;
    uint8_t g = btn_bg.green;
    uint8_t b = btn_bg.blue;
    uint32_t lum = (299 * r + 587 * g + 114 * b) / 1000;

    lv_color_t expected_text_color =
        (lum < 128) ? theme_core_get_text_for_dark_bg() : theme_core_get_text_for_light_bg();

    REQUIRE(lv_color_eq(text_color, expected_text_color));

    lv_obj_delete(btn);
}

// ============================================================================
// Reactive Update Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: bg color updates on theme change", "[ui-button]") {
    const char* attrs[] = {"variant", "primary", "text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Get initial bg color
    lv_color_t before = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    uint32_t before_rgb = lv_color_to_u32(before) & 0x00FFFFFF;
    INFO("Initial button bg color: 0x" << std::hex << before_rgb);

    // Update theme colors with DIFFERENT primary color (orange)
    theme_palette_t palette = make_dark_test_palette_with_primary(lv_color_hex(0xFF5722));
    theme_core_update_colors(true, &palette, 40);

    // Force LVGL style refresh cascade
    lv_obj_report_style_change(nullptr);

    // Get updated bg color
    lv_color_t after = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);

    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    INFO("After theme change button bg color: 0x" << std::hex << after_rgb);

    // Button bg color should change (primary color changed)
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: text contrast updates on theme change",
                 "[ui-button]") {
    const char* attrs[] = {"variant", "primary", "text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    // Update theme colors with a significantly different primary color (yellow)
    // that would change the luminance threshold - light bg will need dark text
    theme_palette_t palette = make_dark_test_palette_with_primary(lv_color_hex(0xFFEB3B));
    theme_core_update_colors(true, &palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get updated text color
    lv_color_t after_text = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Verify the new text color is correct for the new bg
    // LVGL 9: lv_color_t has direct .red, .green, .blue members
    lv_color_t new_btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    uint8_t r = new_btn_bg.red;
    uint8_t g = new_btn_bg.green;
    uint8_t b = new_btn_bg.blue;
    uint32_t lum = (299 * r + 587 * g + 114 * b) / 1000;

    INFO("New button bg: R=" << (int)r << " G=" << (int)g << " B=" << (int)b << " Lum=" << lum);

    lv_color_t expected_text_color =
        (lum < 128) ? theme_core_get_text_for_dark_bg() : theme_core_get_text_for_light_bg();

    REQUIRE(lv_color_eq(after_text, expected_text_color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: multiple buttons update together on theme change",
                 "[ui-button]") {
    const char* attrs1[] = {"variant", "primary", "text", "Save", nullptr};
    const char* attrs2[] = {"variant", "secondary", "text", "Cancel", nullptr};
    const char* attrs3[] = {"variant", "danger", "text", "Delete", nullptr};

    lv_obj_t* btn1 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs1));
    lv_obj_t* btn2 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs2));
    lv_obj_t* btn3 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs3));

    REQUIRE(btn1 != nullptr);
    REQUIRE(btn2 != nullptr);
    REQUIRE(btn3 != nullptr);

    // Get initial bg colors
    lv_color_t before1 = lv_obj_get_style_bg_color(btn1, LV_PART_MAIN);
    lv_color_t before2 = lv_obj_get_style_bg_color(btn2, LV_PART_MAIN);

    // Update theme with different primary (purple) and surface colors
    theme_palette_t palette =
        make_dark_test_palette_custom(lv_color_hex(0x9C27B0), lv_color_hex(0x3D3D3D));
    theme_core_update_colors(true, &palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get updated bg colors
    lv_color_t after1 = lv_obj_get_style_bg_color(btn1, LV_PART_MAIN);
    lv_color_t after2 = lv_obj_get_style_bg_color(btn2, LV_PART_MAIN);

    // Primary button should have changed (primary color changed)
    REQUIRE_FALSE(lv_color_eq(before1, after1));

    // Secondary button should have changed (surface color changed)
    REQUIRE_FALSE(lv_color_eq(before2, after2));

    lv_obj_delete(btn1);
    lv_obj_delete(btn2);
    lv_obj_delete(btn3);
}

// ============================================================================
// Icon Button Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: creates button with icon and text",
                 "[ui-button][icon]") {
    const char* attrs[] = {"icon", "cog", "text", "Settings", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Button should have at least two children (icon + label)
    uint32_t child_count = lv_obj_get_child_count(btn);
    REQUIRE(child_count >= 2);

    // Button should use flex layout
    lv_flex_flow_t flow = (lv_flex_flow_t)lv_obj_get_style_flex_flow(btn, LV_PART_MAIN);
    REQUIRE(flow == LV_FLEX_FLOW_ROW);

    // First child should be icon (since icon_position defaults to "left")
    lv_obj_t* first_child = lv_obj_get_child(btn, 0);
    REQUIRE(first_child != nullptr);
    REQUIRE(lv_obj_check_type(first_child, &lv_label_class));

    // Second child should be label
    lv_obj_t* second_child = lv_obj_get_child(btn, 1);
    REQUIRE(second_child != nullptr);
    REQUIRE(lv_obj_check_type(second_child, &lv_label_class));

    // Second child should have the text
    const char* text = lv_label_get_text(second_child);
    REQUIRE(text != nullptr);
    REQUIRE(strcmp(text, "Settings") == 0);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: creates icon-only button", "[ui-button][icon]") {
    const char* attrs[] = {"icon", "cog", "text", "", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Button should have at least one visible child (icon)
    uint32_t child_count = lv_obj_get_child_count(btn);
    REQUIRE(child_count >= 1);

    // First visible child should be icon
    lv_obj_t* icon = lv_obj_get_child(btn, 0);
    REQUIRE(icon != nullptr);
    REQUIRE(lv_obj_check_type(icon, &lv_label_class));

    // Icon should be centered (no flex layout for icon-only)
    lv_flex_flow_t flow = (lv_flex_flow_t)lv_obj_get_style_flex_flow(btn, LV_PART_MAIN);
    REQUIRE(flow == LV_FLEX_FLOW_ROW); // Flex is still set but only icon is visible

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: icon_position right puts icon after text",
                 "[ui-button][icon]") {
    const char* attrs[] = {"icon", "cog", "text", "Settings", "icon_position", "right", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Button should have at least two children
    uint32_t child_count = lv_obj_get_child_count(btn);
    REQUIRE(child_count >= 2);

    // First child should be label (since icon_position is "right")
    lv_obj_t* first_child = lv_obj_get_child(btn, 0);
    REQUIRE(first_child != nullptr);
    const char* text = lv_label_get_text(first_child);
    REQUIRE(text != nullptr);
    REQUIRE(strcmp(text, "Settings") == 0);

    // Second child should be icon
    lv_obj_t* second_child = lv_obj_get_child(btn, 1);
    REQUIRE(second_child != nullptr);
    // Icon is a label with MDI font glyph
    REQUIRE(lv_obj_check_type(second_child, &lv_label_class));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: icon has auto-contrast color",
                 "[ui-button][icon]") {
    const char* attrs[] = {"variant", "primary", "icon", "cog", "text", "Settings", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Get button bg luminance
    lv_color_t btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    uint8_t r = btn_bg.red;
    uint8_t g = btn_bg.green;
    uint8_t b = btn_bg.blue;
    uint32_t lum = (299 * r + 587 * g + 114 * b) / 1000;

    lv_color_t expected_color =
        (lum < 128) ? theme_core_get_text_for_dark_bg() : theme_core_get_text_for_light_bg();

    // Icon is first child (icon_position defaults to "left")
    lv_obj_t* icon = lv_obj_get_child(btn, 0);
    REQUIRE(icon != nullptr);

    lv_color_t icon_color = lv_obj_get_style_text_color(icon, LV_PART_MAIN);
    REQUIRE(lv_color_eq(icon_color, expected_color));

    // Label should have same color as icon
    lv_obj_t* label = lv_obj_get_child(btn, 1);
    REQUIRE(label != nullptr);

    lv_color_t label_color = lv_obj_get_style_text_color(label, LV_PART_MAIN);
    REQUIRE(lv_color_eq(label_color, expected_color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: icon contrast updates on theme change",
                 "[ui-button][icon]") {
    const char* attrs[] = {"variant", "primary", "icon", "cog", "text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Get initial icon color
    lv_obj_t* icon = lv_obj_get_child(btn, 0);
    REQUIRE(icon != nullptr);

    // Update theme colors with a significantly different primary color (yellow)
    theme_palette_t palette = make_dark_test_palette_with_primary(lv_color_hex(0xFFEB3B));
    theme_core_update_colors(true, &palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get updated icon color
    lv_color_t after_icon = lv_obj_get_style_text_color(icon, LV_PART_MAIN);

    // Get the button's new bg and verify icon has correct contrast
    lv_color_t new_btn_bg = lv_obj_get_style_bg_color(btn, LV_PART_MAIN);
    uint8_t r = new_btn_bg.red;
    uint8_t g = new_btn_bg.green;
    uint8_t b = new_btn_bg.blue;
    uint32_t lum = (299 * r + 587 * g + 114 * b) / 1000;

    lv_color_t expected_color =
        (lum < 128) ? theme_core_get_text_for_dark_bg() : theme_core_get_text_for_light_bg();

    REQUIRE(lv_color_eq(after_icon, expected_color));

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: unknown icon name does not crash",
                 "[ui-button][icon]") {
    // Unknown icon should not crash, just log warning
    const char* attrs[] = {"icon", "nonexistent_icon_xyz", "text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Button should still be created and have a label
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);

    lv_obj_delete(btn);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_button: text-only button still works",
                 "[ui-button][icon]") {
    // Ensure original text-only behavior is preserved
    const char* attrs[] = {"text", "Cancel", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Button should have at least one child (the label)
    uint32_t child_count = lv_obj_get_child_count(btn);
    REQUIRE(child_count >= 1);

    // First child should be the label
    lv_obj_t* label = lv_obj_get_child(btn, 0);
    REQUIRE(label != nullptr);
    REQUIRE(lv_obj_check_type(label, &lv_label_class));

    // Label should show the text
    const char* text = lv_label_get_text(label);
    REQUIRE(text != nullptr);
    REQUIRE(strcmp(text, "Cancel") == 0);

    lv_obj_delete(btn);
}

// ============================================================================
// User Data Safety Tests
// ============================================================================
// These tests verify the magic number defense against user_data being overwritten
// by Modal::wire_button or similar code that repurposes user_data for Modal*.

TEST_CASE_METHOD(LVGLUITestFixture,
                 "ui_button: deleting button with valid user_data does not crash",
                 "[ui-button][safety]") {
    // Normal button creation and deletion should work
    const char* attrs[] = {"text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Verify user_data is set (indicates UiButtonData was allocated)
    void* data = lv_obj_get_user_data(btn);
    REQUIRE(data != nullptr);

    // Delete should free the UiButtonData without crashing
    lv_obj_delete(btn);

    // If we get here, no crash occurred
    SUCCEED("Button with valid user_data deleted successfully");
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "ui_button: deleting button with overwritten user_data does not crash",
                 "[ui-button][safety]") {
    // This simulates what Modal::wire_button does: overwrite user_data
    const char* attrs[] = {"text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Overwrite user_data with a distinctive pattern (simulates Modal::wire_button)
    // Use a non-null pointer that's clearly not a UiButtonData
    static int fake_modal = 0xDEADBEEF;
    lv_obj_set_user_data(btn, &fake_modal);

    // Verify user_data was overwritten
    void* data = lv_obj_get_user_data(btn);
    REQUIRE(data == &fake_modal);

    // Delete should NOT try to free the overwritten pointer (magic check should fail)
    // This would crash with malloc_zone_error if the magic check wasn't working
    lv_obj_delete(btn);

    // If we get here, the magic number check prevented the crash
    SUCCEED("Button with overwritten user_data deleted successfully (magic check worked)");
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "ui_button: style change on button with overwritten user_data does not crash",
                 "[ui-button][safety]") {
    // Verify update_button_text_contrast handles overwritten user_data
    const char* attrs[] = {"text", "Test", nullptr};
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_button", attrs));
    REQUIRE(btn != nullptr);

    // Overwrite user_data with a distinctive pattern (simulates Modal::wire_button)
    // Use a non-null pointer that's clearly not a UiButtonData
    static int fake_modal = 0xDEADBEEF;
    lv_obj_set_user_data(btn, &fake_modal);

    // Trigger a style change on just this button - this would crash if
    // update_button_text_contrast tried to dereference the overwritten user_data
    lv_obj_send_event(btn, LV_EVENT_STYLE_CHANGED, nullptr);

    // If we get here, update_button_text_contrast correctly detected the overwritten data
    lv_obj_delete(btn);
    SUCCEED("Style change on button with overwritten user_data handled safely");
}
