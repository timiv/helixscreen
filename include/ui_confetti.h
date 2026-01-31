// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_confetti.h
 * @brief Confetti celebration animation component
 *
 * Provides a canvas-based confetti particle system for celebratory UI moments.
 * Particles burst from a point and fall with gravity, rotating and fading.
 *
 * Usage:
 *   // Create confetti overlay covering the screen
 *   lv_obj_t* confetti = ui_confetti_create(lv_screen_active());
 *
 *   // Trigger burst (e.g., on print completion)
 *   ui_confetti_burst(confetti);
 *
 * The component auto-cleans after animation completes.
 */

#pragma once

#include "lvgl/lvgl.h"

/**
 * @brief Create a confetti canvas overlay
 *
 * Creates a transparent canvas that covers the parent and can display
 * confetti particle animations. The canvas is click-through.
 *
 * @param parent Parent object (typically screen or modal)
 * @return Confetti canvas object
 */
lv_obj_t* ui_confetti_create(lv_obj_t* parent);

/**
 * @brief Trigger a confetti burst animation
 *
 * Spawns particles that burst upward then fall with gravity.
 * Multiple bursts can be triggered; they accumulate.
 *
 * @param confetti Confetti canvas object
 * @param count Number of particles (default 80)
 */
void ui_confetti_burst(lv_obj_t* confetti, int count);

/**
 * @brief Stop and clear all confetti
 *
 * @param confetti Confetti canvas object
 */
void ui_confetti_clear(lv_obj_t* confetti);

/**
 * @brief Register confetti as XML widget <ui_confetti>
 *
 * Enables use in XML:
 *   <ui_confetti name="celebration"/>
 *
 * Trigger via subject binding or programmatically.
 */
void ui_confetti_init();
