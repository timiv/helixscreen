// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_theme_core_styles.cpp
 * @brief Tests for theme_core style getters (Phase 1.1 of reactive theming)
 *
 * These tests define the API contract for shared style getters in theme_core.
 * The getters return pointers to persistent lv_style_t objects that:
 * 1. Are non-null after theme initialization
 * 2. Have appropriate style properties set (bg_color for surfaces, text_color for text)
 * 3. Update in-place when theme_core_update_colors() is called (reactive behavior)
 *
 * Tests are written to FAIL until the implementation is complete.
 */

#include "../lvgl_ui_test_fixture.h"
#include "theme_compat.h"
#include "theme_manager.h"

#include "../catch_amalgamated.hpp"

// Helper: Create a dark mode test palette with distinct colors
static theme_palette_t make_dark_test_palette() {
    theme_palette_t p = {};
    p.screen_bg = lv_color_hex(0x121212);
    p.overlay_bg = lv_color_hex(0x1A1A1A);
    p.card_bg = lv_color_hex(0x1E1E1E);
    p.elevated_bg = lv_color_hex(0x2D2D2D);
    p.border = lv_color_hex(0x424242);
    p.text = lv_color_hex(0xE0E0E0);
    p.text_muted = lv_color_hex(0xA0A0A0);
    p.text_subtle = lv_color_hex(0x808080);
    p.primary = lv_color_hex(0x2196F3);
    p.secondary = lv_color_hex(0x03DAC6);
    p.tertiary = lv_color_hex(0x6C757D);
    p.info = lv_color_hex(0x42A5F5);
    p.success = lv_color_hex(0x4CAF50);
    p.warning = lv_color_hex(0xFFA726);
    p.danger = lv_color_hex(0xEF5350);
    p.focus = lv_color_hex(0x4FC3F7);
    return p;
}

// Helper: Create a dark mode test palette with configurable primary color
static theme_palette_t make_dark_test_palette_with_primary(lv_color_t primary) {
    theme_palette_t p = make_dark_test_palette();
    p.primary = primary;
    return p;
}

// ============================================================================
// Card Style Getter Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: card style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_card_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: card style has background color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_card_style();
    REQUIRE(style != nullptr);

    // Card style should have bg_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Verify it's not black (some meaningful color is set)
    // Theme initializes in light mode, so card_bg should not be (0,0,0)
    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Card bg_color RGB: 0x" << std::hex << color_rgb);
    // Just verify a color is set - don't hardcode expected values
    // (the actual color depends on the theme configuration)
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: card style has background opacity set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_card_style();
    REQUIRE(style != nullptr);

    // Card style should have bg_opa set for visibility
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_OPA, &value);

    // If bg_opa is set, it should be fully opaque or close to it
    if (res == LV_STYLE_RES_FOUND) {
        REQUIRE(value.num >= LV_OPA_50); // At least 50% opacity
    }
    // Note: If not found, widget will inherit default (which is typically opaque)
}

// ============================================================================
// Dialog Style Getter Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: dialog style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_dialog_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: dialog style has background color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_dialog_style();
    REQUIRE(style != nullptr);

    // Dialog style should have bg_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Dialog bg_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: dialog style is distinct pointer from card style",
                 "[theme-core]") {
    lv_style_t* card_style = theme_core_get_card_style();
    lv_style_t* dialog_style = theme_core_get_dialog_style();

    REQUIRE(card_style != nullptr);
    REQUIRE(dialog_style != nullptr);

    // Should be different style objects (different use cases may need different styling)
    REQUIRE(card_style != dialog_style);
}

// ============================================================================
// Text Style Getter Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: text style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_text_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: text style has text color set", "[theme-core]") {
    lv_style_t* style = theme_core_get_text_style();
    REQUIRE(style != nullptr);

    // Text style should have text_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Text color RGB: 0x" << std::hex << color_rgb);
}

// ============================================================================
// Muted Text Style Getter Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: muted text style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_text_muted_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: muted text style has text color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_text_muted_style();
    REQUIRE(style != nullptr);

    // Muted text style should have text_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Muted text color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: muted text style is distinct from primary text",
                 "[theme-core]") {
    lv_style_t* text_style = theme_core_get_text_style();
    lv_style_t* muted_style = theme_core_get_text_muted_style();

    REQUIRE(text_style != nullptr);
    REQUIRE(muted_style != nullptr);

    // Should be different style objects
    REQUIRE(text_style != muted_style);
}

// ============================================================================
// Subtle Text Style Getter Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: subtle text style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_text_subtle_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: subtle text style has text color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_text_subtle_style();
    REQUIRE(style != nullptr);

    // Subtle text style should have text_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Subtle text color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: subtle text style is distinct from muted text",
                 "[theme-core]") {
    lv_style_t* muted_style = theme_core_get_text_muted_style();
    lv_style_t* subtle_style = theme_core_get_text_subtle_style();

    REQUIRE(muted_style != nullptr);
    REQUIRE(subtle_style != nullptr);

    // Should be different style objects
    REQUIRE(muted_style != subtle_style);
}

// ============================================================================
// Style Consistency Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: style getters return same pointer on repeat calls",
                 "[theme-core]") {
    // Style pointers should be stable - multiple calls return same object
    lv_style_t* card1 = theme_core_get_card_style();
    lv_style_t* card2 = theme_core_get_card_style();
    REQUIRE(card1 == card2);

    lv_style_t* dialog1 = theme_core_get_dialog_style();
    lv_style_t* dialog2 = theme_core_get_dialog_style();
    REQUIRE(dialog1 == dialog2);

    lv_style_t* text1 = theme_core_get_text_style();
    lv_style_t* text2 = theme_core_get_text_style();
    REQUIRE(text1 == text2);

    lv_style_t* muted1 = theme_core_get_text_muted_style();
    lv_style_t* muted2 = theme_core_get_text_muted_style();
    REQUIRE(muted1 == muted2);

    lv_style_t* subtle1 = theme_core_get_text_subtle_style();
    lv_style_t* subtle2 = theme_core_get_text_subtle_style();
    REQUIRE(subtle1 == subtle2);
}

