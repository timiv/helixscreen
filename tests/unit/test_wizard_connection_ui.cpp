// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_wizard.h"
#include "ui_wizard_connection.h"

#include "../lvgl_ui_test_fixture.h"
#include "../ui_test_utils.h"
#include "lvgl/lvgl.h"
#include "moonraker_client.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <thread>

#include "../catch_amalgamated.hpp"

// ============================================================================
// Test Fixture for Wizard Connection UI
// ============================================================================
// Extends LVGLUITestFixture which provides full XML component registration

class WizardConnectionUIFixture : public LVGLUITestFixture {
  public:
    WizardConnectionUIFixture() {
        // LVGLUITestFixture handles all LVGL, theme, widget, subject,
        // callback, and XML component initialization

        // Create the wizard container on the test screen
        wizard = ui_wizard_create(test_screen());
        if (!wizard) {
            spdlog::error("[WizardConnectionUIFixture] Failed to create wizard!");
            return;
        }

        // Navigate to step 2 (Connection screen)
        // NOTE: This starts mDNS discovery - keep test processing minimal
        ui_wizard_navigate_to_step(2);

        // Initialize UI test system with test screen
        UITest::init(test_screen());

        // Skip LVGL processing in constructor - let individual tests process
        // NOTE: mDNS timer processing was causing test hangs
    }

    ~WizardConnectionUIFixture() {
        UITest::cleanup();
        if (wizard) {
            lv_obj_delete(wizard);
            wizard = nullptr;
        }
        // LVGLUITestFixture destructor handles LVGL cleanup
    }

    lv_obj_t* wizard = nullptr;
};

// ============================================================================
// UI Widget Tests
// ============================================================================

