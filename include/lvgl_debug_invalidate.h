// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors
/**
 * @file lvgl_debug_invalidate.h
 * @brief Debug wrapper for lv_obj_invalidate to catch render-phase calls
 *
 * This header provides a macro to wrap lv_obj_invalidate() calls with
 * additional checking to detect when invalidation is called during
 * the render phase, which causes LVGL assertions.
 *
 * Usage: Include this header and use LV_OBJ_INVALIDATE_SAFE(obj) instead
 * of lv_obj_invalidate(obj) in suspect areas.
 */

#pragma once

#include "lvgl/lvgl.h"
#include "lvgl/src/display/lv_display_private.h" // For rendering_in_progress
#include "spdlog/spdlog.h"

/**
 * @brief Check if LVGL is currently rendering
 *
 * Returns true if lv_obj_invalidate() would trigger an assertion.
 * Uses internal LVGL display state - may need updating for new LVGL versions.
 */
static inline bool lvgl_is_rendering(void) {
    lv_display_t* disp = lv_display_get_default();
    if (!disp)
        return false;

    // Access internal rendering_in_progress flag
    // This is checking the same condition as lv_inv_area's assertion
    // Note: This accesses LVGL internals and may break with LVGL updates
    return disp->rendering_in_progress;
}

/**
 * @brief Safe invalidate that logs if called during render
 *
 * Use this macro in areas where you suspect invalidation during render.
 * It will log a warning with file/line info if called during render,
 * helping identify the source of lv_inv_area assertions.
 *
 * @param obj The LVGL object to invalidate
 */
#define LV_OBJ_INVALIDATE_SAFE(obj)                                                                \
    do {                                                                                           \
        if (lvgl_is_rendering()) {                                                                 \
            spdlog::error("[LVGL DEBUG] lv_obj_invalidate() called during render at {}:{} in {}",  \
                          __FILE__, __LINE__, __func__);                                           \
            spdlog::error("[LVGL DEBUG] This will cause lv_inv_area assertion! Skipping.");        \
        } else {                                                                                   \
            lv_obj_invalidate(obj);                                                                \
        }                                                                                          \
    } while (0)

/**
 * @brief Log current render state for debugging
 *
 * Call this at the start of functions that might invalidate during render
 * to help trace the call chain.
 */
#define LV_DEBUG_RENDER_STATE()                                                                    \
    do {                                                                                           \
        if (lvgl_is_rendering()) {                                                                 \
            spdlog::warn("[LVGL DEBUG] In render phase at {}:{} in {}", __FILE__, __LINE__,        \
                         __func__);                                                                \
        }                                                                                          \
    } while (0)
