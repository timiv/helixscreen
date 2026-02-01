// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_ui_severity_card.cpp
 * @brief Unit tests for ui_severity_card.cpp - Reactive severity card widget
 *
 * Tests cover:
 * - Severity card border color matches shared style from theme_core
 * - Severity card border color updates reactively when theme changes
 *
 * Phase 2.5: ui_severity_card should use theme_core_get_severity_*_style() instead of
 * inline styles. This enables automatic theme reactivity via LVGL's style system.
 */

#include "../lvgl_ui_test_fixture.h"
#include "theme_compat.h"

#include "../catch_amalgamated.hpp"

// ============================================================================
// Reactive Severity Card Tests - Phase 2.5
// ============================================================================
// These tests verify that severity_card widgets update their border color when the
// theme changes. The old implementation used inline styles (lv_obj_set_style_border_color)
// which don't respond to theme changes.
//
// The fix makes ui_severity_card use lv_obj_add_style() with the shared severity style
// from theme_core, which updates in-place when theme_core_preview_colors() is called.
// ============================================================================

// Helper to create a base test palette
static theme_palette_t make_base_test_palette() {
    theme_palette_t p = {};
    p.screen_bg = lv_color_hex(0x121212);
    p.overlay_bg = lv_color_hex(0x1E1E1E);
    p.card_bg = lv_color_hex(0x2D2D2D);
    p.elevated_bg = lv_color_hex(0x424242);
    p.border = lv_color_hex(0x424242);
    p.text = lv_color_hex(0xE0E0E0);
    p.text_muted = lv_color_hex(0xB0B0B0);
    p.text_subtle = lv_color_hex(0x757575);
    p.primary = lv_color_hex(0xFF5722);
    p.secondary = lv_color_hex(0xFF8A65);
    p.tertiary = lv_color_hex(0xFFAB91);
    p.info = lv_color_hex(0x42A5F5);
    p.success = lv_color_hex(0x66BB6A);
    p.warning = lv_color_hex(0xFFA726);
    p.danger = lv_color_hex(0xEF5350);
    p.focus = lv_color_hex(0x4FC3F7);
    return p;
}