// =============================================================================
// UI Integration Tests - Require XML component registration
// =============================================================================
// These tests are marked [.ui_integration] because they require:
// 1. XML components to be registered (wizard_container.xml, etc.)
// 2. LVGL filesystem driver to read ui_xml/ directory
//
// The test fixture's ensure_components_registered() is a stub that doesn't
// actually register XML components. To run these tests, you need to either:
// - Set up the XML filesystem driver in the test infrastructure
// - Run tests with: ./build/bin/helix-tests "[ui_integration]"
// =============================================================================

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: All widgets exist",
                 "[wizard][connection][ui][.ui_integration]") {
    // Ensure wizard was created successfully
    REQUIRE(wizard != nullptr);

    // Find the main connection screen widgets (search in wizard, not test_screen)
    lv_obj_t* ip_input = lv_obj_find_by_name(wizard, "ip_input");
    REQUIRE(ip_input != nullptr);

    lv_obj_t* port_input = lv_obj_find_by_name(wizard, "port_input");
    REQUIRE(port_input != nullptr);

    lv_obj_t* test_btn = lv_obj_find_by_name(wizard, "btn_test_connection");
    REQUIRE(test_btn != nullptr);

    // Note: connection_status_text is the actual widget name in XML
    lv_obj_t* status_label = lv_obj_find_by_name(wizard, "connection_status_text");
    REQUIRE(status_label != nullptr);
}

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Input field interaction",
                 "[wizard][connection][ui][.ui_integration]") {
    lv_obj_t* ip_input = UITest::find_by_name(test_screen(), "ip_input");
    REQUIRE(ip_input != nullptr);

    lv_obj_t* port_input = UITest::find_by_name(test_screen(), "port_input");
    REQUIRE(port_input != nullptr);

    // Type IP address
    UITest::type_text(ip_input, "192.168.1.100");
    UITest::wait_ms(50);

    // Verify text was entered
    std::string entered_ip = UITest::get_text(ip_input);
    REQUIRE(entered_ip == "192.168.1.100");

    // Check default port value
    std::string port_value = UITest::get_text(port_input);
    REQUIRE(port_value == "7125");

    // Modify port - clear by selecting all and typing over
    lv_textarea_set_cursor_pos(port_input, 0);
    lv_textarea_set_text(port_input, ""); // Clear existing text
    UITest::type_text(port_input, "8080");
    UITest::wait_ms(50);

    port_value = UITest::get_text(port_input);
    REQUIRE(port_value == "8080");
}

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Test button state",
                 "[wizard][connection][ui][.ui_integration]") {
    lv_obj_t* test_btn = UITest::find_by_name(test_screen(), "btn_test_connection");
    REQUIRE(test_btn != nullptr);

    // Button should not have the CLICKABLE flag removed
    bool has_clickable = lv_obj_has_flag(test_btn, LV_OBJ_FLAG_CLICKABLE);
    REQUIRE(has_clickable == true);

    // Button should be visible
    REQUIRE(UITest::is_visible(test_btn) == true);
}

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Status label updates",
                 "[wizard][connection][ui][.ui_integration]") {
    lv_obj_t* status_label = UITest::find_by_name(test_screen(), "connection_status");
    REQUIRE(status_label != nullptr);

    // Initially status should be empty or hidden
    std::string initial_status = UITest::get_text(status_label);
    REQUIRE(initial_status.empty());

    // Enter invalid IP
    lv_obj_t* ip_input = UITest::find_by_name(test_screen(), "ip_input");
    lv_textarea_set_text(ip_input, ""); // Clear existing text
    UITest::type_text(ip_input, "999.999.999.999");

    // Click test button
    lv_obj_t* test_btn = UITest::find_by_name(test_screen(), "btn_test_connection");
    UITest::click(test_btn);
    UITest::wait_ms(100);

    // Status should show error
    std::string error_status = UITest::get_text(status_label);
    REQUIRE(error_status.find("Invalid") != std::string::npos);
}

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Navigation buttons",
                 "[wizard][connection][ui][.ui_integration]") {
    // Find navigation buttons
    lv_obj_t* back_btn = UITest::find_by_name(test_screen(), "wizard_back_button");
    lv_obj_t* next_btn = UITest::find_by_name(test_screen(), "wizard_next_button");

    // Both should exist (even if back is hidden on step 1)
    REQUIRE(back_btn != nullptr);
    REQUIRE(next_btn != nullptr);

    // On step 2, back button should be visible
    REQUIRE(UITest::is_visible(back_btn) == true);

    // Next button should show "Next" text
    std::string next_text = UITest::get_text(next_btn);
    REQUIRE(next_text == "Next");
}

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Title and progress",
                 "[wizard][connection][ui][.ui_integration]") {
    // Find title and progress labels
    lv_obj_t* title = UITest::find_by_name(test_screen(), "wizard_title");
    lv_obj_t* progress = UITest::find_by_name(test_screen(), "wizard_progress");

    REQUIRE(title != nullptr);
    REQUIRE(progress != nullptr);

    // Check title text
    std::string title_text = UITest::get_text(title);
    REQUIRE(title_text == "Moonraker Connection");

    // Check progress text
    std::string progress_text = UITest::get_text(progress);
    REQUIRE(progress_text == "Step 2 of 7");
}

// ============================================================================
// Mock Connection Tests
// ============================================================================

// Mock MoonrakerClient for testing
class MockMoonrakerClient {
  public:
    int connect(const char* url, std::function<void()> on_connected,
                std::function<void()> on_disconnected) {
        last_url = url;
        connected_callback = on_connected;
        disconnected_callback = on_disconnected;
        return 0;
    }

    void trigger_connected() {
        if (connected_callback) {
            connected_callback();
        }
    }

    void trigger_disconnected() {
        if (disconnected_callback) {
            disconnected_callback();
        }
    }

    void set_connection_timeout(int timeout_ms) {
        timeout = timeout_ms;
    }

    ConnectionState get_connection_state() const {
        return state;
    }

    void close() {
        state = ConnectionState::DISCONNECTED;
    }

    std::string last_url;
    std::function<void()> connected_callback;
    std::function<void()> disconnected_callback;
    int timeout = 0;
    ConnectionState state = ConnectionState::DISCONNECTED;
};

