// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_print_start_collector.cpp
 * @brief Unit tests for PrintStartCollector pattern matching
 *
 * Tests the regex patterns used to detect PRINT_START phases.
 * Includes test cases from real Voron V2 PRINT_START macro output.
 * These tests don't require LVGL or Moonraker - they test pure regex logic.
 */

#include <regex>
#include <string>
#include <vector>

#include "../catch_amalgamated.hpp"

// ============================================================================
// Pattern definitions (replicated from print_start_collector.cpp)
// ============================================================================

// PRINT_START marker pattern
static const std::regex print_start_pattern(R"(PRINT_START|START_PRINT|_PRINT_START)",
                                            std::regex::icase);

// Completion marker (layer 1 detected)
static const std::regex completion_pattern(
    R"(SET_PRINT_STATS_INFO\s+CURRENT_LAYER=|LAYER:?\s*1\b|;LAYER:1|First layer)",
    std::regex::icase);

// Phase detection patterns
// Include both G-code commands AND Voron status_* LED macros (they indicate phase start)
static const std::regex homing_pattern(R"(G28|Homing|Home All Axes|homing|status_homing)",
                                       std::regex::icase);

static const std::regex heating_bed_pattern(
    R"(M190|M140\s+S[1-9]|Heating bed|Heat Bed|BED_TEMP|bed.*heat|status_heating)",
    std::regex::icase);

static const std::regex heating_nozzle_pattern(
    R"(M109|M104\s+S[1-9]|Heating (nozzle|hotend|extruder)|EXTRUDER_TEMP|status_heating)",
    std::regex::icase);

static const std::regex qgl_pattern(R"(QUAD_GANTRY_LEVEL|quad.?gantry.?level|QGL|status_leveling)",
                                    std::regex::icase);

static const std::regex z_tilt_pattern(R"(Z_TILT_ADJUST|z.?tilt.?adjust|status_leveling)",
                                       std::regex::icase);

static const std::regex bed_mesh_pattern(
    R"(BED_MESH_CALIBRATE|BED_MESH_PROFILE\s+LOAD=|Loading bed mesh|mesh.*load|status_meshing)",
    std::regex::icase);

static const std::regex cleaning_pattern(
    R"(CLEAN_NOZZLE|NOZZLE_CLEAN|WIPE_NOZZLE|nozzle.?wipe|clean.?nozzle|status_cleaning)",
    std::regex::icase);

static const std::regex purging_pattern(
    R"(VORON_PURGE|LINE_PURGE|PURGE_LINE|Prime.?Line|Priming|KAMP_.*PURGE|purge.?line)",
    std::regex::icase);

// ============================================================================
// Helper for testing patterns
// ============================================================================

static bool matches(const std::regex& pattern, const std::string& line) {
    return std::regex_search(line, pattern);
}

// ============================================================================
// PRINT_START Marker Tests
// ============================================================================

TEST_CASE("PrintStart: PRINT_START marker detection", "[core][print][marker]") {
    // Should match
    REQUIRE(matches(print_start_pattern, "PRINT_START") == true);
    REQUIRE(matches(print_start_pattern, "START_PRINT") == true);
    REQUIRE(matches(print_start_pattern, "_PRINT_START") == true);
    REQUIRE(matches(print_start_pattern, "print_start") == true); // Case insensitive
    REQUIRE(matches(print_start_pattern, "Calling PRINT_START with args") == true);

    // Real macro invocations
    REQUIRE(matches(print_start_pattern, "START_PRINT BED_TEMP=60 EXTRUDER_TEMP=200") == true);
    REQUIRE(matches(print_start_pattern, "PRINT_START BED=60 EXTRUDER=200 CHAMBER=35") == true);

    // Should NOT match
    REQUIRE(matches(print_start_pattern, "PRINTS_TART") == false);
    REQUIRE(matches(print_start_pattern, "G28") == false);
    REQUIRE(matches(print_start_pattern, "") == false);
}

// ============================================================================
// Completion Marker Tests
// ============================================================================

