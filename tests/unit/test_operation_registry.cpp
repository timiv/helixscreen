// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

// TDD Red Phase: Tests for OperationRegistry class
// These tests will FAIL initially because the header doesn't exist yet.

#include "operation_registry.h"

#include "../catch_amalgamated.hpp"

using namespace helix;

// ============================================================================
// Test 1: Lookup by category returns correct metadata
// ============================================================================

TEST_CASE("OperationRegistry: lookup by category", "[operation_registry][p2]") {
    SECTION("BED_MESH has correct metadata") {
        auto info = OperationRegistry::get(OperationCategory::BED_MESH);
        REQUIRE(info.has_value());
        REQUIRE(info->capability_key == "bed_mesh");
        REQUIRE(info->friendly_name == "Bed mesh");
    }

    SECTION("QGL has correct metadata") {
        auto info = OperationRegistry::get(OperationCategory::QGL);
        REQUIRE(info.has_value());
        REQUIRE(info->capability_key == "qgl");
        REQUIRE(info->friendly_name == "Quad gantry leveling");
    }

    SECTION("Z_TILT has correct metadata") {
        auto info = OperationRegistry::get(OperationCategory::Z_TILT);
        REQUIRE(info.has_value());
        REQUIRE(info->capability_key == "z_tilt");
        REQUIRE(info->friendly_name == "Z-tilt adjustment");
    }

    SECTION("NOZZLE_CLEAN has correct metadata") {
        auto info = OperationRegistry::get(OperationCategory::NOZZLE_CLEAN);
        REQUIRE(info.has_value());
        REQUIRE(info->capability_key == "nozzle_clean");
        REQUIRE(info->friendly_name == "Nozzle cleaning");
    }

    SECTION("PURGE_LINE has correct metadata") {
        auto info = OperationRegistry::get(OperationCategory::PURGE_LINE);
        REQUIRE(info.has_value());
        REQUIRE(info->capability_key == "purge_line");
        REQUIRE(info->friendly_name == "Purge line");
    }

    SECTION("Returned info includes the category") {
        auto info = OperationRegistry::get(OperationCategory::BED_MESH);
        REQUIRE(info.has_value());
        REQUIRE(info->category == OperationCategory::BED_MESH);
    }
}

// ============================================================================
// Test 2: All controllable operations are registered
// ============================================================================

TEST_CASE("OperationRegistry: controllable operations", "[operation_registry][p2]") {
    SECTION("All controllable operations have registry entries") {
        // BED_MESH, QGL, Z_TILT, NOZZLE_CLEAN, PURGE_LINE are controllable
        for (auto cat :
             {OperationCategory::BED_MESH, OperationCategory::QGL, OperationCategory::Z_TILT,
              OperationCategory::NOZZLE_CLEAN, OperationCategory::PURGE_LINE}) {
            INFO("Checking category: " << category_name(cat));
            REQUIRE(OperationRegistry::get(cat).has_value());
        }
    }

    SECTION("Non-controllable operations return nullopt") {
        // HOMING, START_PRINT, UNKNOWN are not controllable
        REQUIRE_FALSE(OperationRegistry::get(OperationCategory::HOMING).has_value());
        REQUIRE_FALSE(OperationRegistry::get(OperationCategory::START_PRINT).has_value());
        REQUIRE_FALSE(OperationRegistry::get(OperationCategory::UNKNOWN).has_value());
    }

    SECTION("BED_LEVEL parent category is not directly controllable") {
        // BED_LEVEL is a parent category for QGL/Z_TILT, not exposed directly
        REQUIRE_FALSE(OperationRegistry::get(OperationCategory::BED_LEVEL).has_value());
    }

    SECTION("CHAMBER_SOAK is not controllable") {
        // Chamber soak is detected but not user-controllable in pre-print UI
        REQUIRE_FALSE(OperationRegistry::get(OperationCategory::CHAMBER_SOAK).has_value());
    }

    SECTION("SKEW_CORRECT is not controllable") {
        // Skew correction is detected but not user-controllable in pre-print UI
        REQUIRE_FALSE(OperationRegistry::get(OperationCategory::SKEW_CORRECT).has_value());
    }
}

// ============================================================================
// Test 3: Reverse lookup by key
// ============================================================================

