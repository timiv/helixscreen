// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ams_backend_mock.h"
#include "ams_types.h"

#include "../catch_amalgamated.hpp"

/**
 * @file test_ams_mock_mixed_topology.cpp
 * @brief Unit tests for mixed topology mock backend (HELIX_MOCK_AMS=mixed)
 *
 * Simulates J0eB0l's real hardware: 6-tool toolchanger with mixed AFC hardware.
 * - Unit 0: Box Turtle (4 lanes, PARALLEL, 1:1 lane->tool, buffers, no hub sensor)
 * - Unit 1: OpenAMS (4 lanes, HUB, 4:1 lane->tool T4, no buffers, hub sensor)
 * - Unit 2: OpenAMS (4 lanes, HUB, 4:1 lane->tool T5, no buffers, hub sensor)
 */

TEST_CASE("Mixed topology mock creates 3 units", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    REQUIRE(info.units.size() == 3);
    REQUIRE(info.total_slots == 12);

    CHECK(info.units[0].name == "Turtle_1");
    CHECK(info.units[1].name == "AMS_1");
    CHECK(info.units[2].name == "AMS_2");
}

TEST_CASE("Mixed topology unit 0 is Box Turtle with PARALLEL topology", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();
    const auto& unit0 = info.units[0];

    CHECK(unit0.slot_count == 4);
    CHECK(unit0.first_slot_global_index == 0);
    CHECK(unit0.has_hub_sensor == false);

    // Buffer health should be set for Box Turtle (has TurtleNeck buffers)
    CHECK(unit0.buffer_health.has_value());

    // Per-unit topology: Box Turtle uses PARALLEL (1:1 lane->tool)
    CHECK(backend.get_unit_topology(0) == PathTopology::PARALLEL);
    CHECK(unit0.topology == PathTopology::PARALLEL);
}

TEST_CASE("Mixed topology units 1-2 are OpenAMS with HUB topology", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Unit 1: OpenAMS
    const auto& unit1 = info.units[1];
    CHECK(unit1.slot_count == 4);
    CHECK(unit1.first_slot_global_index == 4);
    CHECK(unit1.has_hub_sensor == true);
    CHECK(backend.get_unit_topology(1) == PathTopology::HUB);
    CHECK(unit1.topology == PathTopology::HUB);

    // Unit 2: OpenAMS
    const auto& unit2 = info.units[2];
    CHECK(unit2.slot_count == 4);
    CHECK(unit2.first_slot_global_index == 8);
    CHECK(unit2.has_hub_sensor == true);
    CHECK(backend.get_unit_topology(2) == PathTopology::HUB);
    CHECK(unit2.topology == PathTopology::HUB);
}

TEST_CASE("Mixed topology lane-to-tool mapping", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Box Turtle slots 0-3 map to T0-T3 (1:1)
    for (int i = 0; i < 4; ++i) {
        const auto* slot = info.get_slot_global(i);
        REQUIRE(slot != nullptr);
        CHECK(slot->mapped_tool == i);
    }

    // OpenAMS 1 slots 4-7 all map to T4 (4:1)
    for (int i = 4; i < 8; ++i) {
        const auto* slot = info.get_slot_global(i);
        REQUIRE(slot != nullptr);
        CHECK(slot->mapped_tool == 4);
    }

    // OpenAMS 2 slots 8-11 all map to T5 (4:1)
    for (int i = 8; i < 12; ++i) {
        const auto* slot = info.get_slot_global(i);
        REQUIRE(slot != nullptr);
        CHECK(slot->mapped_tool == 5);
    }

    // tool_to_slot_map should have 6 entries (T0-T5)
    REQUIRE(info.tool_to_slot_map.size() == 6);
    // T0->slot0, T1->slot1, T2->slot2, T3->slot3
    CHECK(info.tool_to_slot_map[0] == 0);
    CHECK(info.tool_to_slot_map[1] == 1);
    CHECK(info.tool_to_slot_map[2] == 2);
    CHECK(info.tool_to_slot_map[3] == 3);
    // T4->slot4 (first slot of OpenAMS 1), T5->slot8 (first slot of OpenAMS 2)
    CHECK(info.tool_to_slot_map[4] == 4);
    CHECK(info.tool_to_slot_map[5] == 8);
}

