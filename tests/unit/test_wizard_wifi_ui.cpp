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
 */

#include "../catch_amalgamated.hpp"
#include "../ui_test_utils.h"
#include "../../include/ui_wizard.h"
#include "../../include/wifi_manager.h"
#include "../../include/config.h"
#include "../../include/ui_switch.h"
#include "../../lvgl/lvgl.h"
#include <thread>
#include <chrono>
#include <fstream>

/**
 * @brief Wizard WiFi UI Integration Tests
 *
 * Tests user interactions with the wizard WiFi setup screen:
 * - WiFi toggle enable/disable
 * - Network list population
 * - Secured network click → password modal
 * - Password entry and connection attempt
 * - Open network direct connection
 *
 * Uses UITest utilities for programmatic interaction simulation.
 */

// ============================================================================
// Test Helpers
// ============================================================================

static Config* create_test_config() {
    // Create minimal test config file
    const char* test_config_path = "/tmp/test_guppyconfig.json";
    std::ofstream config_file(test_config_path);
    config_file << R"({
        "default_printer": "test_printer",
        "printers": {
            "test_printer": {
                "moonraker_host": "127.0.0.1",
                "moonraker_port": 7125
            }
        }
    })";
    config_file.close();

    Config* config = new Config();
    config->init(test_config_path);
    return config;
}

// ============================================================================
// Test Fixture
// ============================================================================

class WizardWiFiUIFixture {
public:
    WizardWiFiUIFixture() {
        // Initialize LVGL (only once)
        static bool lvgl_initialized = false;
        if (!lvgl_initialized) {
            lv_init();
            lvgl_initialized = true;
        }

        // Create headless display for testing
        static lv_color_t buf[800 * 10];  // 10-line buffer
        display = lv_display_create(800, 480);
        lv_display_set_buffers(display, buf, nullptr, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(display, [](lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
            lv_display_flush_ready(disp);  // Dummy flush - just acknowledge
        });

        // Create test screen
        screen = lv_obj_create(lv_screen_active());
        lv_obj_set_size(screen, 800, 480);

        // Register XML components (only once)
        static bool components_registered = false;
        if (!components_registered) {
            lv_xml_component_register_from_file("A:ui_xml/globals.xml");
            lv_xml_component_register_from_file("A:ui_xml/network_list_item.xml");
            lv_xml_component_register_from_file("A:ui_xml/wifi_password_modal.xml");
            lv_xml_component_register_from_file("A:ui_xml/wizard_wifi_setup.xml");
            lv_xml_component_register_from_file("A:ui_xml/wizard_container.xml");

            // Register ui_switch custom component
            ui_switch_register();

            components_registered = true;
        }

        // Initialize wizard subjects
        ui_wizard_init_subjects();

        // Create test config
        test_config = create_test_config();

        // Create wizard UI (pass nullptr for moonraker client - not needed for WiFi tests)
        wizard = ui_wizard_create(screen, test_config, nullptr, []() {
            // Completion callback (not used in tests)
        });

        // Initialize UI test system
        UITest::init(screen);

        // Navigate to WiFi setup step
        ui_wizard_goto_step(WizardStep::WIFI_SETUP);
        UITest::wait_ms(100);  // Allow step to load
    }

    ~WizardWiFiUIFixture() {
        // Clean up in reverse order of creation
        UITest::cleanup();

        // Delete wizard and its children
        if (wizard) {
            lv_obj_delete(wizard);
            wizard = nullptr;
        }

        // Delete screen (this also deletes all its children)
        if (screen) {
            lv_obj_delete(screen);
            screen = nullptr;
        }

        // Delete display
        if (display) {
            lv_display_delete(display);
            display = nullptr;
        }

        // Delete config
        if (test_config) {
            delete test_config;
            test_config = nullptr;
        }
    }

    lv_obj_t* screen = nullptr;
    lv_display_t* display = nullptr;
    lv_obj_t* wizard = nullptr;
    Config* test_config = nullptr;
};

// ============================================================================
// WiFi Basic Toggle Tests
// ============================================================================

TEST_CASE_METHOD(WizardWiFiUIFixture, "Wizard WiFi: Basic toggle", "[wizard][wifi][ui]") {
    // WiFi starts disabled
    REQUIRE_FALSE(WiFiManager::is_enabled());

    // Network list should be disabled
    lv_obj_t* network_list = UITest::find_by_name(screen, "network_list_container");
    REQUIRE(network_list != nullptr);
    REQUIRE(lv_obj_has_state(network_list, LV_STATE_DISABLED));

    // Find toggle and click it
    lv_obj_t* toggle = UITest::find_by_name(screen, "wifi_toggle");
    REQUIRE(toggle != nullptr);

    UITest::click(toggle);
    UITest::wait_ms(100);

    // WiFi should now be enabled
    REQUIRE(WiFiManager::is_enabled());

    // Network list should now be enabled
    REQUIRE_FALSE(lv_obj_has_state(network_list, LV_STATE_DISABLED));
}

// ============================================================================
// WiFi Network Scan Tests
// ============================================================================

// TODO: Enable network scan test once fixture cleanup issues are resolved
// The test passes individually but fails when run after other tests due to
// LVGL/wizard state not being properly reset between fixtures.
//
// #ifdef __APPLE__  // macOS mock mode has WiFi scanning
// TEST_CASE_METHOD(WizardWiFiUIFixture, "Wizard WiFi: Network scan", "[wizard][wifi][ui]") {
//     ...
// }
// #endif
