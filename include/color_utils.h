// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

/**
 * @file color_utils.h
 * @brief Color manipulation and naming utilities
 *
 * Provides functions for describing colors in natural language,
 * converting between color spaces, and color manipulation.
 */

namespace helix {

/**
 * @brief Get a human-readable description of an RGB color
 *
 * Uses HSL color space to generate descriptive names like:
 * - "Vibrant Red", "Deep Blue", "Light Muted Green"
 * - Special cases: "White", "Black", "Dark Gray"
 *
 * Algorithm ported from Klipper DESCRIBE_COLOR macro.
 *
 * @param rgb RGB color value (0x00RRGGBB format)
 * @return Descriptive color name string
 *
 * @example
 * describe_color(0xFF0000); // Returns "Red" or "Vibrant Red"
 * describe_color(0x808080); // Returns "Gray"
 * describe_color(0x1A237E); // Returns "Dark Blue" or "Deep Indigo"
 */
std::string describe_color(uint32_t rgb);

/**
 * @brief Convert RGB to HSL color space
 *
 * @param rgb RGB color value (0x00RRGGBB format)
 * @param h Output: Hue 0-360
 * @param s Output: Saturation 0-100
 * @param l Output: Lightness 0-100
 */
void rgb_to_hsl(uint32_t rgb, float& h, float& s, float& l);

} // namespace helix