TEST_CASE("PrintStart: completion marker detection", "[core][print][completion]") {
    // Should match
    REQUIRE(matches(completion_pattern, "SET_PRINT_STATS_INFO CURRENT_LAYER=1") == true);
    REQUIRE(matches(completion_pattern, "LAYER: 1") == true);
    REQUIRE(matches(completion_pattern, "LAYER:1") == true);
    REQUIRE(matches(completion_pattern, ";LAYER:1") == true);
    REQUIRE(matches(completion_pattern, "First layer starting") == true);

    // Should NOT match (not layer 1)
    REQUIRE(matches(completion_pattern, "LAYER: 2") == false);
    REQUIRE(matches(completion_pattern, "LAYER:10") == false);
    REQUIRE(matches(completion_pattern, "LAYER:100") == false);
    REQUIRE(matches(completion_pattern, "SET_PRINT_STATS_INFO") == false); // No CURRENT_LAYER
}

// ============================================================================
// Homing Phase Tests
// ============================================================================

TEST_CASE("PrintStart: homing phase detection", "[core][print][homing]") {
    // Should match
    REQUIRE(matches(homing_pattern, "G28") == true);
    REQUIRE(matches(homing_pattern, "G28 X Y Z") == true);
    REQUIRE(matches(homing_pattern, "G28 Z") == true);
    REQUIRE(matches(homing_pattern, "Homing axes") == true);
    REQUIRE(matches(homing_pattern, "Home All Axes") == true);
    REQUIRE(matches(homing_pattern, "// homing started") == true);

    // Real Voron V2 macro output
    REQUIRE(matches(homing_pattern, "SET_DISPLAY_TEXT MSG=\"Homing\"") == true);

    // Should NOT match
    REQUIRE(matches(homing_pattern, "G29") == false); // Bed leveling
    REQUIRE(matches(homing_pattern, "M104") == false);
}

// ============================================================================
// Heating Phase Tests
// ============================================================================

TEST_CASE("PrintStart: heating bed phase detection", "[core][print][heating]") {
    // Should match
    REQUIRE(matches(heating_bed_pattern, "M190 S60") == true); // Wait for bed
    REQUIRE(matches(heating_bed_pattern, "M140 S60") == true); // Set bed
    REQUIRE(matches(heating_bed_pattern, "Heating bed to 60") == true);
    REQUIRE(matches(heating_bed_pattern, "Heat Bed") == true);
    REQUIRE(matches(heating_bed_pattern, "BED_TEMP=60") == true);
    REQUIRE(matches(heating_bed_pattern, "bed heating") == true);

    // Real Voron V2 macro: M190 S{BED_TEMP}
    REQUIRE(matches(heating_bed_pattern, "M190 S110") == true);

    // Should NOT match
    REQUIRE(matches(heating_bed_pattern, "M140 S0") == false);   // Setting to 0 (cooling)
    REQUIRE(matches(heating_bed_pattern, "M104 S200") == false); // Nozzle temp
}

TEST_CASE("PrintStart: heating nozzle phase detection", "[print][heating]") {
    // Should match
    REQUIRE(matches(heating_nozzle_pattern, "M109 S200") == true); // Wait for nozzle
    REQUIRE(matches(heating_nozzle_pattern, "M104 S200") == true); // Set nozzle
    REQUIRE(matches(heating_nozzle_pattern, "M104 S150") == true); // Mesh temp
    REQUIRE(matches(heating_nozzle_pattern, "Heating nozzle to 200") == true);
    REQUIRE(matches(heating_nozzle_pattern, "Heating hotend") == true);
    REQUIRE(matches(heating_nozzle_pattern, "Heating extruder") == true);
    REQUIRE(matches(heating_nozzle_pattern, "EXTRUDER_TEMP=200") == true);

    // Real Voron V2 macro output
    REQUIRE(matches(heating_nozzle_pattern, "SET_DISPLAY_TEXT MSG=\"Heating for print\"") ==
            false); // "for print" not "nozzle"
    REQUIRE(matches(heating_nozzle_pattern,
                    "SET_DISPLAY_TEXT MSG=\"Heating extruder and bed for probing\"") == true);

    // Should NOT match
    REQUIRE(matches(heating_nozzle_pattern, "M104 S0") == false);  // Cooling
    REQUIRE(matches(heating_nozzle_pattern, "M190 S60") == false); // Bed temp
}

// ============================================================================
// Leveling Phase Tests
// ============================================================================

TEST_CASE("PrintStart: QGL phase detection", "[print][leveling]") {
    // Should match
    REQUIRE(matches(qgl_pattern, "QUAD_GANTRY_LEVEL") == true);
    REQUIRE(matches(qgl_pattern, "quad gantry level") == true);
    REQUIRE(matches(qgl_pattern, "Running QGL") == true);

    // Real Voron V2 macro output
    REQUIRE(matches(qgl_pattern, "SET_DISPLAY_TEXT MSG=\"Leveling gantry\"") ==
            false); // "gantry" alone doesn't match

    // Should NOT match
    REQUIRE(matches(qgl_pattern, "Z_TILT_ADJUST") == false);
    REQUIRE(matches(qgl_pattern, "G28") == false);
}

