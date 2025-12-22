// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>

// ============================================================================
// Filename Utilities
// ============================================================================

/**
 * @brief Extract basename from a file path
 *
 * Returns just the filename portion, stripping any directory path.
 * Examples: "/path/to/file.gcode" -> "file.gcode", "file.gcode" -> "file.gcode"
 *
 * @param path Full path or filename
 * @return Filename only (basename)
 */
std::string get_filename_basename(const std::string& path);

/**
 * @brief Strip G-code file extensions for display
 *
 * Removes common G-code extensions (.gcode, .g, .gco, case-insensitive)
 * for cleaner display in the UI.
 *
 * @param filename The original filename
 * @return Filename without G-code extension, or original if no match
 */
std::string strip_gcode_extension(const std::string& filename);

/**
 * @brief Get display-friendly filename (basename with extension stripped)
 *
 * Combines get_filename_basename() and strip_gcode_extension() for
 * convenient one-call filename formatting.
 *
 * @param path Full path or filename
 * @return Clean display name (e.g., "/path/to/benchy.gcode" -> "benchy")
 */
std::string get_display_filename(const std::string& path);

/**
 * @brief Resolve a G-code filename to its original/canonical form
 *
 * When HelixScreen modifies a G-code file before printing (e.g., to add
 * filament change commands), it stores the modified file with patterns like:
 * - `.helix_temp/modified_123456789_OriginalName.gcode`
 * - `/tmp/helixscreen_mod_123456_OriginalName.gcode`
 *
 * This function extracts the original filename for metadata/thumbnail lookups.
 * If the path is not a modified temp path, returns the input unchanged.
 *
 * @param path File path that might be a modified temp file
 * @return Original filename if temp pattern matches, otherwise input unchanged
 */
std::string resolve_gcode_filename(const std::string& path);

// ============================================================================
// Time Formatting
// ============================================================================

/**
 * @brief Format print time from minutes to human-readable string
 *
 * Converts duration into compact time format.
 * Examples: "5m", "1h30m", "8h"
 *
 * @param minutes Total print time in minutes
 * @return Formatted time string
 */
std::string format_print_time(int minutes);

/**
 * @brief Format filament weight from grams to human-readable string
 *
 * Converts weight to compact format with appropriate precision.
 * Examples: "2.5g", "45g", "120g"
 *
 * @param grams Filament weight in grams
 * @return Formatted weight string
 */
std::string format_filament_weight(float grams);

/**
 * @brief Format file size from bytes to human-readable string
 *
 * Converts bytes to appropriate unit (KB/MB/GB) with decimal precision.
 * Examples: "1.2 KB", "45 MB", "1.5 GB"
 *
 * @param bytes File size in bytes
 * @return Formatted size string
 */
std::string format_file_size(size_t bytes);

/**
 * @brief Format Unix timestamp to date/time string
 *
 * Converts timestamp to localized date/time format.
 * Respects user's 12/24 hour time format setting.
 * Examples: "Jan 15 2:30 PM" (12H) or "Jan 15 14:30" (24H)
 *
 * @param timestamp Unix timestamp (time_t)
 * @return Formatted date/time string
 */
std::string format_modified_date(time_t timestamp);

/**
 * @brief Format time portion only (no date)
 *
 * Returns time in user's preferred format.
 * Examples: "2:30 PM" (12H) or "14:30" (24H)
 *
 * @param tm_info Pointer to tm struct with time to format
 * @return Formatted time string
 */
std::string format_time(const struct tm* tm_info);

/**
 * @brief Get strftime format string for current time format setting
 *
 * Returns "%H:%M" for 24-hour or "%l:%M %p" for 12-hour format.
 * Useful when you need to incorporate time into a larger format string.
 *
 * @return Format string for strftime
 */
const char* get_time_format_string();

/**
 * @brief Get responsive padding for content areas below headers
 *
 * Returns smaller padding on tiny/small screens for more compact layouts.
 *
 * @param screen_height Current screen height in pixels
 * @return Padding value in pixels (20px for large/medium, 10px for small, 6px for tiny)
 */