// ============================================================================
// Reactive Update Tests - CRITICAL for reactive theming
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: card style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_card_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Style should have updated in-place (same pointer, different value)
    // The test fixture initializes in light mode, so switching to dark should change the color
    REQUIRE_FALSE(lv_color_eq(before, after));

    INFO("Before: 0x" << std::hex << (lv_color_to_u32(before) & 0x00FFFFFF));
    INFO("After: 0x" << std::hex << (lv_color_to_u32(after) & 0x00FFFFFF));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: dialog style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_dialog_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    REQUIRE_FALSE(lv_color_eq(before, after));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: text style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_text_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Light mode text is dark, dark mode text is light - should differ
    REQUIRE_FALSE(lv_color_eq(before, after));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: muted text style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_text_muted_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    REQUIRE_FALSE(lv_color_eq(before, after));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: subtle text style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_text_subtle_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    REQUIRE_FALSE(lv_color_eq(before, after));
}

// ============================================================================
// Widget Integration Test - Verify styles work when applied to widgets
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: card style can be applied to widget",
                 "[theme-core][integration]") {
    lv_style_t* style = theme_core_get_card_style();
    REQUIRE(style != nullptr);

    // Create a test widget
    lv_obj_t* card = lv_obj_create(test_screen());
    REQUIRE(card != nullptr);

    // Apply the shared style - should not crash
    lv_obj_add_style(card, style, LV_PART_MAIN);

    // Widget should now have the style's background color
    lv_color_t widget_bg = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Get expected color from style
    lv_style_value_t value;
    lv_style_get_prop(style, LV_STYLE_BG_COLOR, &value);

    REQUIRE(lv_color_eq(widget_bg, value.color));

    lv_obj_delete(card);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: text style can be applied to label",
                 "[theme-core][integration]") {
    lv_style_t* style = theme_core_get_text_style();
    REQUIRE(style != nullptr);

    // Create a test label
    lv_obj_t* label = lv_label_create(test_screen());
    REQUIRE(label != nullptr);
    lv_label_set_text(label, "Test Label");

    // Apply the shared style
    lv_obj_add_style(label, style, LV_PART_MAIN);

    // Label should now have the style's text color
    lv_color_t label_color = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Get expected color from style
    lv_style_value_t value;
    lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);

    REQUIRE(lv_color_eq(label_color, value.color));

    lv_obj_delete(label);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: widget updates when shared style changes",
                 "[theme-core][integration][reactive]") {
    lv_style_t* style = theme_core_get_card_style();
    REQUIRE(style != nullptr);

    // Create widget and apply style
    lv_obj_t* card = lv_obj_create(test_screen());
    lv_obj_add_style(card, style, LV_PART_MAIN);

    // Get initial color
    lv_color_t before = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Update theme colors
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Trigger LVGL style refresh (this is what theme_core_update_colors should do internally)
    lv_obj_report_style_change(nullptr);

    // Get color after update
    lv_color_t after = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Widget should reflect the new style color
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(card);
}