TEST_CASE("Connection UI: Mock connection flow", "[wizard][connection][mock]") {
    MockMoonrakerClient mock_client;

    SECTION("Successful connection") {
        bool connected = false;

        mock_client.connect(
            "ws://192.168.1.100:7125/websocket", [&connected]() { connected = true; }, []() {});

        // Verify URL was captured
        REQUIRE(mock_client.last_url == "ws://192.168.1.100:7125/websocket");

        // Trigger successful connection
        mock_client.trigger_connected();

        REQUIRE(connected == true);
    }

    SECTION("Failed connection") {
        bool disconnected = false;

        mock_client.connect(
            "ws://192.168.1.100:7125/websocket", []() {},
            [&disconnected]() { disconnected = true; });

        // Trigger disconnection/failure
        mock_client.trigger_disconnected();

        REQUIRE(disconnected == true);
    }

    SECTION("Timeout configuration") {
        mock_client.set_connection_timeout(5000);
        REQUIRE(mock_client.timeout == 5000);
    }
}

// ============================================================================
// Input Validation UI Tests
// ============================================================================

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Input validation feedback",
                 "[wizard][connection][ui][validation][.ui_integration]") {
    lv_obj_t* ip_input = UITest::find_by_name(test_screen(), "ip_input");
    lv_obj_t* port_input = UITest::find_by_name(test_screen(), "port_input");
    lv_obj_t* test_btn = UITest::find_by_name(test_screen(), "btn_test_connection");
    lv_obj_t* status = UITest::find_by_name(test_screen(), "connection_status");

    SECTION("Empty IP address") {
        lv_textarea_set_text(ip_input, ""); // Clear text
        UITest::click(test_btn);
        UITest::wait_ms(100);

        std::string status_text = UITest::get_text(status);
        REQUIRE(status_text.find("enter") != std::string::npos);
    }

    SECTION("Invalid port") {
        UITest::type_text(ip_input, "192.168.1.100");
        lv_textarea_set_text(port_input, ""); // Clear text
        UITest::type_text(port_input, "99999");
        UITest::click(test_btn);
        UITest::wait_ms(100);

        std::string status_text = UITest::get_text(status);
        REQUIRE(status_text.find("Invalid port") != std::string::npos);
    }

    SECTION("Valid inputs") {
        lv_textarea_set_text(ip_input, ""); // Clear text
        UITest::type_text(ip_input, "printer.local");
        lv_textarea_set_text(port_input, ""); // Clear text
        UITest::type_text(port_input, "7125");

        // Status should allow testing with valid inputs
        UITest::click(test_btn);
        UITest::wait_ms(100);

        std::string status_text = UITest::get_text(status);
        // Should either be testing or show connection result
        REQUIRE((status_text.find("Testing") != std::string::npos ||
                 status_text.find("Connection") != std::string::npos));
    }
}

// ============================================================================
// Responsive Layout Tests
// ============================================================================

TEST_CASE_METHOD(WizardConnectionUIFixture, "Connection UI: Responsive layout",
                 "[wizard][connection][ui][responsive][.ui_integration]") {
    // Get the connection screen container
    lv_obj_t* container = UITest::find_by_name(test_screen(), "wizard_content");
    REQUIRE(container != nullptr);

    // Verify container uses flex layout
    lv_flex_flow_t flow = lv_obj_get_style_flex_flow(container, LV_PART_MAIN);
    REQUIRE(flow == LV_FLEX_FLOW_COLUMN);

    // Verify responsive sizing
    lv_coord_t width = lv_obj_get_width(container);
    lv_coord_t height = lv_obj_get_height(container);

    // Container should fill available space
    REQUIRE(width > 0);
    REQUIRE(height > 0);

    // Input fields should be responsive
    lv_obj_t* ip_input = UITest::find_by_name(test_screen(), "ip_input");
    lv_coord_t input_width = lv_obj_get_width(ip_input);

    // Input should be reasonably sized
    REQUIRE(input_width > 200);   // Minimum reasonable width
    REQUIRE(input_width < width); // Should not exceed container
}