TEST_CASE("PrintStart: Z_TILT phase detection", "[print][leveling]") {
    // Should match
    REQUIRE(matches(z_tilt_pattern, "Z_TILT_ADJUST") == true);
    REQUIRE(matches(z_tilt_pattern, "z_tilt_adjust") == true);
    REQUIRE(matches(z_tilt_pattern, "z tilt adjust") == true);

    // Should NOT match
    REQUIRE(matches(z_tilt_pattern, "QUAD_GANTRY_LEVEL") == false);
}

// ============================================================================
// Bed Mesh Phase Tests
// ============================================================================

TEST_CASE("PrintStart: bed mesh phase detection", "[print][mesh]") {
    // Should match
    REQUIRE(matches(bed_mesh_pattern, "BED_MESH_CALIBRATE") == true);
    REQUIRE(matches(bed_mesh_pattern, "BED_MESH_PROFILE LOAD=default") == true);
    REQUIRE(matches(bed_mesh_pattern, "Loading bed mesh") == true);
    REQUIRE(matches(bed_mesh_pattern, "mesh loading") == true);

    // Real Voron V2 macro: BED_MESH_CALIBRATE PROFILE=adaptive ADAPTIVE=1
    REQUIRE(matches(bed_mesh_pattern, "BED_MESH_CALIBRATE PROFILE=adaptive ADAPTIVE=1") == true);

    // Should NOT match
    REQUIRE(matches(bed_mesh_pattern, "BED_MESH_CLEAR") == false);
    REQUIRE(matches(bed_mesh_pattern, "SET_DISPLAY_TEXT MSG=\"Performing bed mesh calibration\"") ==
            false);
}

// ============================================================================
// Cleaning Phase Tests
// ============================================================================

TEST_CASE("PrintStart: cleaning phase detection", "[print][cleaning]") {
    // Should match
    REQUIRE(matches(cleaning_pattern, "CLEAN_NOZZLE") == true);
    REQUIRE(matches(cleaning_pattern, "NOZZLE_CLEAN") == true);
    REQUIRE(matches(cleaning_pattern, "WIPE_NOZZLE") == true);
    REQUIRE(matches(cleaning_pattern, "nozzle wipe") == true);
    REQUIRE(matches(cleaning_pattern, "clean nozzle") == true);
    REQUIRE(matches(cleaning_pattern, "clean_nozzle") == true); // Voron V2 macro call

    // Real Voron V2 display text - note: "Cleaning nozzle" has "ing " between,
    // which doesn't match clean.?nozzle pattern (requires 0-1 char between)
    REQUIRE(matches(cleaning_pattern, "SET_DISPLAY_TEXT MSG=\"Cleaning nozzle\"") == false);

    // Should NOT match
    REQUIRE(matches(cleaning_pattern, "PURGE_LINE") == false);
}

// ============================================================================
// Purging Phase Tests
// ============================================================================

TEST_CASE("PrintStart: purging phase detection", "[print][purging]") {
    // Should match
    REQUIRE(matches(purging_pattern, "VORON_PURGE") == true);
    REQUIRE(matches(purging_pattern, "LINE_PURGE") == true);
    REQUIRE(matches(purging_pattern, "PURGE_LINE") == true);
    REQUIRE(matches(purging_pattern, "Prime Line") == true);
    REQUIRE(matches(purging_pattern, "PrimeLine") == true);
    REQUIRE(matches(purging_pattern, "Priming extruder") == true);
    REQUIRE(matches(purging_pattern, "KAMP_ADAPTIVE_PURGE") == true);
    REQUIRE(matches(purging_pattern, "purge line done") == true);

    // Real Voron V2 display text
    REQUIRE(matches(purging_pattern, "SET_DISPLAY_TEXT MSG=\"Purging\"") ==
            false); // Just "Purging" alone

    // Should NOT match
    REQUIRE(matches(purging_pattern, "CLEAN_NOZZLE") == false);
}

// ============================================================================
// Real Voron V2 PRINT_START Macro Tests
// ============================================================================

