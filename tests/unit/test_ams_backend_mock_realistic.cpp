// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ams_backend_mock.h"
#include "runtime_config.h"

#include <chrono>
#include <thread>
#include <vector>

#include "../catch_amalgamated.hpp"

/**
 * @file test_ams_backend_mock_realistic.cpp
 * @brief Unit tests for AMS mock backend realistic mode functionality
 *
 * Tests the multi-phase operation mode where load/unload operations
 * progress through realistic phases (HEATING→LOADING→CHECKING etc.)
 * and integrate with the sim_speedup timing system.
 *
 * Note: These tests set RuntimeConfig::sim_speedup to 1000x so operations
 * complete quickly. Base timing constants are:
 * - HEATING: 3000ms -> 3ms at 1000x
 * - FORMING_TIP: 4000ms -> 4ms at 1000x
 * - CHECKING: 1500ms -> 1.5ms at 1000x
 * - SEGMENT_ANIMATION: 5000ms -> 5ms at 1000x
 */

// RAII helper to set up fast timing and restore on exit
class FastTimingScope {
  public:
    FastTimingScope() {
        auto* config = get_mutable_runtime_config();
        original_speedup_ = config->sim_speedup;
        config->sim_speedup = 1000.0; // 1000x speedup for fast tests
    }
    ~FastTimingScope() {
        auto* config = get_mutable_runtime_config();
        config->sim_speedup = original_speedup_;
    }

  private:
    double original_speedup_ = 1.0;
};

// Helper to set up fast timing for realistic mode tests
static void setup_fast_timing() {
    auto* config = get_mutable_runtime_config();
    config->sim_speedup = 1000.0; // 1000x speedup for fast tests
}

// Helper to restore timing after tests
static void restore_timing() {
    auto* config = get_mutable_runtime_config();
    config->sim_speedup = 1.0;
}

TEST_CASE("AmsBackendMock realistic mode defaults", "[ams][mock][realistic]") {
    AmsBackendMock backend(4);

    SECTION("realistic mode is disabled by default") {
        REQUIRE_FALSE(backend.is_realistic_mode());
    }

    SECTION("can enable realistic mode") {
        backend.set_realistic_mode(true);
        REQUIRE(backend.is_realistic_mode());
    }

    SECTION("can disable realistic mode") {
        backend.set_realistic_mode(true);
        REQUIRE(backend.is_realistic_mode());
        backend.set_realistic_mode(false);
        REQUIRE_FALSE(backend.is_realistic_mode());
    }
}

TEST_CASE("AmsBackendMock realistic mode load operation phases", "[ams][mock][realistic][load]") {
    FastTimingScope timing_guard; // RAII: 1000x speedup, auto-restored

    AmsBackendMock backend(4);
    backend.set_operation_delay(10); // Very fast for testing
    backend.set_realistic_mode(true);
    REQUIRE(backend.start());

    // Track action state changes
    std::vector<AmsAction> observed_actions;
    backend.set_event_callback([&](const std::string& event, const std::string&) {
        if (event == AmsBackend::EVENT_STATE_CHANGED) {
            auto action = backend.get_current_action();
            // Only record unique consecutive actions
            if (observed_actions.empty() || observed_actions.back() != action) {
                observed_actions.push_back(action);
            }
        }
    });

    SECTION("load shows HEATING then LOADING then CHECKING sequence") {
        // Start with slot 1 (slot 0 is pre-loaded in mock)
        auto result = backend.unload_filament();
        REQUIRE(result);

        // Wait for unload to complete (with 1000x speedup: ~20ms total)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        observed_actions.clear();

        // Now do load
        result = backend.load_filament(1);
        REQUIRE(result);

        // Wait for operation to complete (with 1000x speedup: ~12ms total)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Verify phase sequence
        REQUIRE(observed_actions.size() >= 3);

        // Find the HEATING -> LOADING -> CHECKING sequence
        bool found_heating = false;
        bool found_loading_after_heating = false;
        bool found_checking_after_loading = false;

        for (size_t i = 0; i < observed_actions.size(); ++i) {
            if (observed_actions[i] == AmsAction::HEATING) {
                found_heating = true;
            }
            if (found_heating && observed_actions[i] == AmsAction::LOADING) {
                found_loading_after_heating = true;
            }
            if (found_loading_after_heating && observed_actions[i] == AmsAction::CHECKING) {
                found_checking_after_loading = true;
            }
        }

        CHECK(found_heating);
        CHECK(found_loading_after_heating);
        CHECK(found_checking_after_loading);
    }

    backend.stop();
}

