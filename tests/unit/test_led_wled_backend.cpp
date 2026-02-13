// SPDX-License-Identifier: GPL-3.0-or-later

#include "led/led_controller.h"

#include "../catch_amalgamated.hpp"

using namespace helix::led;

TEST_CASE("WledBackend: set_on with null API calls error callback", "[led][wled]") {
    WledBackend backend;

    bool error_called = false;
    backend.set_on("test_strip", nullptr, [&](const std::string& err) {
        error_called = true;
        REQUIRE(err.find("no API") != std::string::npos);
    });
    REQUIRE(error_called);
}

TEST_CASE("WledBackend: set_off with null API calls error callback", "[led][wled]") {
    WledBackend backend;

    bool error_called = false;
    backend.set_off("test_strip", nullptr, [&](const std::string& err) {
        error_called = true;
        REQUIRE(err.find("no API") != std::string::npos);
    });
    REQUIRE(error_called);
}

TEST_CASE("WledBackend: set_brightness with null API calls error callback", "[led][wled]") {
    WledBackend backend;

    bool error_called = false;
    backend.set_brightness("test_strip", 50, nullptr, [&](const std::string& err) {
        error_called = true;
        REQUIRE(err.find("no API") != std::string::npos);
    });
    REQUIRE(error_called);
}

TEST_CASE("WledBackend: set_preset with null API calls error callback", "[led][wled]") {
    WledBackend backend;

    bool error_called = false;
    backend.set_preset("test_strip", 1, nullptr, [&](const std::string& err) {
        error_called = true;
        REQUIRE(err.find("no API") != std::string::npos);
    });
    REQUIRE(error_called);
}

TEST_CASE("WledBackend: toggle with null API calls error callback", "[led][wled]") {
    WledBackend backend;

    bool error_called = false;
    backend.toggle("test_strip", nullptr, [&](const std::string& err) {
        error_called = true;
        REQUIRE(err.find("no API") != std::string::npos);
    });
    REQUIRE(error_called);
}

TEST_CASE("WledBackend: null callbacks don't crash with null API", "[led][wled]") {
    WledBackend backend;
    // All callbacks null, api_ null -- should not crash
    backend.set_on("test", nullptr, nullptr);
    backend.set_off("test", nullptr, nullptr);
    backend.set_brightness("test", 50, nullptr, nullptr);
    backend.set_preset("test", 1, nullptr, nullptr);
    backend.toggle("test", nullptr, nullptr);
}

TEST_CASE("WledBackend: type is WLED", "[led][wled]") {
    WledBackend backend;
    REQUIRE(backend.type() == LedBackendType::WLED);
}

TEST_CASE("WledBackend: strips are discoverable with correct backend type", "[led][wled]") {
    WledBackend backend;

    LedStripInfo strip;
    strip.name = "Printer LED";
    strip.id = "printer_led";
    strip.backend = LedBackendType::WLED;
    strip.supports_color = true;
    strip.supports_white = true;
    backend.add_strip(strip);

    REQUIRE(backend.strips()[0].backend == LedBackendType::WLED);
    REQUIRE(backend.strips()[0].id == "printer_led");
}

TEST_CASE("WledBackend: multiple strip discovery", "[led][wled]") {
    WledBackend backend;

    LedStripInfo s1;
    s1.name = "Printer";
    s1.id = "printer_led";
    s1.backend = LedBackendType::WLED;
    s1.supports_color = true;
    s1.supports_white = true;
    backend.add_strip(s1);

    LedStripInfo s2;
    s2.name = "Enclosure";
    s2.id = "enclosure_led";
    s2.backend = LedBackendType::WLED;
    s2.supports_color = true;
    s2.supports_white = false;
    backend.add_strip(s2);

    REQUIRE(backend.strips().size() == 2);
    REQUIRE(backend.strips()[0].id == "printer_led");
    REQUIRE(backend.strips()[1].id == "enclosure_led");
}

TEST_CASE("WledBackend: strip management", "[led][wled]") {
    WledBackend backend;

    REQUIRE(!backend.is_available());
    REQUIRE(backend.strips().empty());

    LedStripInfo strip;
    strip.name = "WLED Strip";
    strip.id = "wled_living_room";
    strip.backend = LedBackendType::WLED;
    strip.supports_color = true;
    strip.supports_white = false;

    backend.add_strip(strip);
    REQUIRE(backend.is_available());
    REQUIRE(backend.strips().size() == 1);
    REQUIRE(backend.strips()[0].name == "WLED Strip");
    REQUIRE(backend.strips()[0].id == "wled_living_room");

    // Add a second strip
    LedStripInfo strip2;
    strip2.name = "Bedroom LEDs";
    strip2.id = "wled_bedroom";
    strip2.backend = LedBackendType::WLED;
    strip2.supports_color = true;
    strip2.supports_white = true;

    backend.add_strip(strip2);
    REQUIRE(backend.strips().size() == 2);

    backend.clear();
    REQUIRE(!backend.is_available());
    REQUIRE(backend.strips().empty());
}

// ============================================================================
// Strip State Management
// ============================================================================

TEST_CASE("WledBackend: default strip state is off with full brightness", "[led][wled]") {
    WledBackend backend;
    auto state = backend.get_strip_state("unknown_strip");
    REQUIRE(!state.is_on);
    REQUIRE(state.brightness == 255);
    REQUIRE(state.active_preset == -1);
}

