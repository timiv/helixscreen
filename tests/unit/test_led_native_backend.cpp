// SPDX-License-Identifier: GPL-3.0-or-later

#include "led/led_controller.h"

#include "../catch_amalgamated.hpp"

TEST_CASE("NativeBackend: set_color with null API calls error callback", "[led][native]") {
    helix::led::NativeBackend backend;
    // No API set

    bool error_called = false;
    std::string error_msg;
    backend.set_color("neopixel test", 1.0, 0.0, 0.0, 0.0, nullptr, [&](const std::string& err) {
        error_called = true;
        error_msg = err;
    });

    REQUIRE(error_called);
    REQUIRE(!error_msg.empty());
}

TEST_CASE("NativeBackend: turn_on with null API calls error callback", "[led][native]") {
    helix::led::NativeBackend backend;

    bool error_called = false;
    backend.turn_on("neopixel test", nullptr, [&](const std::string& err) { error_called = true; });

    REQUIRE(error_called);
}

TEST_CASE("NativeBackend: turn_off with null API calls error callback", "[led][native]") {
    helix::led::NativeBackend backend;

    bool error_called = false;
    backend.turn_off("neopixel test", nullptr,
                     [&](const std::string& err) { error_called = true; });

    REQUIRE(error_called);
}

TEST_CASE("NativeBackend: set_brightness with null API calls error callback", "[led][native]") {
    helix::led::NativeBackend backend;

    bool error_called = false;
    backend.set_brightness("neopixel test", 50, 1.0, 1.0, 1.0, 0.0, nullptr,
                           [&](const std::string& err) { error_called = true; });

    REQUIRE(error_called);
}

TEST_CASE("NativeBackend: null error callback with null API doesn't crash", "[led][native]") {
    helix::led::NativeBackend backend;

    // Should not crash even without callbacks
    backend.set_color("neopixel test", 1.0, 0.0, 0.0, 0.0, nullptr, nullptr);
    backend.turn_on("neopixel test", nullptr, nullptr);
    backend.turn_off("neopixel test", nullptr, nullptr);
    backend.set_brightness("neopixel test", 50, 1.0, 1.0, 1.0, 0.0, nullptr, nullptr);
}

TEST_CASE("NativeBackend: strip type detection", "[led][native]") {
    helix::led::NativeBackend backend;

    REQUIRE(backend.type() == helix::led::LedBackendType::NATIVE);
}
