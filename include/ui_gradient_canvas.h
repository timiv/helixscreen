// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UI_GRADIENT_CANVAS_H
#define UI_GRADIENT_CANVAS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * @brief Register the ui_gradient_canvas widget with the LVGL XML system
 *
 * Creates a canvas widget that draws a procedural diagonal gradient
 * from start_color (lower-left) to end_color (upper-right). This replaces
 * static PNG gradients to avoid scaling artifacts and banding issues.
 *
 * XML attributes:
 *   - width, height: Canvas dimensions (required for buffer allocation)
 *   - dither: "true" to enable ordered dithering for 16-bit displays
 *   - start_color: Hex color for lower-left corner (default: "505050")
 *   - end_color: Hex color for upper-right corner (default: "000000")
 *
 * Color format: Standard LVGL hex colors - "RRGGBB" or "#RRGGBB" (e.g., "FF00FF" for magenta)
 *
 * Example:
 *   <ui_gradient_canvas width="100%" height="100%" dither="true"
 *                       start_color="00FFFF" end_color="FF00FF"/>
 *
 * Must be called before any XML files using <ui_gradient_canvas> are loaded.
 */
void ui_gradient_canvas_register(void);

/**
 * @brief Redraw the gradient on an existing canvas
 *
 * Call this if the canvas is resized or needs to be refreshed.
 *
 * @param canvas Pointer to the canvas object
 */
void ui_gradient_canvas_redraw(lv_obj_t* canvas);

/**
 * @brief Enable or disable dithering for a gradient canvas
 *
 * @param canvas Pointer to the canvas object
 * @param enable true to enable ordered dithering
 */
void ui_gradient_canvas_set_dither(lv_obj_t* canvas, bool enable);

#ifdef __cplusplus
}
#endif

#endif // UI_GRADIENT_CANVAS_H