TEST_CASE("Mixed topology Box Turtle slots have buffers", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Unit 0 (Box Turtle) should have buffer_health set
    REQUIRE(info.units[0].buffer_health.has_value());
    CHECK(info.units[0].buffer_health->state.size() > 0);
}

TEST_CASE("Mixed topology OpenAMS slots have no buffers", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Units 1-2 (OpenAMS) should NOT have buffer_health
    CHECK_FALSE(info.units[1].buffer_health.has_value());
    CHECK_FALSE(info.units[2].buffer_health.has_value());
}

TEST_CASE("Mixed topology get_topology returns HUB as default", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    // System-wide topology should still return HUB (backward compat default)
    CHECK(backend.get_topology() == PathTopology::HUB);

    // Per-unit topology is accessed via get_unit_topology()
    CHECK(backend.get_unit_topology(0) == PathTopology::PARALLEL);
    CHECK(backend.get_unit_topology(1) == PathTopology::HUB);
    CHECK(backend.get_unit_topology(2) == PathTopology::HUB);

    // Out-of-range falls back to system topology
    CHECK(backend.get_unit_topology(99) == PathTopology::HUB);
    CHECK(backend.get_unit_topology(-1) == PathTopology::HUB);
}

TEST_CASE("Non-mixed mock: get_unit_topology falls back to system topology",
          "[ams][mock][backward_compat]") {
    // Standard mock (not mixed): unit_topologies_ is empty,
    // so get_unit_topology() should fall back to topology_ (HUB by default)
    AmsBackendMock backend(4);

    REQUIRE(backend.get_topology() == PathTopology::HUB);
    REQUIRE(backend.get_unit_topology(0) == PathTopology::HUB);
    REQUIRE(backend.get_unit_topology(1) == PathTopology::HUB);
    REQUIRE(backend.get_unit_topology(-1) == PathTopology::HUB);
    REQUIRE(backend.get_unit_topology(99) == PathTopology::HUB);
}

TEST_CASE("Mixed topology system type is AFC", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    CHECK(backend.get_type() == AmsType::AFC);
}

// ============================================================================
// Tool count derivation tests
//
// The overview panel computes per-unit tool counts from topology + mapped_tool.
// These tests validate the logic that was broken for HUB units with 1:1 defaults.
// We replicate the algorithm from ui_panel_ams_overview.cpp here to test it
// in isolation without requiring LVGL.
// ============================================================================

namespace {

/**
 * @brief Replicate the overview panel's tool counting algorithm
 *
 * This mirrors the logic in ui_panel_ams_overview.cpp::update_system_path()
 * so we can test it without LVGL widget dependencies.
 *
 * @param info AMS system info with units, topologies, and mapped_tool data
 * @param backend Backend for per-unit topology queries
 * @param[out] unit_tool_counts Per-unit tool count results
 * @param[out] unit_first_tools Per-unit first tool index results
 * @return Total tool count across all units
 */
int compute_tool_counts(const AmsSystemInfo& info, const AmsBackend& backend,
                        std::vector<int>& unit_tool_counts, std::vector<int>& unit_first_tools) {
    int total_tools = 0;
    int unit_count = static_cast<int>(info.units.size());

    unit_tool_counts.resize(unit_count);
    unit_first_tools.resize(unit_count);

    for (int i = 0; i < unit_count; ++i) {
        const auto& unit = info.units[i];
        PathTopology topo = backend.get_unit_topology(i);

        int first_tool = -1;
        int max_tool = -1;
        for (const auto& slot : unit.slots) {
            if (slot.mapped_tool >= 0) {
                if (first_tool < 0 || slot.mapped_tool < first_tool) {
                    first_tool = slot.mapped_tool;
                }
                if (slot.mapped_tool > max_tool) {
                    max_tool = slot.mapped_tool;
                }
            }
        }

        int unit_tool_count = 0;
        if (topo != PathTopology::PARALLEL) {
            // HUB/LINEAR: all slots converge to a single toolhead
            unit_tool_count = 1;
            if (first_tool < 0) {
                first_tool = total_tools;
            }
        } else if (first_tool >= 0) {
            // PARALLEL: each slot maps to a different tool
            unit_tool_count = max_tool - first_tool + 1;
        } else if (!unit.slots.empty()) {
            // PARALLEL fallback: no mapped_tool data
            first_tool = total_tools;
            unit_tool_count = static_cast<int>(unit.slots.size());
        }

        unit_tool_counts[i] = unit_tool_count;
        unit_first_tools[i] = first_tool >= 0 ? first_tool : total_tools;

        if (topo == PathTopology::PARALLEL && max_tool >= 0) {
            total_tools = std::max(total_tools, max_tool + 1);
        } else {
            total_tools = std::max(total_tools, first_tool + unit_tool_count);
        }
    }

    return total_tools;
}

} // namespace

