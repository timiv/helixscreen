// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file ui_button.h
 * @brief Semantic button widget with variant styles, icons, and auto-contrast
 *
 * Provides a <ui_button> XML widget with:
 * - Semantic variants: primary, secondary, danger, success, tertiary, warning, ghost
 * - Optional icon support with auto-contrast (light icon on dark bg, dark on light)
 * - Auto-contrast text: automatically picks light/dark text based on bg luminance
 * - Reactive styling: updates automatically when theme changes
 *
 * Usage in XML:
 *   <ui_button variant="primary" text="Save"/>
 *   <ui_button variant="danger" text="Delete"/>
 *   <ui_button variant="secondary" text="Cancel"/>
 *   <ui_button variant="ghost" text="Skip"/>
 *   <ui_button icon="heat_wave" text="Dryer"/>
 *   <ui_button icon="settings" text=""/>
 *   <ui_button icon="cog" icon_position="right" text="Settings"/>
 *
 * Attributes:
 * - variant: Button style variant (default: "primary")
 *   - primary: ThemeManager::get_style(StyleRole::ButtonPrimary) (primary/accent color bg)
 *   - secondary: ThemeManager::get_style(StyleRole::ButtonSecondary) (surface color bg)
 *   - danger: ThemeManager::get_style(StyleRole::ButtonDanger) (danger/red color bg)
 *   - success: ThemeManager::get_style(StyleRole::ButtonSuccess) (success/green color bg)
 *   - tertiary: ThemeManager::get_style(StyleRole::ButtonTertiary) (tertiary color bg)
 *   - warning: ThemeManager::get_style(StyleRole::ButtonWarning) (warning/amber color bg)
 *   - ghost: ThemeManager::get_style(StyleRole::ButtonGhost) (transparent bg)
 *
 * - text: Button label text (optional, can be empty for icon-only buttons)
 *
 * - icon: Icon name from MDI font (optional, e.g., "settings", "heat_wave", "cog")
 *   Icon uses auto-contrast just like text.
 *
 * - icon_position: Where to place icon relative to text (default: "left")
 *   - "left": Icon before text
 *   - "right": Icon after text
 *
 * Layout:
 * - Icon + text: Horizontal flex layout with small gap between
 * - Icon only: Centered icon
 * - Text only: Centered text (original behavior)
 *
 * Auto-contrast is computed using luminance formula:
 *   L = (299*R + 587*G + 114*B) / 1000
 * If L < 128 (dark bg): light text/icon (ThemeManager text contrast helpers)
 * If L >= 128 (light bg): dark text/icon (ThemeManager text contrast helpers)
 */

/**
 * @brief Initialize the ui_button custom widget
 *
 * Registers the <ui_button> XML widget with LVGL's XML parser.
 * Must be called after lv_xml_init() and after theme is initialized.
 */
void ui_button_init();