/**
 * Test against real output from Voron V2 at 192.168.1.112
 * START_PRINT macro includes:
 *   - M104 S{MESH_TEMP}       -> heating nozzle
 *   - M190 S{BED_TEMP}        -> heating bed
 *   - G28                     -> homing
 *   - clean_nozzle            -> cleaning
 *   - QUAD_GANTRY_LEVEL       -> QGL
 *   - G28 Z                   -> homing Z
 *   - BED_MESH_CALIBRATE      -> bed mesh
 *   - M109 S{EXTRUDER_TEMP}   -> heating nozzle (wait)
 *   - VORON_PURGE             -> purging
 */
TEST_CASE("PrintStart: real Voron V2 START_PRINT macro lines", "[print][voron][integration]") {
    // Lines from actual Voron V2 START_PRINT macro
    struct TestCase {
        std::string line;
        const std::regex* expected_pattern;
        const char* description;
    };

    std::vector<TestCase> voron_lines = {
        {"START_PRINT BED_TEMP=110 EXTRUDER_TEMP=250 CHAMBER_TEMP=45", &print_start_pattern,
         "macro invocation"},
        {"M104 S150", &heating_nozzle_pattern, "mesh temp heating"},
        {"M190 S110", &heating_bed_pattern, "bed temp wait"},
        {"G28", &homing_pattern, "home all"},
        {"clean_nozzle", &cleaning_pattern, "nozzle clean macro"},
        {"QUAD_GANTRY_LEVEL", &qgl_pattern, "quad gantry level"},
        {"G28 Z", &homing_pattern, "home Z after QGL"},
        {"BED_MESH_CALIBRATE PROFILE=adaptive ADAPTIVE=1", &bed_mesh_pattern, "adaptive bed mesh"},
        {"M109 S250", &heating_nozzle_pattern, "extruder temp wait"},
        {"VORON_PURGE", &purging_pattern, "voron purge"},
    };

    for (const auto& tc : voron_lines) {
        CAPTURE(tc.description, tc.line);
        REQUIRE(matches(*tc.expected_pattern, tc.line) == true);
    }
}

TEST_CASE("PrintStart: Voron V2 SET_DISPLAY_TEXT messages", "[print][voron]") {
    // These are the display messages from the macro
    REQUIRE(matches(homing_pattern, "SET_DISPLAY_TEXT MSG=\"Homing\"") == true);

    // Note: "Cleaning nozzle" has "ing " between clean and nozzle,
    // so it doesn't match clean.?nozzle pattern (which requires 0-1 char)
    REQUIRE(matches(cleaning_pattern, "SET_DISPLAY_TEXT MSG=\"Cleaning nozzle\"") == false);

    // These DON'T match because they use different wording
    // This is intentional - we match G-code commands, not display text
    REQUIRE(matches(qgl_pattern, "SET_DISPLAY_TEXT MSG=\"Leveling gantry\"") == false);
    REQUIRE(matches(heating_nozzle_pattern, "SET_DISPLAY_TEXT MSG=\"Heating for print\"") == false);
}

// ============================================================================
// Real AD5M Pro START_PRINT Macro Tests
// ============================================================================

/**
 * Test against real output from FlashForge AD5M Pro at 192.168.1.67
 * START_PRINT macro (with mod firmware) includes:
 *   - M140 S{bed_temp}        -> heating bed
 *   - M104 S{extruder_temp}   -> heating nozzle
 *   - G28                     -> homing
 *   - KAMP or _FULL_BED_LEVEL -> bed mesh (adaptive or full)
 *   - BED_MESH_PROFILE LOAD=  -> mesh loading
 *   - LINE_PURGE              -> KAMP purge
 *
 * Notable differences from Voron V2:
 *   - No QGL or Z_TILT (fixed bed CoreXY)
 *   - Uses KAMP for adaptive meshing
 *   - Has CHECK_MD5 verification step
 *   - Uses _PRINT_STATUS S="..." for display
 */