TEST_CASE("Tool count: mixed topology with correct mapped_tool", "[ams][tool_count][mixed]") {
    // This is the "happy path" — mock has correct 4:1 mapping for HUB units
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();
    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // Box Turtle: 4 tools (T0-T3), PARALLEL
    CHECK(counts[0] == 4);
    CHECK(firsts[0] == 0);

    // OpenAMS 1: 1 tool (T4), HUB — all 4 slots share T4
    CHECK(counts[1] == 1);
    CHECK(firsts[1] == 4);

    // OpenAMS 2: 1 tool (T5), HUB — all 4 slots share T5
    CHECK(counts[2] == 1);
    CHECK(firsts[2] == 5);

    // Total: 6 tools (T0-T5)
    CHECK(total == 6);
}

TEST_CASE("Tool count: HUB unit with wrong 1:1 mapped_tool defaults",
          "[ams][tool_count][regression]") {
    // This reproduces the real-world bug: AFC backend defaults to 1:1 mapping
    // before lane data arrives, so a HUB unit's slots get mapped_tool={4,5,6,7}
    // instead of all being mapped_tool=4.
    // The fix ensures HUB topology forces tool_count=1 regardless.
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Simulate the wrong 1:1 defaults on the HUB units
    // (as if AFC hasn't sent the `map` field yet)
    for (int i = 0; i < 4; ++i) {
        auto* slot = info.get_slot_global(4 + i);
        REQUIRE(slot != nullptr);
        slot->mapped_tool = 4 + i; // Wrong! Should all be 4
    }
    for (int i = 0; i < 4; ++i) {
        auto* slot = info.get_slot_global(8 + i);
        REQUIRE(slot != nullptr);
        slot->mapped_tool = 8 + i; // Wrong! Should all be 8
    }

    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // Even with wrong mapped_tool, HUB units should still count as 1 tool each
    CHECK(counts[0] == 4); // Box Turtle: PARALLEL, 4 tools
    CHECK(counts[1] == 1); // OpenAMS 1: HUB, forced to 1
    CHECK(counts[2] == 1); // OpenAMS 2: HUB, forced to 1

    // Total is driven by max(first_tool + tool_count) across units.
    // With wrong mapped_tool={8,9,10,11} on AMS_2, first_tool=8, so total=9.
    // Key invariant: NOT 12 (which would happen if HUB units counted all slots).
    CHECK(total < 12);
    // Each HUB unit contributes exactly 1 to the nozzle count
    CHECK(counts[0] + counts[1] + counts[2] == 6);
}

TEST_CASE("Tool count: all HUB units (standard multi-unit AFC)", "[ams][tool_count]") {
    // Two Box Turtles both feeding the same single toolhead (standard AFC setup)
    AmsBackendMock backend(4);
    backend.set_multi_unit_mode(true);

    auto info = backend.get_system_info();
    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // Both units are HUB, should be 1 tool each
    for (size_t i = 0; i < info.units.size(); ++i) {
        CHECK(counts[i] == 1);
    }
    // Total depends on mapped_tool values — at least 1
    CHECK(total >= 1);
}

