// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace helix {

/**
 * @brief Show startup splash screen with HelixScreen logo
 *
 * Displays a centered, scaled logo with fade-in animation.
 * Blocks for the splash duration (2 seconds by default).
 * Must be called after LVGL and theme initialization.
 *
 * @param screen_width Display width in pixels (for logo scaling)
 * @param screen_height Display height in pixels (for logo scaling)
 */
void show_splash_screen(int screen_width, int screen_height);

} // namespace helix
