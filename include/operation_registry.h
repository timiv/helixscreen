// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "operation_patterns.h"

#include <optional>
#include <string>
#include <vector>

namespace helix {

/**
 * @brief Metadata for a controllable pre-print operation
 *
 * Contains all the information needed to display and identify
 * an operation in the pre-print UI.
 */
struct OperationInfo {
    OperationCategory category; ///< The operation category enum
    std::string capability_key; ///< Machine-readable key (e.g., "bed_mesh")
    std::string friendly_name;  ///< Human-readable name (e.g., "Bed mesh")
};

/**
 * @brief Registry for controllable pre-print operations
 *
 * Provides lookup and iteration over operations that can be toggled
 * in the pre-print UI. Non-controllable operations (HOMING, CHAMBER_SOAK,
 * SKEW_CORRECT, BED_LEVEL, START_PRINT, UNKNOWN) are excluded.
 *
 * Controllable operations:
 * - BED_MESH: Bed mesh calibration
 * - QGL: Quad gantry leveling
 * - Z_TILT: Z-tilt adjustment
 * - NOZZLE_CLEAN: Nozzle cleaning/wiping
 * - PURGE_LINE: Purge/prime line
 */
class OperationRegistry {
  public:
    /**
     * @brief Get operation info by category
     *
     * @param cat The operation category to look up
     * @return OperationInfo if controllable, nullopt otherwise
     */
    static std::optional<OperationInfo> get(OperationCategory cat) {
        if (!is_controllable(cat)) {
            return std::nullopt;
        }
        return OperationInfo{cat, category_key(cat), category_name(cat)};
    }

    /**
     * @brief Reverse lookup by capability key
     *
     * @param key The capability key (e.g., "bed_mesh", "qgl")
     * @return OperationInfo if found and controllable, nullopt otherwise
     */
    static std::optional<OperationInfo> get_by_key(const std::string& key) {
        for (const auto& info : all()) {
            if (info.capability_key == key) {
                return info;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Get all controllable operations
     *
     * @return Reference to vector containing all controllable operation infos
     */
    static const std::vector<OperationInfo>& all() {
        // Meyer's singleton for thread-safe lazy initialization
        static const std::vector<OperationInfo> controllable_ops = build_all();
        return controllable_ops;
    }

  private:
    /**
     * @brief Check if a category is controllable in pre-print UI
     *
     * Controllable operations are those that users can toggle on/off
     * before starting a print.
     *
     * @param cat The operation category to check
     * @return true if controllable, false otherwise
     */
    static bool is_controllable(OperationCategory cat) {
        switch (cat) {
        case OperationCategory::BED_MESH:
        case OperationCategory::QGL:
        case OperationCategory::Z_TILT:
        case OperationCategory::NOZZLE_CLEAN:
        case OperationCategory::PURGE_LINE:
            return true;
        default:
            return false;
        }
    }

    /**
     * @brief Build the vector of all controllable operations
     *
     * Called once during static initialization.
     *
     * @return Vector of OperationInfo for all controllable operations
     */
    static std::vector<OperationInfo> build_all() {
        std::vector<OperationInfo> result;
        // Add all controllable operations in a defined order
        constexpr OperationCategory controllable[] = {
            OperationCategory::BED_MESH,   OperationCategory::QGL,
            OperationCategory::Z_TILT,     OperationCategory::NOZZLE_CLEAN,
            OperationCategory::PURGE_LINE,
        };
        for (auto cat : controllable) {
            result.push_back(OperationInfo{cat, category_key(cat), category_name(cat)});
        }
        return result;
    }
};

} // namespace helix
