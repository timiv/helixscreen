// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors
/**
 * @file app_constants.h
 * @brief Centralized application constants and configuration values
 *
 * This file contains application-wide constants, safety limits, and configuration
 * values shared between frontend (UI) and backend (business logic) code.
 * Centralizing these values ensures consistency and makes the codebase easier
 * to maintain.
 *
 * These constants are usable by both UI components and backend services.
 */

#pragma once

#include "lvgl.h"

#include <chrono>

/**
 * @brief Application-wide constants shared between UI and backend
 */
namespace AppConstants {
/**
 * @brief Temperature-related constants
 *
 * Safety limits and default values for temperature control.
 * Used by both UI panels and backend temperature management.
 */
namespace Temperature {
/// Minimum safe temperature for extrusion operations (Klipper default)
constexpr int MIN_EXTRUSION_TEMP = 170;

/// Default maximum temperature for nozzle/hotend
constexpr int DEFAULT_NOZZLE_MAX = 500;

/// Default maximum temperature for heated bed
constexpr int DEFAULT_BED_MAX = 150;

/// Default minimum temperature (ambient)
constexpr int DEFAULT_MIN_TEMP = 0;
} // namespace Temperature

/**
 * @brief Responsive layout breakpoints
 *
 * These define the screen height thresholds for different UI layouts.
 * Use these consistently across all panels for uniform responsive behavior.
 */
namespace Responsive {
/// Tiny screens: <= 479px height
constexpr lv_coord_t BREAKPOINT_TINY_MAX = 479;

/// Small screens: 480-599px height
constexpr lv_coord_t BREAKPOINT_SMALL_MAX = 599;

/// Medium screens: 600-1023px height
constexpr lv_coord_t BREAKPOINT_MEDIUM_MAX = 1023;

/// Large screens: >= 1024px height
// (No max defined - anything above MEDIUM is large)
} // namespace Responsive

/**
 * @brief Material temperature presets
 *
 * Common filament material extrusion temperatures.
 * These can be overridden by user settings in the future.
 */
namespace MaterialPresets {
constexpr int PLA = 210;
constexpr int PETG = 240;
constexpr int ABS = 250;
constexpr int CUSTOM_DEFAULT = 200;
} // namespace MaterialPresets

/**
 * @brief Startup timing constants
 *
 * Grace periods for suppressing notifications during initial boot.
 * On embedded devices, Moonraker connection may take 10+ seconds.
 */
namespace Startup {
/// Grace period for suppressing initial state notifications (filament, Klipper ready)
constexpr std::chrono::seconds NOTIFICATION_GRACE_PERIOD{10};
} // namespace Startup
} // namespace AppConstants
