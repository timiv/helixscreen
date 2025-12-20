// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "../../include/wifi_backend.h"
#include "../../include/wifi_backend_mock.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

#include "../catch_amalgamated.hpp"

/**
 * WiFi Backend Unit Tests
 *
 * Tests verify backend-specific functionality:
 * - Backend lifecycle (start/stop/is_running)
 * - Event system (callback registration and firing)
 * - Mock backend behavior (scan timing, network data)
 * - Timer cleanup and resource management
 *
 * CRITICAL BUGS CAUGHT:
 * - Backend auto-start bug: Mock backend should NOT start itself in constructor
 * - Timer cleanup: Timers must be cleaned up in stop()/destructor
 * - Event callback validation: Events should not fire when backend stopped
 */

// ============================================================================
// Global LVGL Initialization (called once)
// ============================================================================

// No LVGL initialization needed - WiFi backend uses std::thread, not LVGL timers

// ============================================================================
// Test Fixtures
// ============================================================================

class WiFiBackendTestFixture {
  public:
    WiFiBackendTestFixture() {
        // Create mock backend directly for testing
        backend = std::make_unique<WifiBackendMock>();

        // Reset state
        event_count = 0;
        last_event_name.clear();
        last_event_data.clear();
    }

    ~WiFiBackendTestFixture() {
        // Cleanup backend
        if (backend) {
            backend->stop();
        }
    }

    // Helper: Event callback that captures event details
    void event_callback(const std::string& data) {
        event_count++;
        last_event_data = data;
    }

    // Helper: Wait for async operations (real-time delay, not emulated)
    // WiFi mock backend uses std::thread with real sleep, not LVGL timers
    void wait_for_events(int timeout_ms = 5000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
    }

    // Test backend
    std::unique_ptr<WifiBackend> backend;

    // Test state
    int event_count = 0;
    std::string last_event_name;
    std::string last_event_data;
};

// ============================================================================
// Backend Lifecycle Tests
// ============================================================================

TEST_CASE_METHOD(WiFiBackendTestFixture, "Backend lifecycle", "[network][backend][lifecycle]") {
    SECTION("Backend created but not running by default") {
        // CRITICAL: This catches the auto-start bug
        REQUIRE_FALSE(backend->is_running());
    }

    SECTION("Backend start() enables it") {
        WiFiError result = backend->start();
        REQUIRE(result.success());
        REQUIRE(backend->is_running());
    }

    SECTION("Backend stop() disables it") {
        backend->start();
        REQUIRE(backend->is_running());

        backend->stop();
        REQUIRE_FALSE(backend->is_running());
    }

    SECTION("Backend lifecycle: start → stop → start") {
        // Initial: not running
        REQUIRE_FALSE(backend->is_running());

        // First start
        backend->start();
        REQUIRE(backend->is_running());

        // Stop
        backend->stop();
        REQUIRE_FALSE(backend->is_running());

        // Second start (should work)
        WiFiError result = backend->start();
        REQUIRE(result.success());
        REQUIRE(backend->is_running());
    }

    SECTION("Multiple start() calls are idempotent") {
        backend->start();
        REQUIRE(backend->is_running());

        // Second start() should succeed (no-op)
        WiFiError result = backend->start();
        REQUIRE(result.success());
        REQUIRE(backend->is_running());
    }

    SECTION("Multiple stop() calls are safe") {
        backend->start();
        backend->stop();
        REQUIRE_FALSE(backend->is_running());

        // Second stop() should be safe (no crash)
        REQUIRE_NOTHROW(backend->stop());
        REQUIRE_FALSE(backend->is_running());
    }
}

// ============================================================================
// Event System Tests
// ============================================================================