TEST_CASE("AmsBackendMock realistic mode unload operation phases",
          "[ams][mock][realistic][unload]") {
    FastTimingScope timing_guard; // RAII: 1000x speedup, auto-restored

    AmsBackendMock backend(4);
    backend.set_operation_delay(10); // Very fast for testing
    backend.set_realistic_mode(true);
    REQUIRE(backend.start());

    // Track action state changes
    std::vector<AmsAction> observed_actions;
    backend.set_event_callback([&](const std::string& event, const std::string&) {
        if (event == AmsBackend::EVENT_STATE_CHANGED) {
            auto action = backend.get_current_action();
            if (observed_actions.empty() || observed_actions.back() != action) {
                observed_actions.push_back(action);
            }
        }
    });

    SECTION("unload shows HEATING then FORMING_TIP then UNLOADING sequence") {
        // Slot 0 is pre-loaded, so we can unload directly
        auto result = backend.unload_filament();
        REQUIRE(result);

        // Wait for operation to complete (with 1000x speedup: ~15ms total)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Verify phase sequence
        REQUIRE(observed_actions.size() >= 3);

        // Find the HEATING -> FORMING_TIP -> UNLOADING sequence
        bool found_heating = false;
        bool found_forming_tip = false;
        bool found_unloading = false;

        for (size_t i = 0; i < observed_actions.size(); ++i) {
            if (observed_actions[i] == AmsAction::HEATING) {
                found_heating = true;
            }
            if (found_heating && observed_actions[i] == AmsAction::FORMING_TIP) {
                found_forming_tip = true;
            }
            if (found_forming_tip && observed_actions[i] == AmsAction::UNLOADING) {
                found_unloading = true;
            }
        }

        CHECK(found_heating);
        CHECK(found_forming_tip);
        CHECK(found_unloading);
    }

    backend.stop();
}

TEST_CASE("AmsBackendMock simple mode skips extra phases", "[ams][mock][realistic][simple]") {
    FastTimingScope timing_guard; // RAII: 1000x speedup, auto-restored

    AmsBackendMock backend(4);
    backend.set_operation_delay(10); // Very fast for testing
    // Realistic mode is OFF by default
    REQUIRE_FALSE(backend.is_realistic_mode());
    REQUIRE(backend.start());

    // Track action state changes
    std::vector<AmsAction> observed_actions;
    backend.set_event_callback([&](const std::string& event, const std::string&) {
        if (event == AmsBackend::EVENT_STATE_CHANGED) {
            auto action = backend.get_current_action();
            if (observed_actions.empty() || observed_actions.back() != action) {
                observed_actions.push_back(action);
            }
        }
    });

    SECTION("unload in simple mode shows only UNLOADING") {
        auto result = backend.unload_filament();
        REQUIRE(result);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Should NOT see HEATING or FORMING_TIP in simple mode
        bool found_heating = false;
        bool found_forming_tip = false;
        bool found_unloading = false;

        for (const auto& action : observed_actions) {
            if (action == AmsAction::HEATING)
                found_heating = true;
            if (action == AmsAction::FORMING_TIP)
                found_forming_tip = true;
            if (action == AmsAction::UNLOADING)
                found_unloading = true;
        }

        CHECK_FALSE(found_heating);
        CHECK_FALSE(found_forming_tip);
        CHECK(found_unloading);
    }

    backend.stop();
}

TEST_CASE("AmsBackendMock realistic mode completes to IDLE", "[ams][mock][realistic][completion]") {
    FastTimingScope timing_guard; // RAII: 1000x speedup, auto-restored

    AmsBackendMock backend(4);
    backend.set_operation_delay(10);
    backend.set_realistic_mode(true);
    REQUIRE(backend.start());

    SECTION("load completes to IDLE state") {
        // Unload first
        backend.unload_filament();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Load
        backend.load_filament(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto action = backend.get_current_action();
        REQUIRE(action == AmsAction::IDLE);

        auto info = backend.get_system_info();
        REQUIRE(info.filament_loaded == true);
        REQUIRE(info.current_slot == 1);
    }

    SECTION("unload completes to IDLE state") {
        backend.unload_filament();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto action = backend.get_current_action();
        REQUIRE(action == AmsAction::IDLE);

        auto info = backend.get_system_info();
        REQUIRE(info.filament_loaded == false);
        REQUIRE(info.current_slot == -1);
    }

    backend.stop();
}

TEST_CASE("AmsBackendMock realistic mode can be cancelled", "[ams][mock][realistic][cancel]") {
    FastTimingScope timing_guard; // RAII: 1000x speedup, auto-restored

    AmsBackendMock backend(4);
    backend.set_operation_delay(100); // Slower to give time to cancel
    backend.set_realistic_mode(true);
    REQUIRE(backend.start());

    SECTION("cancel during heating phase") {
        backend.unload_filament();

        // Give it a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Cancel mid-operation
        auto cancel_result = backend.cancel();
        REQUIRE(cancel_result);

        auto action = backend.get_current_action();
        REQUIRE(action == AmsAction::IDLE);
    }

    backend.stop();
}