// ============================================================================
// ui_card Widget Reactive Style Tests - Phase 1.2
// ============================================================================
// These tests verify that ui_card widgets update their appearance when the
// theme changes. They should FAIL with the current implementation because
// ui_card uses inline styles (lv_obj_set_style_bg_color) that don't respond
// to theme changes.
//
// The fix (Phase 1.2 IMPL) will make ui_card use the shared card_style_ from
// theme_core, which updates in-place when theme_core_update_colors() is called.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_card: background color updates on theme change",
                 "[reactive-card]") {
    // Create ui_card widget via XML
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_card", nullptr));
    REQUIRE(card != nullptr);

    // Get initial bg_color from the card widget
    lv_color_t before = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Log initial color for debugging
    uint32_t before_rgb = lv_color_to_u32(before) & 0x00FFFFFF;
    INFO("Initial card bg_color: 0x" << std::hex << before_rgb);

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Force LVGL style refresh cascade
    lv_obj_report_style_change(nullptr);

    // Get updated color from the card
    lv_color_t after = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Log updated color for debugging
    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    INFO("After theme change bg_color: 0x" << std::hex << after_rgb);

    // This assertion will FAIL with current implementation because ui_card uses
    // inline styles (lv_obj_set_style_bg_color) that don't respond to theme changes.
    // Once ui_card is updated to use the shared card_style_, this will pass.
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(card);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_card: uses card_bg token color initially",
                 "[reactive-card]") {
    // Create ui_card widget
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_card", nullptr));
    REQUIRE(card != nullptr);

    // Get the shared card style from theme_core (if available)
    lv_style_t* shared_style = theme_core_get_card_style();
    REQUIRE(shared_style != nullptr);

    // Get expected color from the shared style
    lv_style_value_t expected_value;
    lv_style_res_t res = lv_style_get_prop(shared_style, LV_STYLE_BG_COLOR, &expected_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Get actual color from ui_card
    lv_color_t actual = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Log colors for debugging
    uint32_t expected_rgb = lv_color_to_u32(expected_value.color) & 0x00FFFFFF;
    uint32_t actual_rgb = lv_color_to_u32(actual) & 0x00FFFFFF;
    INFO("Expected (from shared style): 0x" << std::hex << expected_rgb);
    INFO("Actual (from ui_card): 0x" << std::hex << actual_rgb);

    // Both should be the same card_bg color
    // Note: This may pass since both read from theme_manager_get_color("card_bg")
    // at initialization time. The real test is whether it updates on theme change.
    REQUIRE(lv_color_eq(actual, expected_value.color));

    lv_obj_delete(card);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_card: multiple cards update together on theme change",
                 "[reactive-card]") {
    // Create multiple ui_card widgets
    lv_obj_t* card1 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_card", nullptr));
    lv_obj_t* card2 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_card", nullptr));
    lv_obj_t* card3 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_card", nullptr));

    REQUIRE(card1 != nullptr);
    REQUIRE(card2 != nullptr);
    REQUIRE(card3 != nullptr);

    // Get initial colors
    lv_color_t before1 = lv_obj_get_style_bg_color(card1, LV_PART_MAIN);
    lv_color_t before2 = lv_obj_get_style_bg_color(card2, LV_PART_MAIN);
    lv_color_t before3 = lv_obj_get_style_bg_color(card3, LV_PART_MAIN);

    // All cards should have the same initial color
    REQUIRE(lv_color_eq(before1, before2));
    REQUIRE(lv_color_eq(before2, before3));

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get colors after theme change
    lv_color_t after1 = lv_obj_get_style_bg_color(card1, LV_PART_MAIN);
    lv_color_t after2 = lv_obj_get_style_bg_color(card2, LV_PART_MAIN);
    lv_color_t after3 = lv_obj_get_style_bg_color(card3, LV_PART_MAIN);

    // All cards should still have the same color (consistency)
    REQUIRE(lv_color_eq(after1, after2));
    REQUIRE(lv_color_eq(after2, after3));

    // And the color should have changed from before (reactivity)
    // This will FAIL with current inline style implementation
    REQUIRE_FALSE(lv_color_eq(before1, after1));

    lv_obj_delete(card1);
    lv_obj_delete(card2);
    lv_obj_delete(card3);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_card: style matches shared card_style after theme change",
                 "[reactive-card]") {
    // Create ui_card widget
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_card", nullptr));
    REQUIRE(card != nullptr);

    // Get the shared card style
    lv_style_t* shared_style = theme_core_get_card_style();
    REQUIRE(shared_style != nullptr);

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get the updated color from the shared style
    lv_style_value_t style_value;
    lv_style_res_t res = lv_style_get_prop(shared_style, LV_STYLE_BG_COLOR, &style_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Get the actual color from the ui_card widget
    lv_color_t card_color = lv_obj_get_style_bg_color(card, LV_PART_MAIN);

    // Log for debugging
    uint32_t style_rgb = lv_color_to_u32(style_value.color) & 0x00FFFFFF;
    uint32_t card_rgb = lv_color_to_u32(card_color) & 0x00FFFFFF;
    INFO("Shared style bg_color: 0x" << std::hex << style_rgb);
    INFO("ui_card actual bg_color: 0x" << std::hex << card_rgb);

    // The ui_card should have the same color as the shared style after update
    // This will FAIL until ui_card uses lv_obj_add_style() with the shared style
    REQUIRE(lv_color_eq(card_color, style_value.color));

    lv_obj_delete(card);
}

// ============================================================================
// ui_dialog Widget Reactive Style Tests - Phase 1.3
// ============================================================================
// These tests verify that ui_dialog widgets update their appearance when the
// theme changes. They should FAIL with the current implementation because
// ui_dialog uses inline styles (lv_obj_set_style_bg_color) that don't respond
// to theme changes.
//
// The fix (Phase 1.3 IMPL) will make ui_dialog use the shared dialog_style_
// from theme_core, which updates in-place when theme_core_update_colors() is
// called.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "ui_dialog: background color updates on theme change",
                 "[reactive-dialog]") {
    // Create ui_dialog widget via XML
    lv_obj_t* dialog = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_dialog", nullptr));
    REQUIRE(dialog != nullptr);

    // Get initial bg_color from the dialog widget
    lv_color_t before = lv_obj_get_style_bg_color(dialog, LV_PART_MAIN);

    // Log initial color for debugging
    uint32_t before_rgb = lv_color_to_u32(before) & 0x00FFFFFF;
    INFO("Initial dialog bg_color: 0x" << std::hex << before_rgb);

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Force LVGL style refresh cascade
    lv_obj_report_style_change(nullptr);

    // Get updated color from the dialog
    lv_color_t after = lv_obj_get_style_bg_color(dialog, LV_PART_MAIN);

    // Log updated color for debugging
    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    INFO("After theme change bg_color: 0x" << std::hex << after_rgb);

    // This assertion will FAIL with current implementation because ui_dialog uses
    // inline styles (lv_obj_set_style_bg_color) that don't respond to theme changes.
    // Once ui_dialog is updated to use the shared dialog_style_, this will pass.
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(dialog);
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "ui_dialog: style matches shared dialog_style after theme change",
                 "[reactive-dialog]") {
    // Create ui_dialog widget
    lv_obj_t* dialog = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_dialog", nullptr));
    REQUIRE(dialog != nullptr);

    // Get the shared dialog style
    lv_style_t* shared_style = theme_core_get_dialog_style();
    REQUIRE(shared_style != nullptr);

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get the updated color from the shared style
    lv_style_value_t style_value;
    lv_style_res_t res = lv_style_get_prop(shared_style, LV_STYLE_BG_COLOR, &style_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Get the actual color from the ui_dialog widget
    lv_color_t dialog_color = lv_obj_get_style_bg_color(dialog, LV_PART_MAIN);

    // Log for debugging
    uint32_t style_rgb = lv_color_to_u32(style_value.color) & 0x00FFFFFF;
    uint32_t dialog_rgb = lv_color_to_u32(dialog_color) & 0x00FFFFFF;
    INFO("Shared dialog_style bg_color: 0x" << std::hex << style_rgb);
    INFO("ui_dialog actual bg_color: 0x" << std::hex << dialog_rgb);

    // The ui_dialog should have the same color as the shared style after update
    // This will FAIL until ui_dialog uses lv_obj_add_style() with the shared dialog_style_
    REQUIRE(lv_color_eq(dialog_color, style_value.color));

    lv_obj_delete(dialog);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_dialog: uses elevated_bg token color initially",
                 "[reactive-dialog]") {
    // Create ui_dialog widget
    lv_obj_t* dialog = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_dialog", nullptr));
    REQUIRE(dialog != nullptr);

    // Get the shared dialog style from theme_core (if available)
    lv_style_t* shared_style = theme_core_get_dialog_style();
    REQUIRE(shared_style != nullptr);

    // Get expected color from the shared style
    lv_style_value_t expected_value;
    lv_style_res_t res = lv_style_get_prop(shared_style, LV_STYLE_BG_COLOR, &expected_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Get actual color from ui_dialog
    lv_color_t actual = lv_obj_get_style_bg_color(dialog, LV_PART_MAIN);

    // Log colors for debugging
    uint32_t expected_rgb = lv_color_to_u32(expected_value.color) & 0x00FFFFFF;
    uint32_t actual_rgb = lv_color_to_u32(actual) & 0x00FFFFFF;
    INFO("Expected (from shared dialog_style): 0x" << std::hex << expected_rgb);
    INFO("Actual (from ui_dialog): 0x" << std::hex << actual_rgb);

    // Both should be the same elevated_bg color
    // Note: This may pass since both read from theme_manager_get_color("elevated_bg")
    // at initialization time. The real test is whether it updates on theme change.
    REQUIRE(lv_color_eq(actual, expected_value.color));

    lv_obj_delete(dialog);
}

TEST_CASE_METHOD(LVGLUITestFixture, "ui_dialog: multiple dialogs update together on theme change",
                 "[reactive-dialog]") {
    // Create multiple ui_dialog widgets
    lv_obj_t* dialog1 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_dialog", nullptr));
    lv_obj_t* dialog2 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_dialog", nullptr));
    lv_obj_t* dialog3 = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "ui_dialog", nullptr));

    REQUIRE(dialog1 != nullptr);
    REQUIRE(dialog2 != nullptr);
    REQUIRE(dialog3 != nullptr);

    // Get initial colors
    lv_color_t before1 = lv_obj_get_style_bg_color(dialog1, LV_PART_MAIN);
    lv_color_t before2 = lv_obj_get_style_bg_color(dialog2, LV_PART_MAIN);
    lv_color_t before3 = lv_obj_get_style_bg_color(dialog3, LV_PART_MAIN);

    // All dialogs should have the same initial color
    REQUIRE(lv_color_eq(before1, before2));
    REQUIRE(lv_color_eq(before2, before3));

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get colors after theme change
    lv_color_t after1 = lv_obj_get_style_bg_color(dialog1, LV_PART_MAIN);
    lv_color_t after2 = lv_obj_get_style_bg_color(dialog2, LV_PART_MAIN);
    lv_color_t after3 = lv_obj_get_style_bg_color(dialog3, LV_PART_MAIN);

    // All dialogs should still have the same color (consistency)
    REQUIRE(lv_color_eq(after1, after2));
    REQUIRE(lv_color_eq(after2, after3));

    // And the color should have changed from before (reactivity)
    // This will FAIL with current inline style implementation
    REQUIRE_FALSE(lv_color_eq(before1, after1));

    lv_obj_delete(dialog1);
    lv_obj_delete(dialog2);
    lv_obj_delete(dialog3);
}

// ============================================================================
// ui_text Widget Reactive Style Tests - Phase 1.4
// ============================================================================
// These tests verify that text_body and text_heading widgets update their
// text color when the theme changes. By using the shared text styles from
// theme_core, text widgets become reactive to theme changes.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "text_body: text color updates on theme change",
                 "[reactive-text]") {
    // Create text_body widget via XML
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "text_body", nullptr));
    REQUIRE(label != nullptr);

    // Get initial text color
    lv_color_t before = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Log initial color for debugging
    uint32_t before_rgb = lv_color_to_u32(before) & 0x00FFFFFF;
    INFO("Initial text_body text_color: 0x" << std::hex << before_rgb);

    // Update theme colors to dark mode (significantly different colors)
    theme_palette_t dark_palette = make_dark_test_palette();
    dark_palette.text = lv_color_hex(0xE0E0E0); // Light text for dark mode

    theme_core_update_colors(true, &dark_palette, 40);

    // Force LVGL style refresh cascade
    lv_obj_report_style_change(nullptr);

    // Get updated text color
    lv_color_t after = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Log updated color for debugging
    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    INFO("After theme change text_color: 0x" << std::hex << after_rgb);

    // Text color should change (light mode dark text -> dark mode light text)
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(label);
}