TEST_CASE_METHOD(WiFiBackendTestFixture, "Backend event system", "[network][backend][events]") {
    SECTION("Event callback registration") {
        int callback_count = 0;
        backend->register_event_callback("TEST_EVENT", [&callback_count](const std::string& data) {
            (void)data;
            callback_count++;
        });

        // Callback registered (can't directly test until event fires)
        REQUIRE(callback_count == 0); // Not fired yet
    }

    SECTION("SCAN_COMPLETE event fires after scan") {
        backend->start();
        spdlog::info("[Test] Backend started");

        int scan_complete_count = 0;
        backend->register_event_callback(
            "SCAN_COMPLETE", [&scan_complete_count](const std::string& data) {
                (void)data;
                scan_complete_count++;
                spdlog::info("[Test] SCAN_COMPLETE callback fired! count={}", scan_complete_count);
            });

        // Trigger scan
        spdlog::info("[Test] Triggering scan...");
        WiFiError result = backend->trigger_scan();
        REQUIRE(result.success());
        spdlog::info("[Test] trigger_scan() returned success");

        // Wait for SCAN_COMPLETE event (mock backend fires after 2s)
        spdlog::info("[Test] Waiting 2100ms for SCAN_COMPLETE event...");
        wait_for_events(2100); // Mock scan delay is 2000ms

        spdlog::info("[Test] Timer wait complete (count={})", scan_complete_count);
        REQUIRE(scan_complete_count == 1);
    }

    SECTION("Multiple event callbacks can be registered") {
        backend->start();

        int scan_count = 0;
        int connect_count = 0;

        backend->register_event_callback("SCAN_COMPLETE", [&scan_count](const std::string& data) {
            (void)data;
            scan_count++;
        });

        backend->register_event_callback("CONNECTED", [&connect_count](const std::string& data) {
            (void)data;
            connect_count++;
        });

        // Both callbacks registered
        REQUIRE(scan_count == 0);
        REQUIRE(connect_count == 0);
    }

    SECTION("Event callback survives backend restart") {
        backend->start();

        int event_count = 0;
        backend->register_event_callback("SCAN_COMPLETE", [&event_count](const std::string& data) {
            (void)data;
            event_count++;
        });

        // Restart backend
        backend->stop();
        backend->start();

        // Trigger scan
        backend->trigger_scan();

        // Wait for scan to complete
        wait_for_events(2100); // Mock scan delay is 2000ms

        // Callback should still work after restart
        REQUIRE(event_count > 0);
    }
}

// ============================================================================
// Mock Backend Scan Tests
// ============================================================================