TEST_CASE("OperationRegistry: reverse lookup by key", "[operation_registry][p2]") {
    SECTION("get_by_key returns correct category for qgl") {
        auto info = OperationRegistry::get_by_key("qgl");
        REQUIRE(info.has_value());
        REQUIRE(info->category == OperationCategory::QGL);
        REQUIRE(info->friendly_name == "Quad gantry leveling");
    }

    SECTION("get_by_key returns correct category for bed_mesh") {
        auto info = OperationRegistry::get_by_key("bed_mesh");
        REQUIRE(info.has_value());
        REQUIRE(info->category == OperationCategory::BED_MESH);
    }

    SECTION("get_by_key returns correct category for z_tilt") {
        auto info = OperationRegistry::get_by_key("z_tilt");
        REQUIRE(info.has_value());
        REQUIRE(info->category == OperationCategory::Z_TILT);
    }

    SECTION("get_by_key returns correct category for nozzle_clean") {
        auto info = OperationRegistry::get_by_key("nozzle_clean");
        REQUIRE(info.has_value());
        REQUIRE(info->category == OperationCategory::NOZZLE_CLEAN);
    }

    SECTION("get_by_key returns correct category for purge_line") {
        auto info = OperationRegistry::get_by_key("purge_line");
        REQUIRE(info.has_value());
        REQUIRE(info->category == OperationCategory::PURGE_LINE);
    }

    SECTION("Unknown key returns nullopt") {
        REQUIRE_FALSE(OperationRegistry::get_by_key("unknown_key").has_value());
        REQUIRE_FALSE(OperationRegistry::get_by_key("homing").has_value());
        REQUIRE_FALSE(OperationRegistry::get_by_key("start_print").has_value());
        REQUIRE_FALSE(OperationRegistry::get_by_key("").has_value());
    }

    SECTION("Keys are case-sensitive (lowercase expected)") {
        // The registry uses lowercase keys like "bed_mesh", not "BED_MESH"
        REQUIRE_FALSE(OperationRegistry::get_by_key("BED_MESH").has_value());
        REQUIRE_FALSE(OperationRegistry::get_by_key("QGL").has_value());
    }
}

// ============================================================================
// Test 4: Iteration over all controllable operations
// ============================================================================

TEST_CASE("OperationRegistry: all() iteration", "[operation_registry][p2]") {
    SECTION("Returns non-empty collection") {
        const auto& all = OperationRegistry::all();
        REQUIRE(all.size() >= 5); // At least BED_MESH, QGL, Z_TILT, NOZZLE_CLEAN, PURGE_LINE
    }

    SECTION("All entries have valid data") {
        for (const auto& info : OperationRegistry::all()) {
            INFO("Checking entry: " << info.capability_key);
            REQUIRE_FALSE(info.capability_key.empty());
            REQUIRE_FALSE(info.friendly_name.empty());
        }
    }

    SECTION("All entries have valid category") {
        for (const auto& info : OperationRegistry::all()) {
            INFO("Checking entry: " << info.capability_key);
            // Category should not be UNKNOWN
            REQUIRE(info.category != OperationCategory::UNKNOWN);
            // Category should not be non-controllable types
            REQUIRE(info.category != OperationCategory::HOMING);
            REQUIRE(info.category != OperationCategory::START_PRINT);
        }
    }

    SECTION("Collection contains expected controllable operations") {
        const auto& all = OperationRegistry::all();

        // Helper to check if a category exists in the collection
        auto has_category = [&all](OperationCategory cat) {
            return std::find_if(all.begin(), all.end(), [cat](const OperationInfo& info) {
                       return info.category == cat;
                   }) != all.end();
        };

        REQUIRE(has_category(OperationCategory::BED_MESH));
        REQUIRE(has_category(OperationCategory::QGL));
        REQUIRE(has_category(OperationCategory::Z_TILT));
        REQUIRE(has_category(OperationCategory::NOZZLE_CLEAN));
        REQUIRE(has_category(OperationCategory::PURGE_LINE));
    }

    SECTION("Collection does not contain non-controllable operations") {
        const auto& all = OperationRegistry::all();

        auto has_category = [&all](OperationCategory cat) {
            return std::find_if(all.begin(), all.end(), [cat](const OperationInfo& info) {
                       return info.category == cat;
                   }) != all.end();
        };

        REQUIRE_FALSE(has_category(OperationCategory::HOMING));
        REQUIRE_FALSE(has_category(OperationCategory::START_PRINT));
        REQUIRE_FALSE(has_category(OperationCategory::UNKNOWN));
        REQUIRE_FALSE(has_category(OperationCategory::BED_LEVEL));
    }

    SECTION("Entries are consistent with get() lookup") {
        for (const auto& info : OperationRegistry::all()) {
            auto lookup = OperationRegistry::get(info.category);
            REQUIRE(lookup.has_value());
            REQUIRE(lookup->capability_key == info.capability_key);
            REQUIRE(lookup->friendly_name == info.friendly_name);
        }
    }

    SECTION("Entries are consistent with get_by_key() lookup") {
        for (const auto& info : OperationRegistry::all()) {
            auto lookup = OperationRegistry::get_by_key(info.capability_key);
            REQUIRE(lookup.has_value());
            REQUIRE(lookup->category == info.category);
            REQUIRE(lookup->friendly_name == info.friendly_name);
        }
    }
}

// ============================================================================
// Test 5: Metadata consistency with operation_patterns.h
// ============================================================================

TEST_CASE("OperationRegistry: consistency with operation_patterns", "[operation_registry][p2]") {
    SECTION("friendly_name matches category_name()") {
        for (const auto& info : OperationRegistry::all()) {
            INFO("Checking: " << info.capability_key);
            REQUIRE(info.friendly_name == category_name(info.category));
        }
    }

    SECTION("capability_key matches category_key()") {
        for (const auto& info : OperationRegistry::all()) {
            INFO("Checking: " << info.capability_key);
            REQUIRE(info.capability_key == category_key(info.category));
        }
    }
}