lv_coord_t ui_get_header_content_padding(lv_coord_t screen_height);

// ============================================================================
// Color Utilities
// ============================================================================

/**
 * @brief Parse hex color string to RGB integer value
 *
 * Converts color strings like "#ED1C24" or "ED1C24" to 0xRRGGBB format.
 * Returns std::nullopt for invalid input, allowing black (#000000) to be
 * correctly distinguished from parse errors.
 *
 * @param hex_str Color string with optional # prefix (e.g., "#FF0000", "00FF00")
 * @return RGB value as 0xRRGGBB, or std::nullopt if invalid/empty
 *
 * @code
 * auto red = ui_parse_hex_color("#FF0000");    // returns 0xFF0000
 * auto black = ui_parse_hex_color("#000000");  // returns 0x000000 (not nullopt!)
 * auto invalid = ui_parse_hex_color("xyz");    // returns std::nullopt
 * @endcode
 */
std::optional<uint32_t> ui_parse_hex_color(const std::string& hex_str);

/**
 * @brief Calculate perceptual color distance between two RGB colors
 *
 * Uses weighted Euclidean distance with human perception weights
 * (R=0.30, G=0.59, B=0.11 based on luminance). Returns a value where
 * 0 = identical colors, ~100+ = very different colors.
 *
 * @param color1 First color as 0xRRGGBB
 * @param color2 Second color as 0xRRGGBB
 * @return Perceptual distance (0 = identical, larger = more different)
 *
 * @code
 * int dist = ui_color_distance(0xFF0000, 0xFF1111);  // Small (similar reds)
 * int dist2 = ui_color_distance(0xFF0000, 0x0000FF); // Large (red vs blue)
 * @endcode
 */
int ui_color_distance(uint32_t color1, uint32_t color2);

/**
 * @brief Get responsive header height based on screen size
 *
 * Returns smaller header on tiny/small screens for more compact layouts.
 *
 * @param screen_height Current screen height in pixels
 * @return Header height in pixels (60px for large/medium, 48px for small, 40px for tiny)
 */
lv_coord_t ui_get_responsive_header_height(lv_coord_t screen_height);

// ============================================================================
// App-level resize handling
// ============================================================================

/**
 * @brief Callback type for resize notifications
 *
 * Called when display size changes, debounced to avoid excessive calls.
 */
typedef void (*ui_resize_callback_t)(void);

/**
 * @brief Initialize app-level resize handler
 *
 * Sets up automatic monitoring for display size changes.
 * Must be called once during app initialization, after screen is created.
 *
 * @param screen Main screen object to monitor for size changes
 */
void ui_resize_handler_init(lv_obj_t* screen);

/**
 * @brief Register callback for resize events
 *
 * Callbacks are invoked after debounce period (250ms default) to avoid
 * excessive redraws during continuous resize operations.
 *
 * @param callback Function to call when resize occurs
 */
void ui_resize_handler_register(ui_resize_callback_t callback);

// ============================================================================
// Image Scaling Utilities
// ============================================================================

/**
 * Scale image to cover a target area (like CSS object-fit: cover)
 * Image may be cropped but will fill the entire area with no empty space
 *
 * @param image_widget The lv_image widget to scale
 * @param target_width Target width in pixels
 * @param target_height Target height in pixels
 * @return true if scaling succeeded, false if image info could not be obtained
 */
bool ui_image_scale_to_cover(lv_obj_t* image_widget, lv_coord_t target_width,
                             lv_coord_t target_height);

/**
 * Scale image to fit within a target area (like CSS object-fit: contain)
 * Entire image will be visible, may have empty space around it
 *
 * @param image_widget The lv_image widget to scale
 * @param target_width Target width in pixels
 * @param target_height Target height in pixels
 * @param align Alignment within the target area (default: LV_IMAGE_ALIGN_CENTER)
 * @return true if scaling succeeded, false if image info could not be obtained
 */
bool ui_image_scale_to_contain(lv_obj_t* image_widget, lv_coord_t target_width,
                               lv_coord_t target_height,
                               lv_image_align_t align = LV_IMAGE_ALIGN_CENTER);