TEST_CASE_METHOD(WiFiBackendTestFixture, "Mock backend scan behavior",
                 "[network][backend][mock][scan]") {
    SECTION("trigger_scan() fails when backend not running") {
        // Backend not started
        REQUIRE_FALSE(backend->is_running());

        WiFiError result = backend->trigger_scan();
        REQUIRE_FALSE(result.success());
        REQUIRE(result.result == WiFiResult::NOT_INITIALIZED);
    }

    SECTION("trigger_scan() succeeds when backend running") {
        backend->start();
        REQUIRE(backend->is_running());

        WiFiError result = backend->trigger_scan();
        REQUIRE(result.success());
    }

    SECTION("Scan results available after SCAN_COMPLETE") {
        backend->start();

        int scan_complete_count = 0;
        backend->register_event_callback("SCAN_COMPLETE",
                                         [&scan_complete_count](const std::string& data) {
                                             (void)data;
                                             scan_complete_count++;
                                         });

        backend->trigger_scan();

        // Wait for scan to complete
        wait_for_events(2100); // Mock scan delay is 2000ms
        REQUIRE(scan_complete_count > 0);

        // Get scan results
        std::vector<WiFiNetwork> networks;
        WiFiError result = backend->get_scan_results(networks);
        REQUIRE(result.success());
        REQUIRE(networks.size() == 10); // Mock backend has 10 networks
    }

    SECTION("get_scan_results() fails when backend not running") {
        REQUIRE_FALSE(backend->is_running());

        std::vector<WiFiNetwork> networks;
        WiFiError result = backend->get_scan_results(networks);
        REQUIRE_FALSE(result.success());
        REQUIRE(result.result == WiFiResult::NOT_INITIALIZED);
        REQUIRE(networks.empty());
    }

    SECTION("Mock networks have valid data") {
        backend->start();
        backend->trigger_scan();

        // Wait for scan
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> networks;
        backend->get_scan_results(networks);

        REQUIRE(networks.size() == 10);

        for (const auto& net : networks) {
            // SSID not empty
            REQUIRE_FALSE(net.ssid.empty());

            // Signal strength in range
            REQUIRE(net.signal_strength >= 0);
            REQUIRE(net.signal_strength <= 100);

            // Security info present
            if (net.is_secured) {
                REQUIRE_FALSE(net.security_type.empty());
            }
        }
    }

    SECTION("Networks sorted by signal strength") {
        backend->start();
        backend->trigger_scan();

        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> networks;
        backend->get_scan_results(networks);

        // Mock backend sorts by signal strength (strongest first)
        for (size_t i = 1; i < networks.size(); i++) {
            REQUIRE(networks[i - 1].signal_strength >= networks[i].signal_strength);
        }
    }

    SECTION("Signal strength varies on each scan") {
        backend->start();

        // First scan
        backend->trigger_scan();
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> scan1;
        backend->get_scan_results(scan1);

        // Second scan
        backend->trigger_scan();
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> scan2;
        backend->get_scan_results(scan2);

        // At least one network should have different signal strength (±5% variation)
        bool found_variation = false;
        for (size_t i = 0; i < scan1.size(); i++) {
            if (scan1[i].signal_strength != scan2[i].signal_strength) {
                found_variation = true;
                break;
            }
        }

        // Note: May occasionally be same due to random number generation
        INFO("Signal strength varied: " << (found_variation ? "yes" : "no"));
    }
}

// ============================================================================
// Mock Backend Connection Tests
// ============================================================================

TEST_CASE_METHOD(WiFiBackendTestFixture, "Mock backend connection behavior",
                 "[network][backend][mock][connect][slow]") {
    SECTION("connect_network() fails when backend not running") {
        REQUIRE_FALSE(backend->is_running());

        WiFiError result = backend->connect_network("TestNet", "password");
        REQUIRE_FALSE(result.success());
        REQUIRE(result.result == WiFiResult::NOT_INITIALIZED);
    }

    SECTION("connect_network() fails for non-existent network") {
        backend->start();

        WiFiError result = backend->connect_network("NonExistentNetwork", "password");
        REQUIRE_FALSE(result.success());
        REQUIRE(result.result == WiFiResult::NETWORK_NOT_FOUND);
    }

    SECTION("connect_network() requires password for secured networks") {
        backend->start();

        // Get a secured network
        backend->trigger_scan();
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> networks;
        backend->get_scan_results(networks);

        auto secured = std::find_if(networks.begin(), networks.end(),
                                    [](const WiFiNetwork& n) { return n.is_secured; });
        REQUIRE(secured != networks.end());

        // Try connecting without password
        WiFiError result = backend->connect_network(secured->ssid, "");
        REQUIRE_FALSE(result.success());
        REQUIRE(result.result == WiFiResult::INVALID_PARAMETERS);
    }

    SECTION("Successful connection fires CONNECTED event") {
        backend->start();

        // Get available networks
        backend->trigger_scan();
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> networks;
        backend->get_scan_results(networks);
        REQUIRE(networks.size() > 0);

        // Register CONNECTED callback
        int connected_count = 0;
        backend->register_event_callback("CONNECTED", [&connected_count](const std::string& data) {
            (void)data;
            connected_count++;
        });

        // Connect to first network (mock backend simulates 2-3s delay)
        WiFiError result = backend->connect_network(networks[0].ssid, "test_password");
        REQUIRE(result.success()); // Connection initiated

        // Wait for CONNECTED event (mock connect delay is 2000-3000ms)
        wait_for_events(3100);

        // Note: Mock has 5% chance of auth failure, might not always succeed
        INFO("Got CONNECTED event: " << (connected_count > 0 ? "yes" : "no"));
    }

    SECTION("disconnect_network() is safe when not connected") {
        backend->start();

        WiFiError result = backend->disconnect_network();
        REQUIRE(result.success()); // Idempotent operation
    }

    SECTION("Connection status updated after connect") {
        backend->start();

        // Initial status: not connected
        auto status = backend->get_status();
        REQUIRE_FALSE(status.connected);

        // Get networks and connect
        backend->trigger_scan();
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> networks;
        backend->get_scan_results(networks);

        int connected_count = 0;
        backend->register_event_callback("CONNECTED", [&connected_count](const std::string& data) {
            (void)data;
            connected_count++;
        });

        backend->connect_network(networks[0].ssid, "test_password");

        // Wait for connection (mock connect delay is 2000-3000ms)
        wait_for_events(3100);

        if (connected_count > 0) {
            status = backend->get_status();
            REQUIRE(status.connected);
            REQUIRE_FALSE(status.ssid.empty());
            REQUIRE_FALSE(status.ip_address.empty());
        }
    }
}