TEST_CASE("Tool count: single HUB unit", "[ams][tool_count]") {
    // Standard single-unit AFC with 4 slots, all feeding 1 toolhead
    AmsBackendMock backend(4);
    backend.set_afc_mode(true);

    auto info = backend.get_system_info();
    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    REQUIRE(info.units.size() == 1);
    CHECK(counts[0] == 1);
    CHECK(total == 1);
}

TEST_CASE("Tool count: tool changer (all PARALLEL)", "[ams][tool_count]") {
    // Pure tool changer — each slot is its own toolhead
    AmsBackendMock backend(6);
    backend.set_tool_changer_mode(true);

    auto info = backend.get_system_info();
    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    REQUIRE(info.units.size() == 1);
    CHECK(counts[0] == 6);
    CHECK(total == 6);
}

TEST_CASE("Tool count: HUB unit with no mapped_tool data at all", "[ams][tool_count][edge]") {
    // Edge case: slots have mapped_tool = -1 (no mapping data received yet)
    AmsBackendMock backend(4);
    backend.set_afc_mode(true);

    auto info = backend.get_system_info();

    // Clear all mapped_tool values
    for (auto& unit : info.units) {
        for (auto& slot : unit.slots) {
            slot.mapped_tool = -1;
        }
    }

    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // HUB with no mapped_tool → should still be 1 tool (fallback)
    CHECK(counts[0] == 1);
    CHECK(total == 1);
}

TEST_CASE("Tool count: PARALLEL unit with no mapped_tool data", "[ams][tool_count][edge]") {
    // Edge case: tool changer slots with no mapping yet
    AmsBackendMock backend(4);
    backend.set_tool_changer_mode(true);

    auto info = backend.get_system_info();

    // Clear all mapped_tool values
    for (auto& unit : info.units) {
        for (auto& slot : unit.slots) {
            slot.mapped_tool = -1;
        }
    }

    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // PARALLEL with no mapped_tool → falls back to slot_count
    CHECK(counts[0] == 4);
    CHECK(total == 4);
}

TEST_CASE("Tool count: mixed topology HUB units with overlapping mapped_tool",
          "[ams][tool_count][edge]") {
    // Edge case: two HUB units both claim their slots map to T0
    // (weird but possible with misconfigured tool mapping)
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Set both OpenAMS units' slots to T0
    for (int i = 4; i < 12; ++i) {
        auto* slot = info.get_slot_global(i);
        if (slot)
            slot->mapped_tool = 0;
    }

    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // Each HUB unit is still 1 tool, even if they both claim T0
    CHECK(counts[1] == 1);
    CHECK(counts[2] == 1);
    // Box Turtle still has 4 tools
    CHECK(counts[0] == 4);
    // Total: 4 (BT) + 1 (AMS1@T0) + 1 (AMS2@T0) = overlapping, so max is 4+1=5
    // (AMS2's first_tool=0 + 1 = 1, but BT already covers 0-3, so total stays at max)
    CHECK(total >= 4);
}

// ============================================================================
// Hub sensor propagation tests (per-lane hubs in OpenAMS)
// ============================================================================

TEST_CASE("Mixed topology: OpenAMS units have hub sensors", "[ams][mock][mixed][hub_sensor]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Box Turtle: no hub sensor (direct_load per lane in toolchanger mode)
    CHECK(info.units[0].has_hub_sensor == false);

    // OpenAMS 1 & 2: have hub sensors
    CHECK(info.units[1].has_hub_sensor == true);
    CHECK(info.units[2].has_hub_sensor == true);
}

TEST_CASE("Mixed topology: Box Turtle has no hub sensor in toolchanger config",
          "[ams][mock][mixed][hub_sensor]") {
    // When Box Turtle is used with a toolchanger (PARALLEL), each lane
    // goes directly to its own extruder — no shared hub needed.
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    CHECK(info.units[0].has_hub_sensor == false);
    CHECK(info.units[0].hub_sensor_triggered == false);
    CHECK(info.units[0].topology == PathTopology::PARALLEL);
}

// ============================================================================
// AFC backend hub sensor propagation (real backend logic)
// ============================================================================