TEST_CASE("WledBackend: update and get strip state", "[led][wled]") {
    WledBackend backend;

    WledStripState new_state;
    new_state.is_on = true;
    new_state.brightness = 128;
    new_state.active_preset = 3;
    backend.update_strip_state("test_led", new_state);

    auto state = backend.get_strip_state("test_led");
    REQUIRE(state.is_on);
    REQUIRE(state.brightness == 128);
    REQUIRE(state.active_preset == 3);
}

TEST_CASE("WledBackend: clear resets strip states", "[led][wled]") {
    WledBackend backend;

    WledStripState new_state{true, 100, 2};
    backend.update_strip_state("test_led", new_state);
    backend.clear();

    auto state = backend.get_strip_state("test_led");
    REQUIRE(!state.is_on);
    REQUIRE(state.active_preset == -1);
}

TEST_CASE("WledBackend: multiple strip states are independent", "[led][wled]") {
    WledBackend backend;

    backend.update_strip_state("strip_a", {true, 200, 1});
    backend.update_strip_state("strip_b", {false, 50, 5});

    auto a = backend.get_strip_state("strip_a");
    auto b = backend.get_strip_state("strip_b");

    REQUIRE(a.is_on);
    REQUIRE(a.brightness == 200);
    REQUIRE(a.active_preset == 1);

    REQUIRE(!b.is_on);
    REQUIRE(b.brightness == 50);
    REQUIRE(b.active_preset == 5);
}

// ============================================================================
// Strip Address Management
// ============================================================================

TEST_CASE("WledBackend: set and get strip address", "[led][wled]") {
    WledBackend backend;

    backend.set_strip_address("printer_led", "192.168.1.50");
    REQUIRE(backend.get_strip_address("printer_led") == "192.168.1.50");
}

TEST_CASE("WledBackend: unknown strip returns empty address", "[led][wled]") {
    WledBackend backend;
    REQUIRE(backend.get_strip_address("nonexistent").empty());
}

TEST_CASE("WledBackend: clear removes addresses", "[led][wled]") {
    WledBackend backend;
    backend.set_strip_address("printer_led", "192.168.1.50");
    backend.clear();
    REQUIRE(backend.get_strip_address("printer_led").empty());
}

TEST_CASE("WledBackend: overwrite strip address", "[led][wled]") {
    WledBackend backend;
    backend.set_strip_address("printer_led", "192.168.1.50");
    backend.set_strip_address("printer_led", "10.0.0.100");
    REQUIRE(backend.get_strip_address("printer_led") == "10.0.0.100");
}

// ============================================================================
// Preset Management
// ============================================================================

TEST_CASE("WledBackend: set and get presets", "[led][wled]") {
    WledBackend backend;

    std::vector<WledPresetInfo> presets = {{1, "Warm White"}, {2, "Rainbow"}, {3, "Fire"}};
    backend.set_strip_presets("printer_led", presets);

    const auto& result = backend.get_strip_presets("printer_led");
    REQUIRE(result.size() == 3);
    REQUIRE(result[0].id == 1);
    REQUIRE(result[0].name == "Warm White");
    REQUIRE(result[1].id == 2);
    REQUIRE(result[1].name == "Rainbow");
    REQUIRE(result[2].id == 3);
    REQUIRE(result[2].name == "Fire");
}

TEST_CASE("WledBackend: unknown strip returns empty presets", "[led][wled]") {
    WledBackend backend;
    const auto& result = backend.get_strip_presets("unknown");
    REQUIRE(result.empty());
}

TEST_CASE("WledBackend: clear removes presets", "[led][wled]") {
    WledBackend backend;
    backend.set_strip_presets("test", {{1, "Test"}});
    backend.clear();
    REQUIRE(backend.get_strip_presets("test").empty());
}

TEST_CASE("WledBackend: overwrite presets", "[led][wled]") {
    WledBackend backend;
    backend.set_strip_presets("test", {{1, "First"}, {2, "Second"}});
    backend.set_strip_presets("test", {{10, "New Preset"}});

    const auto& result = backend.get_strip_presets("test");
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].id == 10);
    REQUIRE(result[0].name == "New Preset");
}

TEST_CASE("WledBackend: per-strip presets are independent", "[led][wled]") {
    WledBackend backend;
    backend.set_strip_presets("strip_a", {{1, "Warm White"}, {2, "Rainbow"}});
    backend.set_strip_presets("strip_b", {{1, "Bright White"}});

    REQUIRE(backend.get_strip_presets("strip_a").size() == 2);
    REQUIRE(backend.get_strip_presets("strip_b").size() == 1);
    REQUIRE(backend.get_strip_presets("strip_b")[0].name == "Bright White");
}

// ============================================================================
// fetch_presets_from_device (without HTTP, just behavior check)
// ============================================================================

TEST_CASE("WledBackend: fetch_presets_from_device with no address calls on_complete",
          "[led][wled]") {
    WledBackend backend;
    bool completed = false;
    backend.fetch_presets_from_device("test_strip", [&]() { completed = true; });
    REQUIRE(completed);
}

TEST_CASE("WledBackend: fetch_presets_from_device with address calls on_complete", "[led][wled]") {
    WledBackend backend;
    backend.set_strip_address("test_strip", "192.168.1.50");
    bool completed = false;
    backend.fetch_presets_from_device("test_strip", [&]() { completed = true; });
    REQUIRE(completed);
}

// ============================================================================
// poll_status (without API, behavior check)
// ============================================================================

TEST_CASE("WledBackend: poll_status with no API calls on_complete", "[led][wled]") {
    WledBackend backend;
    bool completed = false;
    backend.poll_status([&]() { completed = true; });
    REQUIRE(completed);
}
