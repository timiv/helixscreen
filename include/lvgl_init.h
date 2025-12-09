// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "display_backend.h"

#include <lvgl.h>
#include <memory>

namespace helix {

/**
 * @brief Context holding LVGL display resources
 *
 * Holds ownership of the display backend and provides access to
 * the LVGL display and input device handles. The backend must remain
 * alive for the duration of the application (it owns the framebuffer).
 */
struct LvglContext {
    std::unique_ptr<DisplayBackend> backend; ///< Display backend (owns framebuffer/SDL window)
    lv_display_t* display = nullptr;         ///< LVGL display handle
    lv_indev_t* pointer = nullptr;           ///< Mouse/touch input device (may be null on desktop)
};

/**
 * @brief Initialize LVGL with auto-detected display backend
 *
 * Creates the LVGL display backend (DRM → framebuffer → SDL auto-detect),
 * initializes the display at the specified resolution, and sets up input
 * devices (pointer and optional keyboard).
 *
 * On embedded platforms (DRM/fbdev), missing input device is fatal.
 * On desktop (SDL), pointer is optional.
 *
 * @param width Screen width in pixels
 * @param height Screen height in pixels
 * @param ctx Output context containing display resources
 * @return true on success, false on failure (logged)
 */
bool init_lvgl(int width, int height, LvglContext& ctx);

/**
 * @brief Deinitialize LVGL and release resources
 *
 * Should be called before program exit. The context will be invalidated.
 *
 * @param ctx Context to deinitialize (will be reset)
 */
void deinit_lvgl(LvglContext& ctx);

} // namespace helix