TEST_CASE("PrintStart: real AD5M Pro START_PRINT macro lines", "[print][ad5m][integration]") {
    struct TestCase {
        std::string line;
        const std::regex* expected_pattern;
        const char* description;
    };

    std::vector<TestCase> ad5m_lines = {
        {"START_PRINT BED_TEMP=60 EXTRUDER_TEMP=200", &print_start_pattern, "macro invocation"},
        {"RESPOND MSG=\"START_PRINT\"", &print_start_pattern, "respond with start marker"},
        {"M140 S60", &heating_bed_pattern, "set bed temp"},
        {"M104 S200", &heating_nozzle_pattern, "set nozzle temp"},
        {"G28", &homing_pattern, "home all"},
        {"BED_MESH_CALIBRATE mesh_min=-100,-100 mesh_max=100,100", &bed_mesh_pattern,
         "KAMP mesh calibrate"},
        {"BED_MESH_PROFILE LOAD=auto", &bed_mesh_pattern, "load auto mesh profile"},
        {"LINE_PURGE", &purging_pattern, "KAMP line purge"},
    };

    for (const auto& tc : ad5m_lines) {
        CAPTURE(tc.description, tc.line);
        REQUIRE(matches(*tc.expected_pattern, tc.line) == true);
    }
}

TEST_CASE("PrintStart: AD5M Pro _PRINT_STATUS messages", "[print][ad5m]") {
    // These are unique to AD5M Pro mod firmware
    REQUIRE(matches(homing_pattern, "_PRINT_STATUS S=\"HOMING...\"") == true);

    // Note: These DON'T match because they use different wording (status strings only)
    REQUIRE(matches(heating_bed_pattern, "_PRINT_STATUS S=\"HEATING...\"") == false);
    REQUIRE(matches(bed_mesh_pattern, "_PRINT_STATUS S=\"MESH CHECKING...\"") == false);
}

TEST_CASE("PrintStart: AD5M Pro KAMP-specific patterns", "[print][ad5m][kamp]") {
    // KAMP adaptive purge patterns
    REQUIRE(matches(purging_pattern, "KAMP_ADAPTIVE_PURGE") == true);
    REQUIRE(matches(purging_pattern, "_LINE_PURGE") == true);

    // KAMP bed mesh with parameters
    REQUIRE(matches(bed_mesh_pattern, "BED_MESH_CALIBRATE PROFILE=adaptive ADAPTIVE=1") == true);
    REQUIRE(matches(bed_mesh_pattern, "_KAMP_BED_MESH_CALIBRATE") == true);
}

// ============================================================================
// Noise Rejection Tests
// ============================================================================

// ============================================================================
// Voron Status LED Macro Tests
// ============================================================================

TEST_CASE("PrintStart: Voron status_* LED macros are valid phase indicators",
          "[print][voron][status]") {
    // These LED macros are called at the START of each phase in Voron configs
    REQUIRE(matches(homing_pattern, "status_homing") == true);
    REQUIRE(matches(heating_bed_pattern, "status_heating") == true);
    REQUIRE(matches(heating_nozzle_pattern, "status_heating") == true);
    REQUIRE(matches(qgl_pattern, "status_leveling") == true);
    REQUIRE(matches(z_tilt_pattern, "status_leveling") == true);
    REQUIRE(matches(bed_mesh_pattern, "status_meshing") == true);
    REQUIRE(matches(cleaning_pattern, "status_cleaning") == true);

    // status_printing indicates print started (end of PRINT_START)
    REQUIRE(matches(completion_pattern, "status_printing") == false); // Not a completion marker
}

// ============================================================================
// Noise Rejection Tests
// ============================================================================

TEST_CASE("PrintStart: typical noise lines should not match phases", "[print][negative]") {
    // These are common Klipper output lines that should NOT trigger phase detection
    std::vector<std::string> noise_lines = {
        "ok",
        "// Klipper state: Ready",
        "T:210.5 /210.0 B:60.2 /60.0",
        "echo: Command completed",
        "TOOLHEAD_PARK_MACRO",
        "SET_LED LED=nozzle RED=1",
        "M141 S45", // Chamber temp (not bed or nozzle)
        "AFC_PARK",
        "SMART_PARK",
        "TOOLCHANGE TOOL=0",
        "BED_MESH_CLEAR",
        "SET_AFC_TOOLCHANGES TOOLCHANGES=0",
        "status_printing", // End of PRINT_START, not a phase
        "status_busy",     // Generic status, not a phase
        "status_ready",    // Idle status
    };

    std::vector<const std::regex*> phase_patterns = {
        &homing_pattern, &heating_bed_pattern, &heating_nozzle_pattern, &qgl_pattern,
        &z_tilt_pattern, &bed_mesh_pattern,    &cleaning_pattern,       &purging_pattern};

    for (const auto& line : noise_lines) {
        for (const auto* pattern : phase_patterns) {
            CAPTURE(line);
            REQUIRE(matches(*pattern, line) == false);
        }
    }
}
