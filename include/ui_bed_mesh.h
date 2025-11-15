// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UI_BED_MESH_H
#define UI_BED_MESH_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bed mesh canvas dimensions
#define BED_MESH_CANVAS_WIDTH 600
#define BED_MESH_CANVAS_HEIGHT 400

// Rotation angle ranges and defaults
#define BED_MESH_ROTATION_X_MIN (-85)
#define BED_MESH_ROTATION_X_MAX (-10)
#define BED_MESH_ROTATION_X_DEFAULT (-80)
#define BED_MESH_ROTATION_Z_MIN 0
#define BED_MESH_ROTATION_Z_MAX 360
#define BED_MESH_ROTATION_Z_DEFAULT 15

/**
 * @brief Register <bed_mesh> widget with LVGL XML system
 *
 * Creates a canvas widget (600×400 RGB888) optimized for 3D bed mesh rendering.
 * Automatically allocates buffer memory and renderer in create handler.
 *
 * Usage in XML:
 * @code{.xml}
 * <bed_mesh name="my_canvas" width="600" height="400"/>
 * @endcode
 */
void ui_bed_mesh_register(void);

/**
 * @brief Set mesh data for rendering
 *
 * Updates the renderer with new mesh height data. Mesh layout is row-major:
 * - mesh[row][col] where row = Y-axis (front to back)
 * - col = X-axis (left to right)
 * - values are absolute Z heights from printer bed
 *
 * @param canvas The bed_mesh canvas widget
 * @param mesh 2D array of height values (row-major)
 * @param rows Number of rows in mesh
 * @param cols Number of columns in mesh
 * @return true on success, false on error (NULL pointer, invalid dimensions)
 */
bool ui_bed_mesh_set_data(lv_obj_t* canvas, const float* const* mesh, int rows, int cols);

/**
 * @brief Set camera rotation angles
 *
 * @param canvas The bed_mesh canvas widget
 * @param angle_x Tilt angle in degrees (typically -85 to -10, negative = looking down)
 * @param angle_z Spin angle in degrees (horizontal rotation around vertical axis)
 */
void ui_bed_mesh_set_rotation(lv_obj_t* canvas, int angle_x, int angle_z);

/**
 * @brief Force redraw of mesh visualization
 *
 * Clears the canvas and re-renders the mesh with current rotation angles.
 *
 * @param canvas The bed_mesh canvas widget
 */
void ui_bed_mesh_redraw(lv_obj_t* canvas);

#ifdef __cplusplus
}
#endif

#endif // UI_BED_MESH_H