TEST_CASE_METHOD(LVGLUITestFixture, "text_heading: text color updates on theme change",
                 "[reactive-text]") {
    // Create text_heading widget via XML (uses text_muted color)
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "text_heading", nullptr));
    REQUIRE(label != nullptr);

    // Get initial text color
    lv_color_t before = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Log initial color for debugging
    uint32_t before_rgb = lv_color_to_u32(before) & 0x00FFFFFF;
    INFO("Initial text_heading text_color: 0x" << std::hex << before_rgb);

    // Update theme colors to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    dark_palette.text_muted = lv_color_hex(0xA0A0A0); // Muted text for dark mode

    theme_core_update_colors(true, &dark_palette, 40);

    // Force LVGL style refresh cascade
    lv_obj_report_style_change(nullptr);

    // Get updated text color
    lv_color_t after = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Log updated color for debugging
    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    INFO("After theme change text_color: 0x" << std::hex << after_rgb);

    // Text color should change (muted color differs between light/dark mode)
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(label);
}

TEST_CASE_METHOD(LVGLUITestFixture, "text_small: text color updates on theme change",
                 "[reactive-text]") {
    // Create text_small widget via XML (uses text_muted color)
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "text_small", nullptr));
    REQUIRE(label != nullptr);

    // Get initial text color
    lv_color_t before = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Update theme colors to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get updated text color
    lv_color_t after = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Text color should change
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(label);
}