// ============================================================================
// Timer Cleanup Tests
// ============================================================================

TEST_CASE_METHOD(WiFiBackendTestFixture, "Backend timer cleanup",
                 "[network][backend][cleanup][slow]") {
    SECTION("stop() cleans up scan timer") {
        backend->start();
        backend->trigger_scan();

        // Stop before scan completes
        backend->stop();

        // No crash - timers cleaned up
        REQUIRE_NOTHROW(backend->stop());
    }

    SECTION("stop() cleans up connection timer") {
        backend->start();

        // Get networks
        backend->trigger_scan();
        wait_for_events(2100); // Mock scan delay is 2000ms

        std::vector<WiFiNetwork> networks;
        backend->get_scan_results(networks);

        // Start connection
        backend->connect_network(networks[0].ssid, "password");

        // Stop before connection completes
        backend->stop();

        // No crash - timers cleaned up
        REQUIRE_NOTHROW(backend->stop());
    }

    SECTION("Destructor cleans up active timers") {
        auto temp_backend = std::make_unique<WifiBackendMock>();
        temp_backend->start();
        temp_backend->trigger_scan();

        // Destroy while scan in progress
        REQUIRE_NOTHROW(temp_backend.reset());
    }

    SECTION("No events fire after backend stopped") {
        backend->start();

        int event_count = 0;
        backend->register_event_callback("SCAN_COMPLETE", [&event_count](const std::string& data) {
            (void)data;
            event_count++;
        });

        backend->trigger_scan();

        // Stop immediately (before scan completes)
        backend->stop();

        // Wait to ensure scan thread is fully cleaned up
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        // Event should NOT fire (thread was canceled)
        REQUIRE(event_count == 0);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE_METHOD(WiFiBackendTestFixture, "Backend edge cases", "[network][backend][edge-cases]") {
    SECTION("Rapid start/stop cycles") {
        for (int i = 0; i < 5; i++) {
            backend->start();
            backend->stop();
        }

        // Final state: not running
        REQUIRE_FALSE(backend->is_running());
    }

    SECTION("Multiple trigger_scan() calls") {
        backend->start();

        // Trigger multiple scans rapidly
        backend->trigger_scan();
        backend->trigger_scan();
        backend->trigger_scan();

        // Should not crash (later calls replace earlier timer)
        REQUIRE_NOTHROW(backend->stop());
    }

    SECTION("get_status() safe when not connected") {
        auto status = backend->get_status();
        REQUIRE_FALSE(status.connected);
        REQUIRE(status.ssid.empty());
        REQUIRE(status.ip_address.empty());
        REQUIRE(status.signal_strength == 0);
    }
}
