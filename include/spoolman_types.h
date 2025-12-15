// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

/**
 * @file spoolman_types.h
 * @brief Data structures for Spoolman filament tracking integration
 *
 * Types for interacting with Spoolman, the open-source filament manager.
 * Used by the Spoolman panel, AMS integration, and filament tracking features.
 */

// ============================================================================
// Spoolman Data Types
// ============================================================================

/**
 * @brief Filament spool information from Spoolman
 */
struct SpoolInfo {
    int id = 0;                    ///< Spoolman spool ID
    std::string vendor;            ///< Filament vendor (e.g., "Hatchbox", "Prusament")
    std::string material;          ///< Material type (e.g., "PLA", "PETG", "ABS", "TPU")
    std::string color_name;        ///< Color name (e.g., "Galaxy Black", "Jet Black")
    std::string color_hex;         ///< Hex color code (e.g., "#1A1A2E")
    std::string multi_color_hexes; ///< Comma-separated hex codes for multi-color filaments
                                   ///< (e.g., "#D4AF37,#C0C0C0,#B87333" for gold/silver/copper)
    double remaining_weight_g = 0; ///< Remaining filament weight in grams
    double remaining_length_m = 0; ///< Remaining filament length in meters
    double spool_weight_g = 0;     ///< Empty spool weight in grams
    double initial_weight_g = 0;   ///< Initial filament weight when new
    bool is_active = false;        ///< True if this is the currently tracked spool

    // Temperature recommendations from filament database
    int nozzle_temp_min = 0;
    int nozzle_temp_max = 0;
    int nozzle_temp_recommended = 0;
    int bed_temp_min = 0;
    int bed_temp_max = 0;
    int bed_temp_recommended = 0;

    /**
     * @brief Get remaining percentage
     * @return Percentage of filament remaining (0-100)
     */
    [[nodiscard]] double remaining_percent() const {
        if (initial_weight_g <= 0)
            return 0;
        return (remaining_weight_g / initial_weight_g) * 100.0;
    }

    /**
     * @brief Check if filament is running low
     * @param threshold_grams Warning threshold in grams
     * @return true if remaining weight is below threshold
     */
    [[nodiscard]] bool is_low(double threshold_grams = 100.0) const {
        return remaining_weight_g < threshold_grams;
    }

    /**
     * @brief Check if this is a multi-color filament
     * @return true if multi_color_hexes contains color data
     */
    [[nodiscard]] bool is_multi_color() const {
        return !multi_color_hexes.empty();
    }

    /**
     * @brief Get display name combining vendor, material, and color
     */
    [[nodiscard]] std::string display_name() const {
        std::string name;
        if (!vendor.empty())
            name += vendor + " ";
        if (!material.empty())
            name += material;
        if (!color_name.empty())
            name += " - " + color_name;
        return name.empty() ? "Unknown Spool" : name;
    }
};

/**
 * @brief Filament usage record for history tracking
 */
struct FilamentUsageRecord {
    int spool_id = 0;
    double used_weight_g = 0;
    double used_length_m = 0;
    std::string print_filename;
    double timestamp = 0; ///< Unix timestamp
};

// ============================================================================
// Spoolman Callback Types
// ============================================================================

/// Spool list callback
using SpoolListCallback = std::function<void(const std::vector<SpoolInfo>&)>;

/// Single spool callback (optional - empty if not found)
using SpoolCallback = std::function<void(const std::optional<SpoolInfo>&)>;

/// Filament usage history callback
using FilamentUsageCallback = std::function<void(const std::vector<FilamentUsageRecord>&)>;