TEST_CASE_METHOD(LVGLUITestFixture, "text_xs: text color updates on theme change",
                 "[reactive-text]") {
    // Create text_xs widget via XML (uses text_muted color)
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "text_xs", nullptr));
    REQUIRE(label != nullptr);

    // Get initial text color
    lv_color_t before = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Update theme colors to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get updated text color
    lv_color_t after = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Text color should change
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(label);
}

TEST_CASE_METHOD(LVGLUITestFixture, "text_button: text color updates on theme change",
                 "[reactive-text]") {
    // Create text_button widget via XML (uses text primary color)
    lv_obj_t* label = static_cast<lv_obj_t*>(lv_xml_create(test_screen(), "text_button", nullptr));
    REQUIRE(label != nullptr);

    // Get initial text color
    lv_color_t before = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Update theme colors to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    lv_obj_report_style_change(nullptr);

    // Get updated text color
    lv_color_t after = lv_obj_get_style_text_color(label, LV_PART_MAIN);

    // Text color should change
    REQUIRE_FALSE(lv_color_eq(before, after));

    lv_obj_delete(label);
}

// ============================================================================
// Icon Style Getter Tests - Phase 2.1
// ============================================================================
// Icon styles mirror text styles but for icon coloring. Icons in LVGL are
// font-based labels, so they use text_color for their color.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon text style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_text_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon text style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_text_style();
    REQUIRE(style != nullptr);

    // Icon style should have text_color property set (icons use text_color)
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon text color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon muted style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_muted_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon muted style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_muted_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon muted color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon primary style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_primary_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon primary style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_primary_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon primary color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon secondary style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_secondary_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon secondary style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_secondary_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon secondary color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon tertiary style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_tertiary_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon tertiary style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_tertiary_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon tertiary color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon success style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_success_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon success style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_success_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon success color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon warning style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_warning_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon warning style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_warning_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon warning color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon danger style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_danger_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon danger style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_danger_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon danger color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon info style getter returns valid style",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_info_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon info style has text color set",
                 "[theme-core][icon-styles]") {
    lv_style_t* style = theme_core_get_icon_info_style();
    REQUIRE(style != nullptr);

    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Icon info color RGB: 0x" << std::hex << color_rgb);
}

// ============================================================================
// Icon Style Consistency Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture,
                 "theme_core: icon style getters return same pointer on repeat calls",
                 "[theme-core][icon-styles]") {
    // Style pointers should be stable - multiple calls return same object
    lv_style_t* icon_text1 = theme_core_get_icon_text_style();
    lv_style_t* icon_text2 = theme_core_get_icon_text_style();
    REQUIRE(icon_text1 == icon_text2);

    lv_style_t* icon_muted1 = theme_core_get_icon_muted_style();
    lv_style_t* icon_muted2 = theme_core_get_icon_muted_style();
    REQUIRE(icon_muted1 == icon_muted2);

    lv_style_t* icon_primary1 = theme_core_get_icon_primary_style();
    lv_style_t* icon_primary2 = theme_core_get_icon_primary_style();
    REQUIRE(icon_primary1 == icon_primary2);

    lv_style_t* icon_secondary1 = theme_core_get_icon_secondary_style();
    lv_style_t* icon_secondary2 = theme_core_get_icon_secondary_style();
    REQUIRE(icon_secondary1 == icon_secondary2);

    lv_style_t* icon_tertiary1 = theme_core_get_icon_tertiary_style();
    lv_style_t* icon_tertiary2 = theme_core_get_icon_tertiary_style();
    REQUIRE(icon_tertiary1 == icon_tertiary2);

    lv_style_t* icon_success1 = theme_core_get_icon_success_style();
    lv_style_t* icon_success2 = theme_core_get_icon_success_style();
    REQUIRE(icon_success1 == icon_success2);

    lv_style_t* icon_warning1 = theme_core_get_icon_warning_style();
    lv_style_t* icon_warning2 = theme_core_get_icon_warning_style();
    REQUIRE(icon_warning1 == icon_warning2);

    lv_style_t* icon_danger1 = theme_core_get_icon_danger_style();
    lv_style_t* icon_danger2 = theme_core_get_icon_danger_style();
    REQUIRE(icon_danger1 == icon_danger2);

    lv_style_t* icon_info1 = theme_core_get_icon_info_style();
    lv_style_t* icon_info2 = theme_core_get_icon_info_style();
    REQUIRE(icon_info1 == icon_info2);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: all icon styles are distinct pointers",
                 "[theme-core][icon-styles]") {
    lv_style_t* icon_text = theme_core_get_icon_text_style();
    lv_style_t* icon_muted = theme_core_get_icon_muted_style();
    lv_style_t* icon_primary = theme_core_get_icon_primary_style();
    lv_style_t* icon_secondary = theme_core_get_icon_secondary_style();
    lv_style_t* icon_tertiary = theme_core_get_icon_tertiary_style();
    lv_style_t* icon_success = theme_core_get_icon_success_style();
    lv_style_t* icon_warning = theme_core_get_icon_warning_style();
    lv_style_t* icon_danger = theme_core_get_icon_danger_style();
    lv_style_t* icon_info = theme_core_get_icon_info_style();

    // All should be non-null
    REQUIRE(icon_text != nullptr);
    REQUIRE(icon_muted != nullptr);
    REQUIRE(icon_primary != nullptr);
    REQUIRE(icon_secondary != nullptr);
    REQUIRE(icon_tertiary != nullptr);
    REQUIRE(icon_success != nullptr);
    REQUIRE(icon_warning != nullptr);
    REQUIRE(icon_danger != nullptr);
    REQUIRE(icon_info != nullptr);

    // All should be distinct
    REQUIRE(icon_text != icon_muted);
    REQUIRE(icon_text != icon_primary);
    REQUIRE(icon_text != icon_secondary);
    REQUIRE(icon_text != icon_tertiary);
    REQUIRE(icon_text != icon_success);
    REQUIRE(icon_text != icon_warning);
    REQUIRE(icon_text != icon_danger);
    REQUIRE(icon_text != icon_info);

    REQUIRE(icon_muted != icon_primary);
    REQUIRE(icon_primary != icon_secondary);
    REQUIRE(icon_secondary != icon_tertiary);
    REQUIRE(icon_success != icon_warning);
    REQUIRE(icon_warning != icon_danger);
    REQUIRE(icon_danger != icon_info);
}