// Helper to create a test palette with a specific warning color
static theme_palette_t make_test_palette_with_warning(lv_color_t warning_color) {
    theme_palette_t p = {};
    p.screen_bg = lv_color_hex(0x121212);
    p.overlay_bg = lv_color_hex(0x1E1E1E);
    p.card_bg = lv_color_hex(0x2D2D2D);
    p.elevated_bg = lv_color_hex(0x424242);
    p.border = lv_color_hex(0x424242);
    p.text = lv_color_hex(0xE0E0E0);
    p.text_muted = lv_color_hex(0xB0B0B0);
    p.text_subtle = lv_color_hex(0x757575);
    p.primary = lv_color_hex(0xFF5722);
    p.secondary = lv_color_hex(0xFF8A65);
    p.tertiary = lv_color_hex(0xFFAB91);
    p.info = lv_color_hex(0x42A5F5);
    p.success = lv_color_hex(0x66BB6A);
    p.warning = warning_color;
    p.danger = lv_color_hex(0xEF5350);
    p.focus = lv_color_hex(0x4FC3F7);
    return p;
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_severity_card: border color matches shared severity style",
                 "[reactive-severity][reactive]") {
    // Create severity_card widget via XML with info severity (default)
    const char* attrs[] = {"severity", "info", nullptr};
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs));
    REQUIRE(card != nullptr);

    // Get the card's border color
    lv_color_t card_border_color = lv_obj_get_style_border_color(card, LV_PART_MAIN);

    // Get expected color from the shared severity info style
    lv_style_t* severity_style = theme_core_get_severity_info_style();
    REQUIRE(severity_style != nullptr);
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(severity_style, LV_STYLE_BORDER_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Severity card should have the same border color as the shared style
    REQUIRE(lv_color_eq(card_border_color, value.color));

    lv_obj_delete(card);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_severity_card: border color updates on theme change",
                 "[reactive-severity][reactive]") {
    // Create severity_card widget via XML with warning severity
    const char* attrs[] = {"severity", "warning", nullptr};
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs));
    REQUIRE(card != nullptr);

    // Get initial border color
    lv_color_t before = lv_obj_get_style_border_color(card, LV_PART_MAIN);

    uint32_t before_rgb = lv_color_to_u32(before) & 0x00FFFFFF;
    INFO("Initial severity card border color: 0x" << std::hex << before_rgb);

    // Update theme via theme_core_preview_colors with DIFFERENT warning color
    // Using bright magenta to ensure visible change
    theme_palette_t dark_palette = make_test_palette_with_warning(lv_color_hex(0xFF00FF));

    theme_core_preview_colors(true, &dark_palette, 8, 100);

    // Force LVGL style refresh cascade
    lv_obj_report_style_change(nullptr);

    // Get updated border color
    lv_color_t after = lv_obj_get_style_border_color(card, LV_PART_MAIN);

    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    INFO("After theme change severity card border color: 0x" << std::hex << after_rgb);

    // Severity card border color should change (warning color changed)
    // This will FAIL with inline style implementation, PASS with shared style
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(card);
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "ui_severity_card: style matches shared style after theme change",
                 "[reactive-severity][reactive]") {
    // Create severity_card widget via XML with error severity
    const char* attrs[] = {"severity", "error", nullptr};
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs));
    REQUIRE(card != nullptr);

    // Get the shared danger style (error maps to danger)
    lv_style_t* shared_style = theme_core_get_severity_danger_style();
    REQUIRE(shared_style != nullptr);

    // Update theme via theme_core_preview_colors with different danger color
    // Using hot pink to ensure visible change
    theme_palette_t dark_palette = make_base_test_palette();
    dark_palette.danger = lv_color_hex(0xFF1493); // HOT PINK

    theme_core_preview_colors(true, &dark_palette, 8, 100);
    lv_obj_report_style_change(nullptr);

    // Get the updated color from the shared style
    lv_style_value_t style_value;
    lv_style_res_t res = lv_style_get_prop(shared_style, LV_STYLE_BORDER_COLOR, &style_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Get the actual color from the severity card widget
    lv_color_t card_color = lv_obj_get_style_border_color(card, LV_PART_MAIN);

    // Log for debugging
    uint32_t style_rgb = lv_color_to_u32(style_value.color) & 0x00FFFFFF;
    uint32_t card_rgb = lv_color_to_u32(card_color) & 0x00FFFFFF;
    INFO("Shared style border_color: 0x" << std::hex << style_rgb);
    INFO("Severity card actual border_color: 0x" << std::hex << card_rgb);

    // The severity card should have the same color as the shared style after update
    // This verifies the card is actually using the shared style
    REQUIRE(lv_color_eq(card_color, style_value.color));

    lv_obj_delete(card);
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "ui_severity_card: multiple cards update together on theme change",
                 "[reactive-severity][reactive]") {
    // Create multiple severity cards with different severities
    const char* attrs_info[] = {"severity", "info", nullptr};
    const char* attrs_warning[] = {"severity", "warning", nullptr};
    const char* attrs_error[] = {"severity", "error", nullptr};
    const char* attrs_success[] = {"severity", "success", nullptr};

    lv_obj_t* card_info =
        static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs_info));
    lv_obj_t* card_warning =
        static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs_warning));
    lv_obj_t* card_error =
        static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs_error));
    lv_obj_t* card_success =
        static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "severity_card", attrs_success));

    REQUIRE(card_info != nullptr);
    REQUIRE(card_warning != nullptr);
    REQUIRE(card_error != nullptr);
    REQUIRE(card_success != nullptr);

    // Get initial colors
    lv_color_t before_info = lv_obj_get_style_border_color(card_info, LV_PART_MAIN);
    lv_color_t before_warning = lv_obj_get_style_border_color(card_warning, LV_PART_MAIN);
    lv_color_t before_error = lv_obj_get_style_border_color(card_error, LV_PART_MAIN);
    lv_color_t before_success = lv_obj_get_style_border_color(card_success, LV_PART_MAIN);

    // Update theme via theme_core_preview_colors with ALL different semantic colors
    theme_palette_t dark_palette = make_base_test_palette();
    dark_palette.success = lv_color_hex(0x00FF00); // BRIGHT GREEN
    dark_palette.warning = lv_color_hex(0xFFFF00); // BRIGHT YELLOW
    dark_palette.danger = lv_color_hex(0xFF0000);  // PURE RED
    dark_palette.info = lv_color_hex(0x0000FF);    // PURE BLUE

    theme_core_preview_colors(true, &dark_palette, 8, 100);
    lv_obj_report_style_change(nullptr);

    // Get colors after theme change
    lv_color_t after_info = lv_obj_get_style_border_color(card_info, LV_PART_MAIN);
    lv_color_t after_warning = lv_obj_get_style_border_color(card_warning, LV_PART_MAIN);
    lv_color_t after_error = lv_obj_get_style_border_color(card_error, LV_PART_MAIN);
    lv_color_t after_success = lv_obj_get_style_border_color(card_success, LV_PART_MAIN);

    // All severity cards should have changed (reactivity)
    REQUIRE_FALSE(lv_color_eq(before_info, after_info));
    REQUIRE_FALSE(lv_color_eq(before_warning, after_warning));
    REQUIRE_FALSE(lv_color_eq(before_error, after_error));
    REQUIRE_FALSE(lv_color_eq(before_success, after_success));

    // Each severity should have a distinct color (correctness)
    REQUIRE_FALSE(lv_color_eq(after_info, after_warning));
    REQUIRE_FALSE(lv_color_eq(after_info, after_error));
    REQUIRE_FALSE(lv_color_eq(after_info, after_success));
    REQUIRE_FALSE(lv_color_eq(after_warning, after_error));
    REQUIRE_FALSE(lv_color_eq(after_warning, after_success));
    REQUIRE_FALSE(lv_color_eq(after_error, after_success));

    lv_obj_delete(card_info);
    lv_obj_delete(card_warning);
    lv_obj_delete(card_error);
    lv_obj_delete(card_success);
}
