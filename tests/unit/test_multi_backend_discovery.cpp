// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../ui_test_utils.h"
#include "printer_discovery.h"

#include "../catch_amalgamated.hpp"

TEST_CASE("PrinterDiscovery: single MMU detected as one system", "[ams][multi-backend]") {
    helix::PrinterDiscovery hw;
    nlohmann::json objects = nlohmann::json::array(
        {"mmu", "mmu_encoder mmu_encoder", "extruder", "heater_bed", "gcode_move"});
    hw.parse_objects(objects);

    const auto& systems = hw.detected_ams_systems();
    REQUIRE(systems.size() == 1);
    REQUIRE(systems[0].type == AmsType::HAPPY_HARE);
    REQUIRE(hw.mmu_type() == AmsType::HAPPY_HARE);
}

TEST_CASE("PrinterDiscovery: toolchanger only detected as one system", "[ams][multi-backend]") {
    helix::PrinterDiscovery hw;
    nlohmann::json objects = nlohmann::json::array(
        {"toolchanger", "tool T0", "tool T1", "extruder", "extruder1", "heater_bed", "gcode_move"});
    hw.parse_objects(objects);

    const auto& systems = hw.detected_ams_systems();
    REQUIRE(systems.size() == 1);
    REQUIRE(systems[0].type == AmsType::TOOL_CHANGER);
}

TEST_CASE("PrinterDiscovery: toolchanger + Happy Hare detected as two systems",
          "[ams][multi-backend]") {
    helix::PrinterDiscovery hw;
    nlohmann::json objects = nlohmann::json::array({"toolchanger", "tool T0", "tool T1", "mmu",
                                                    "mmu_encoder mmu_encoder", "extruder",
                                                    "extruder1", "heater_bed", "gcode_move"});
    hw.parse_objects(objects);

    const auto& systems = hw.detected_ams_systems();
    REQUIRE(systems.size() == 2);

    bool has_tc = false, has_hh = false;
    for (const auto& sys : systems) {
        if (sys.type == AmsType::TOOL_CHANGER)
            has_tc = true;
        if (sys.type == AmsType::HAPPY_HARE)
            has_hh = true;
    }
    REQUIRE(has_tc);
    REQUIRE(has_hh);
    REQUIRE(hw.mmu_type() == AmsType::TOOL_CHANGER);
}

TEST_CASE("PrinterDiscovery: AFC + toolchanger detected as two systems", "[ams][multi-backend]") {
    helix::PrinterDiscovery hw;
    nlohmann::json objects = nlohmann::json::array(
        {"toolchanger", "tool T0", "tool T1", "AFC", "AFC_stepper lane1", "AFC_stepper lane2",
         "extruder", "extruder1", "heater_bed", "gcode_move"});
    hw.parse_objects(objects);

    const auto& systems = hw.detected_ams_systems();
    REQUIRE(systems.size() == 2);

    bool has_tc = false, has_afc = false;
    for (const auto& sys : systems) {
        if (sys.type == AmsType::TOOL_CHANGER)
            has_tc = true;
        if (sys.type == AmsType::AFC)
            has_afc = true;
    }
    REQUIRE(has_tc);
    REQUIRE(has_afc);
}

TEST_CASE("PrinterDiscovery: no AMS detected returns empty", "[ams][multi-backend]") {
    helix::PrinterDiscovery hw;
    nlohmann::json objects = nlohmann::json::array({"extruder", "heater_bed", "gcode_move"});
    hw.parse_objects(objects);

    REQUIRE(hw.detected_ams_systems().empty());
    REQUIRE(hw.mmu_type() == AmsType::NONE);
}