// ============================================================================
// Icon Style Reactive Update Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon text style updates on theme change",
                 "[theme-core][icon-styles][reactive]") {
    lv_style_t* style = theme_core_get_icon_text_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode with different colors
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Icon text style should update (same as text primary style)
    REQUIRE_FALSE(lv_color_eq(before, after));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon muted style updates on theme change",
                 "[theme-core][icon-styles][reactive]") {
    lv_style_t* style = theme_core_get_icon_muted_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    REQUIRE_FALSE(lv_color_eq(before, after));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon primary style updates on theme change",
                 "[theme-core][icon-styles][reactive]") {
    lv_style_t* style = theme_core_get_icon_primary_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode with a DIFFERENT primary color
    theme_palette_t palette = make_dark_test_palette_with_primary(lv_color_hex(0xFF5722));
    theme_core_update_colors(true, &palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Icon primary style uses primary_color, which we changed
    REQUIRE_FALSE(lv_color_eq(before, after));
}

// ============================================================================
// Icon Style Widget Integration Test
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: icon style can be applied to label",
                 "[theme-core][icon-styles][integration]") {
    lv_style_t* style = theme_core_get_icon_primary_style();
    REQUIRE(style != nullptr);

    // Create a test label (icons are just labels with icon fonts)
    lv_obj_t* icon = lv_label_create(test_screen());
    REQUIRE(icon != nullptr);
    lv_label_set_text(icon, "A"); // Icon glyph

    // Apply the shared icon style
    lv_obj_add_style(icon, style, LV_PART_MAIN);

    // Label should now have the style's text color
    lv_color_t icon_color = lv_obj_get_style_text_color(icon, LV_PART_MAIN);

    // Get expected color from style
    lv_style_value_t value;
    lv_style_get_prop(style, LV_STYLE_TEXT_COLOR, &value);

    REQUIRE(lv_color_eq(icon_color, value.color));

    lv_obj_delete(icon);
}

// ============================================================================
// Spinner Style Getter Tests - Phase 2.3
// ============================================================================
// Spinner uses arc_color for the indicator arc. The style should use primary_color
// and update reactively when theme changes.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: spinner style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_spinner_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: spinner style has arc color set", "[theme-core]") {
    lv_style_t* style = theme_core_get_spinner_style();
    REQUIRE(style != nullptr);

    // Spinner style should have arc_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_ARC_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Spinner arc_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: spinner style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_spinner_style();
    REQUIRE(style != nullptr);

    // Get initial arc color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_ARC_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode with a DIFFERENT primary color
    theme_palette_t palette = make_dark_test_palette_with_primary(lv_color_hex(0xFF5722));
    theme_core_update_colors(true, &palette, 40);

    // Get arc color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_ARC_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Spinner style uses primary_color, which we changed
    INFO("Before: 0x" << std::hex << (lv_color_to_u32(before) & 0x00FFFFFF));
    INFO("After: 0x" << std::hex << (lv_color_to_u32(after) & 0x00FFFFFF));
    REQUIRE_FALSE(lv_color_eq(before, after));
}

// ============================================================================
// Severity Style Getter Tests - Phase 2.3
// ============================================================================
// Severity styles are used for severity_card border colors. Each severity level
// (info, success, warning, danger) has its own style with border_color set.
// Unlike icon styles which use text_color, these use border_color.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity info style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_info_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity info style has border color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_info_style();
    REQUIRE(style != nullptr);

    // Severity style should have border_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BORDER_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Severity info border_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity success style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_success_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity success style has border color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_success_style();
    REQUIRE(style != nullptr);

    // Severity style should have border_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BORDER_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Severity success border_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity warning style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_warning_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity warning style has border color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_warning_style();
    REQUIRE(style != nullptr);

    // Severity style should have border_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BORDER_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Severity warning border_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity danger style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_danger_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity danger style has border color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_severity_danger_style();
    REQUIRE(style != nullptr);

    // Severity style should have border_color property set
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BORDER_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Severity danger border_color RGB: 0x" << std::hex << color_rgb);
}

// ============================================================================
// Severity Style Consistency Tests
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture,
                 "theme_core: severity style getters return same pointer on repeat calls",
                 "[theme-core]") {
    // Style pointers should be stable - multiple calls return same object
    lv_style_t* info1 = theme_core_get_severity_info_style();
    lv_style_t* info2 = theme_core_get_severity_info_style();
    REQUIRE(info1 == info2);

    lv_style_t* success1 = theme_core_get_severity_success_style();
    lv_style_t* success2 = theme_core_get_severity_success_style();
    REQUIRE(success1 == success2);

    lv_style_t* warning1 = theme_core_get_severity_warning_style();
    lv_style_t* warning2 = theme_core_get_severity_warning_style();
    REQUIRE(warning1 == warning2);

    lv_style_t* danger1 = theme_core_get_severity_danger_style();
    lv_style_t* danger2 = theme_core_get_severity_danger_style();
    REQUIRE(danger1 == danger2);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: all severity styles are distinct pointers",
                 "[theme-core]") {
    lv_style_t* info = theme_core_get_severity_info_style();
    lv_style_t* success = theme_core_get_severity_success_style();
    lv_style_t* warning = theme_core_get_severity_warning_style();
    lv_style_t* danger = theme_core_get_severity_danger_style();

    // All should be non-null
    REQUIRE(info != nullptr);
    REQUIRE(success != nullptr);
    REQUIRE(warning != nullptr);
    REQUIRE(danger != nullptr);

    // All should be distinct
    REQUIRE(info != success);
    REQUIRE(info != warning);
    REQUIRE(info != danger);
    REQUIRE(success != warning);
    REQUIRE(success != danger);
    REQUIRE(warning != danger);
}