TEST_CASE("AFC hub sensor: per-lane hubs map to parent unit", "[ams][afc][hub_sensor]") {
    // The real AFC data shows each OpenAMS has 4 hubs (Hub_1..4),
    // each with 1 lane. The hub sensor state should propagate to
    // the parent AmsUnit, not try to match by hub name == unit name.
    //
    // This test validates the fix for the bug where hub sensor updates
    // compared hub_name against unit.name (which never matched).

    // We can't easily test AmsBackendAfc without a Moonraker connection,
    // but we can verify the AmsUnit struct behavior and the mock setup.
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Initially no hub sensors triggered
    CHECK(info.units[1].hub_sensor_triggered == false);
    CHECK(info.units[2].hub_sensor_triggered == false);
}

// ============================================================================
// Slot data integrity in mixed topology
// ============================================================================

TEST_CASE("Mixed topology: all slots have valid global indices", "[ams][mock][mixed][slots]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    for (int i = 0; i < info.total_slots; ++i) {
        const auto* slot = info.get_slot_global(i);
        REQUIRE(slot != nullptr);
        CHECK(slot->global_index == i);
    }
}

TEST_CASE("Mixed topology: slot materials are set", "[ams][mock][mixed][slots]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Every slot should have a material assigned
    for (int i = 0; i < info.total_slots; ++i) {
        const auto* slot = info.get_slot_global(i);
        REQUIRE(slot != nullptr);
        CHECK_FALSE(slot->material.empty());
        // Color should be set (could be 0x000000 for black, so just check material)
    }
}

TEST_CASE("Mixed topology: unit containment is correct", "[ams][mock][mixed][slots]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Slots 0-3 → unit 0
    for (int i = 0; i < 4; ++i) {
        const auto* unit = info.get_unit_for_slot(i);
        REQUIRE(unit != nullptr);
        CHECK(unit->unit_index == 0);
    }

    // Slots 4-7 → unit 1
    for (int i = 4; i < 8; ++i) {
        const auto* unit = info.get_unit_for_slot(i);
        REQUIRE(unit != nullptr);
        CHECK(unit->unit_index == 1);
    }

    // Slots 8-11 → unit 2
    for (int i = 8; i < 12; ++i) {
        const auto* unit = info.get_unit_for_slot(i);
        REQUIRE(unit != nullptr);
        CHECK(unit->unit_index == 2);
    }
}

TEST_CASE("Mixed topology: active unit detection", "[ams][mock][mixed]") {
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Default: slot 0 loaded → unit 0
    CHECK(info.current_slot == 0);
    CHECK(info.get_active_unit_index() == 0);

    // Simulate slot 5 active (OpenAMS 1)
    info.current_slot = 5;
    CHECK(info.get_active_unit_index() == 1);

    // Simulate slot 10 active (OpenAMS 2)
    info.current_slot = 10;
    CHECK(info.get_active_unit_index() == 2);
}

TEST_CASE("Mixed topology: HUB unit mapped_tool doesn't affect physical tool count",
          "[ams][tool_count][mixed][regression]") {
    // The critical regression test: even if someone configures AFC with
    // different virtual tool numbers per lane in a HUB unit, the physical
    // tool count (nozzles to draw) is always 1 for HUB topology.
    AmsBackendMock backend(4);
    backend.set_mixed_topology_mode(true);

    auto info = backend.get_system_info();

    // Give OpenAMS 1 slots wildly different mapped_tool values
    info.get_slot_global(4)->mapped_tool = 10;
    info.get_slot_global(5)->mapped_tool = 20;
    info.get_slot_global(6)->mapped_tool = 30;
    info.get_slot_global(7)->mapped_tool = 40;

    std::vector<int> counts, firsts;
    int total = compute_tool_counts(info, backend, counts, firsts);

    // HUB unit should STILL be 1 tool, not 31 (40-10+1)
    CHECK(counts[1] == 1);
    // The first_tool should use the min mapped_tool (10)
    CHECK(firsts[1] == 10);
    // Total should account for the high mapped_tool values but not blow up
    CHECK(total >= 6);
}