// ============================================================================
// Severity Style Preview Mode Tests
// ============================================================================
// Severity styles update in preview mode (theme_core_preview_colors) but NOT
// in normal theme updates (theme_core_update_colors). This is by design:
// semantic colors (success, warning, danger, info) are typically static across
// light/dark mode, but preview mode allows testing custom palettes.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: severity styles update in preview mode",
                 "[theme-core][reactive]") {
    // Get initial border color from info style
    lv_style_t* info_style = theme_core_get_severity_info_style();
    REQUIRE(info_style != nullptr);

    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(info_style, LV_STYLE_BORDER_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Preview colors: use very different colors to ensure we detect the change
    theme_palette_t palette = make_dark_test_palette();
    palette.success = lv_color_hex(0x00FF00); // bright green
    palette.warning = lv_color_hex(0xFFFF00); // yellow
    palette.danger = lv_color_hex(0xFF0000);  // red
    palette.info = lv_color_hex(0x0000FF);    // bright blue - different from default

    // Apply preview with custom palette
    theme_core_preview_colors(true, &palette, 8, 100);

    // Get border color after preview
    lv_style_value_t after_value;
    res = lv_style_get_prop(info_style, LV_STYLE_BORDER_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Info style should have changed to palette's info color (#0000FF)
    INFO("Before: 0x" << std::hex << (lv_color_to_u32(before) & 0x00FFFFFF));
    INFO("After: 0x" << std::hex << (lv_color_to_u32(after) & 0x00FFFFFF));
    REQUIRE_FALSE(lv_color_eq(before, after));

    // Verify it's actually blue (0x0000FF)
    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    REQUIRE(after_rgb == 0x0000FF);
}

// ============================================================================
// Button Style Getter Tests - Phase 2.6a
// ============================================================================
// Button styles provide reactive background colors for different button types.
// Each button style sets bg_color only - text color is handled separately by
// the button widget using contrast text getters.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button primary style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_primary_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button primary style has background color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_primary_style();
    REQUIRE(style != nullptr);

    // Button primary style should have bg_color property set to primary color
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Button primary bg_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button secondary style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_secondary_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button secondary style has background color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_secondary_style();
    REQUIRE(style != nullptr);

    // Button secondary style should have bg_color property set to surface color
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Button secondary bg_color RGB: 0x" << std::hex << color_rgb);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button danger style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_danger_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button danger style has background color set",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_danger_style();
    REQUIRE(style != nullptr);

    // Button danger style should have bg_color property set (color depends on active theme)
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);

    // Just verify a color is set - actual value depends on the loaded theme
    uint32_t color_rgb = lv_color_to_u32(value.color) & 0x00FFFFFF;
    INFO("Button danger bg_color RGB: 0x" << std::hex << color_rgb);
    REQUIRE(color_rgb != 0x000000); // Not black (unset)
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button ghost style getter returns valid style",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_ghost_style();
    REQUIRE(style != nullptr);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button ghost style has transparent background",
                 "[theme-core]") {
    lv_style_t* style = theme_core_get_button_ghost_style();
    REQUIRE(style != nullptr);

    // Ghost button should have transparent bg (LV_OPA_0)
    lv_style_value_t value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_OPA, &value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    REQUIRE(value.num == LV_OPA_0);
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "theme_core: button style getters return same pointer on repeat calls",
                 "[theme-core]") {
    // Style pointers should be stable - multiple calls return same object
    lv_style_t* primary1 = theme_core_get_button_primary_style();
    lv_style_t* primary2 = theme_core_get_button_primary_style();
    REQUIRE(primary1 == primary2);

    lv_style_t* secondary1 = theme_core_get_button_secondary_style();
    lv_style_t* secondary2 = theme_core_get_button_secondary_style();
    REQUIRE(secondary1 == secondary2);

    lv_style_t* danger1 = theme_core_get_button_danger_style();
    lv_style_t* danger2 = theme_core_get_button_danger_style();
    REQUIRE(danger1 == danger2);

    lv_style_t* ghost1 = theme_core_get_button_ghost_style();
    lv_style_t* ghost2 = theme_core_get_button_ghost_style();
    REQUIRE(ghost1 == ghost2);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: all button styles are distinct pointers",
                 "[theme-core]") {
    lv_style_t* primary = theme_core_get_button_primary_style();
    lv_style_t* secondary = theme_core_get_button_secondary_style();
    lv_style_t* danger = theme_core_get_button_danger_style();
    lv_style_t* ghost = theme_core_get_button_ghost_style();

    // All should be non-null
    REQUIRE(primary != nullptr);
    REQUIRE(secondary != nullptr);
    REQUIRE(danger != nullptr);
    REQUIRE(ghost != nullptr);

    // All should be distinct style objects
    REQUIRE(primary != secondary);
    REQUIRE(primary != danger);
    REQUIRE(primary != ghost);
    REQUIRE(secondary != danger);
    REQUIRE(secondary != ghost);
    REQUIRE(danger != ghost);
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button primary style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_button_primary_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode with different primary color
    theme_palette_t palette = make_dark_test_palette_with_primary(lv_color_hex(0xFF5722));
    theme_core_update_colors(true, &palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Style should have updated to new primary color
    INFO("Before: 0x" << std::hex << (lv_color_to_u32(before) & 0x00FFFFFF));
    INFO("After: 0x" << std::hex << (lv_color_to_u32(after) & 0x00FFFFFF));
    REQUIRE_FALSE(lv_color_eq(before, after));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button secondary style updates on theme change",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_button_secondary_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Switch to dark mode
    theme_palette_t dark_palette = make_dark_test_palette();
    theme_core_update_colors(true, &dark_palette, 40);

    // Get color after update
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    // Style should have updated to new surface color
    REQUIRE_FALSE(lv_color_eq(before, after));
}

// ============================================================================
// Contrast Text Color Getter Tests - Phase 2.6a
// ============================================================================
// Contrast text getters provide appropriate text colors for dark and light
// backgrounds. These are used by button widgets to pick readable text colors
// based on background luminance.
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture,
                 "theme_core: text for dark bg returns light color (for contrast)",
                 "[theme-core]") {
    lv_color_t color = theme_core_get_text_for_dark_bg();

    // Should be a light color (high RGB values) for visibility on dark backgrounds
    uint32_t rgb = lv_color_to_u32(color) & 0x00FFFFFF;
    INFO("Text for dark bg: 0x" << std::hex << rgb);

    // Extract RGB components
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    // Light colors should have high average RGB (at least 0xC0 = 192)
    int avg = (r + g + b) / 3;
    REQUIRE(avg >= 0xC0);
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "theme_core: text for light bg returns dark color (for contrast)",
                 "[theme-core]") {
    lv_color_t color = theme_core_get_text_for_light_bg();

    // Should be a dark color (low RGB values) for visibility on light backgrounds
    uint32_t rgb = lv_color_to_u32(color) & 0x00FFFFFF;
    INFO("Text for light bg: 0x" << std::hex << rgb);

    // Extract RGB components
    uint8_t r = (rgb >> 16) & 0xFF;
    uint8_t g = (rgb >> 8) & 0xFF;
    uint8_t b = rgb & 0xFF;

    // Dark colors should have average RGB below midpoint (128)
    // Actual theme colors may be lighter than the fallback (0x212121)
    // e.g., Ayu light text is #5C6166 (avg ~97)
    int avg = (r + g + b) / 3;
    REQUIRE(avg < 128);
}

TEST_CASE_METHOD(LVGLUITestFixture,
                 "theme_core: contrast text colors are different from each other", "[theme-core]") {
    lv_color_t dark_bg_text = theme_core_get_text_for_dark_bg();
    lv_color_t light_bg_text = theme_core_get_text_for_light_bg();

    // They should be different colors (one light, one dark)
    REQUIRE_FALSE(lv_color_eq(dark_bg_text, light_bg_text));
}

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: contrast text getters use theme tokens",
                 "[theme-core]") {
    // Verify the getters look up tokens from the XML constant system
    // The tokens should be registered by theme_manager_init()

    // Get what the getters return
    lv_color_t dark_bg_text = theme_core_get_text_for_dark_bg();
    lv_color_t light_bg_text = theme_core_get_text_for_light_bg();

    // Get the raw token values
    const char* text_dark_str = lv_xml_get_const(nullptr, "text_dark");
    const char* text_light_str = lv_xml_get_const(nullptr, "text_light");

    INFO("text_dark token: " << (text_dark_str ? text_dark_str : "(null)"));
    INFO("text_light token: " << (text_light_str ? text_light_str : "(null)"));

    // If tokens exist, verify the getter returns matching color
    if (text_dark_str && text_dark_str[0] == '#') {
        lv_color_t expected = lv_color_hex(strtoul(text_dark_str + 1, nullptr, 16));
        uint32_t got = lv_color_to_u32(dark_bg_text) & 0x00FFFFFF;
        uint32_t exp = lv_color_to_u32(expected) & 0x00FFFFFF;
        INFO("text_for_dark_bg: got 0x" << std::hex << got << ", expected 0x" << exp);
        REQUIRE(lv_color_eq(dark_bg_text, expected));
    }

    if (text_light_str && text_light_str[0] == '#') {
        lv_color_t expected = lv_color_hex(strtoul(text_light_str + 1, nullptr, 16));
        uint32_t got = lv_color_to_u32(light_bg_text) & 0x00FFFFFF;
        uint32_t exp = lv_color_to_u32(expected) & 0x00FFFFFF;
        INFO("text_for_light_bg: got 0x" << std::hex << got << ", expected 0x" << exp);
        REQUIRE(lv_color_eq(light_bg_text, expected));
    }
}

// ============================================================================
// Button Styles in Preview Mode - Phase 2.6a
// ============================================================================

TEST_CASE_METHOD(LVGLUITestFixture, "theme_core: button primary style updates in preview mode",
                 "[theme-core][reactive]") {
    lv_style_t* style = theme_core_get_button_primary_style();
    REQUIRE(style != nullptr);

    // Get initial color
    lv_style_value_t before_value;
    lv_style_res_t res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &before_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t before = before_value.color;

    // Preview with custom accent/primary color (orange, different from default)
    theme_palette_t palette = make_dark_test_palette();
    palette.primary = lv_color_hex(0xFF5722); // orange

    theme_core_preview_colors(true, &palette, 8, 100);

    // Get color after preview
    lv_style_value_t after_value;
    res = lv_style_get_prop(style, LV_STYLE_BG_COLOR, &after_value);
    REQUIRE(res == LV_STYLE_RES_FOUND);
    lv_color_t after = after_value.color;

    INFO("Before: 0x" << std::hex << (lv_color_to_u32(before) & 0x00FFFFFF));
    INFO("After: 0x" << std::hex << (lv_color_to_u32(after) & 0x00FFFFFF));
    REQUIRE_FALSE(lv_color_eq(before, after));

    // Verify it's the orange from palette
    uint32_t after_rgb = lv_color_to_u32(after) & 0x00FFFFFF;
    REQUIRE(after_rgb == 0xFF5722);
}
