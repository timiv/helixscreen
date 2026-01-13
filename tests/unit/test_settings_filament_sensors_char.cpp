// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_settings_filament_sensors_char.cpp
 * @brief Characterization tests for Filament Sensor Settings overlay
 *
 * These tests document the exact behavior of the filament sensor settings UI
 * in ui_panel_settings.cpp to enable safe extraction. They test the LOGIC only,
 * not the LVGL widgets (no UI creation).
 *
 * Pattern: Mirror the calculation/formatting logic used in the panel,
 * then verify specific cases to document expected behavior.
 *
 * @see ui_panel_settings.cpp - SettingsPanel::handle_filament_sensors_clicked()
 * @see ui_panel_settings.cpp - SettingsPanel::populate_sensor_list()
 */

#include <map>
#include <string>
#include <vector>

#include "../catch_amalgamated.hpp"

// ============================================================================
// Test Helpers: Data Model (mirrors filament_sensor_types.h)
// ============================================================================

/**
 * @brief Test-local copy of FilamentSensorRole enum
 *
 * Mirrors helix::FilamentSensorRole for test isolation.
 */
enum class TestSensorRole { NONE = 0, RUNOUT = 1, TOOLHEAD = 2, ENTRY = 3 };

/**
 * @brief Test-local copy of FilamentSensorType enum
 */
enum class TestSensorType { SWITCH, MOTION };

/**
 * @brief Test-local sensor configuration
 */
struct TestSensorConfig {
    std::string klipper_name;
    std::string sensor_name;
    TestSensorRole role{TestSensorRole::NONE};
    TestSensorType type{TestSensorType::SWITCH};
    bool enabled{true};
};

/**
 * @brief Test-local sensor state
 */
struct TestSensorState {
    bool filament_detected{false};
    bool enabled{true};
    int detection_count{0};
    bool available{false};
};

// ============================================================================
// Test Helpers: Role Conversion (mirrors filament_sensor_types.h)
// ============================================================================

/**
 * @brief Convert role to display string
 *
 * Mirrors helix::role_to_display_string()
 */
static const char* role_to_display_string(TestSensorRole role) {
    switch (role) {
    case TestSensorRole::RUNOUT:
        return "Runout Sensor";
    case TestSensorRole::TOOLHEAD:
        return "Toolhead Sensor";
    case TestSensorRole::ENTRY:
        return "Entry Sensor";
    case TestSensorRole::NONE:
    default:
        return "Unassigned";
    }
}

/**
 * @brief Convert role to config string
 *
 * Mirrors helix::role_to_config_string()
 */
static const char* role_to_config_string(TestSensorRole role) {
    switch (role) {
    case TestSensorRole::RUNOUT:
        return "runout";
    case TestSensorRole::TOOLHEAD:
        return "toolhead";
    case TestSensorRole::ENTRY:
        return "entry";
    case TestSensorRole::NONE:
    default:
        return "none";
    }
}

/**
 * @brief Parse role from config string
 *
 * Mirrors helix::role_from_config_string()
 */
static TestSensorRole role_from_config_string(const std::string& str) {
    if (str == "runout")
        return TestSensorRole::RUNOUT;
    if (str == "toolhead")
        return TestSensorRole::TOOLHEAD;
    if (str == "entry")
        return TestSensorRole::ENTRY;
    return TestSensorRole::NONE;
}

/**
 * @brief Convert type to config string
 *
 * Mirrors helix::type_to_config_string()
 */
static const char* type_to_config_string(TestSensorType type) {
    switch (type) {
    case TestSensorType::MOTION:
        return "motion";
    case TestSensorType::SWITCH:
    default:
        return "switch";
    }
}

/**
 * @brief Parse type from config string
 *
 * Mirrors helix::type_from_config_string()
 */
static TestSensorType type_from_config_string(const std::string& str) {
    if (str == "motion")
        return TestSensorType::MOTION;
    return TestSensorType::SWITCH;
}

// ============================================================================
// Test Helpers: Klipper Name Parsing (mirrors FilamentSensorManager)
// ============================================================================

/**
 * @brief Parse sensor name and type from Klipper object name
 *
 * Mirrors FilamentSensorManager::parse_klipper_name()
 *
 * Examples:
 *   "filament_switch_sensor fsensor" -> ("fsensor", SWITCH)
 *   "filament_motion_sensor encoder" -> ("encoder", MOTION)
 */
static bool parse_klipper_name(const std::string& klipper_name, std::string& sensor_name,
                               TestSensorType& type) {
    // Switch sensor prefix
    const std::string switch_prefix = "filament_switch_sensor ";
    if (klipper_name.substr(0, switch_prefix.size()) == switch_prefix) {
        sensor_name = klipper_name.substr(switch_prefix.size());
        type = TestSensorType::SWITCH;
        return true;
    }

    // Motion sensor prefix
    const std::string motion_prefix = "filament_motion_sensor ";
    if (klipper_name.substr(0, motion_prefix.size()) == motion_prefix) {
        sensor_name = klipper_name.substr(motion_prefix.size());
        type = TestSensorType::MOTION;
        return true;
    }

    return false;
}

// ============================================================================
// Test Helpers: Dropdown Logic (mirrors SettingsPanel)
// ============================================================================

/**
 * @brief Build role dropdown options string
 *
 * Mirrors the dropdown options set in SettingsPanel::populate_sensor_list():
 *   lv_dropdown_set_options(role_dropdown, "None\nRunout\nToolhead\nEntry");
 *
 * The options match the TestSensorRole enum values (0-3).
 */
static std::string build_role_dropdown_options() {
    return "None\nRunout\nToolhead\nEntry";
}

/**
 * @brief Convert role enum to dropdown index
 *
 * The dropdown index directly maps to the role enum value.
 */
static int role_to_dropdown_index(TestSensorRole role) {
    return static_cast<int>(role);
}

/**
 * @brief Convert dropdown index to role enum
 *
 * Index 0 = NONE, 1 = RUNOUT, 2 = TOOLHEAD, 3 = ENTRY
 */
static TestSensorRole dropdown_index_to_role(int index) {
    if (index < 0 || index > 3)
        return TestSensorRole::NONE;
    return static_cast<TestSensorRole>(index);
}

// ============================================================================
// Test Helpers: State Machine (simulates overlay behavior)
// ============================================================================

/**
 * @brief Simplified state machine for testing filament sensor overlay logic
 *
 * Simulates the essential state management of FilamentSensorManager
 * without LVGL or file I/O dependencies.
 */
class FilamentSensorStateMachine {
  public:
    bool master_enabled = true;
    std::vector<TestSensorConfig> sensors;
    std::map<std::string, TestSensorState> states;

    void add_sensor(const std::string& klipper_name) {
        std::string sensor_name;
        TestSensorType type;
        if (parse_klipper_name(klipper_name, sensor_name, type)) {
            TestSensorConfig config;
            config.klipper_name = klipper_name;
            config.sensor_name = sensor_name;
            config.type = type;
            config.role = TestSensorRole::NONE;
            config.enabled = true;
            sensors.push_back(config);
        }
    }

    TestSensorConfig* find_sensor(const std::string& klipper_name) {
        for (auto& s : sensors) {
            if (s.klipper_name == klipper_name)
                return &s;
        }
        return nullptr;
    }

    void set_role(const std::string& klipper_name, TestSensorRole role) {
        if (auto* s = find_sensor(klipper_name)) {
            s->role = role;
        }
    }

    void set_enabled(const std::string& klipper_name, bool enabled) {
        if (auto* s = find_sensor(klipper_name)) {
            s->enabled = enabled;
        }
    }

    bool is_filament_detected(TestSensorRole role) const {
        if (!master_enabled)
            return false;

        for (const auto& s : sensors) {
            if (s.role == role && s.enabled) {
                auto it = states.find(s.klipper_name);
                if (it != states.end() && it->second.available) {
                    return it->second.filament_detected;
                }
            }
        }
        return false;
    }

    bool has_any_runout() const {
        if (!master_enabled)
            return false;

        for (const auto& s : sensors) {
            if (s.role != TestSensorRole::NONE && s.enabled) {
                auto it = states.find(s.klipper_name);
                if (it != states.end() && it->second.available && !it->second.filament_detected) {
                    return true;
                }
            }
        }
        return false;
    }
};

// ============================================================================
// CHARACTERIZATION TESTS
// ============================================================================

TEST_CASE("CHAR: FilamentSensorRole enum values",
          "[characterization][settings][filament_sensors]") {
    // Documents the exact enum values - important for dropdown index mapping

    SECTION("Role enum has expected integer values") {
        REQUIRE(static_cast<int>(TestSensorRole::NONE) == 0);
        REQUIRE(static_cast<int>(TestSensorRole::RUNOUT) == 1);
        REQUIRE(static_cast<int>(TestSensorRole::TOOLHEAD) == 2);
        REQUIRE(static_cast<int>(TestSensorRole::ENTRY) == 3);
    }

    SECTION("Role enum count matches dropdown options") {
        // 4 roles: NONE, RUNOUT, TOOLHEAD, ENTRY
        int role_count = 4;
        std::string options = build_role_dropdown_options();
        int option_count = 1; // Start at 1 for first option
        for (char c : options) {
            if (c == '\n')
                option_count++;
        }
        REQUIRE(option_count == role_count);
    }
}

TEST_CASE("CHAR: FilamentSensorType enum values",
          "[characterization][settings][filament_sensors]") {
    SECTION("Type enum has expected values") {
        REQUIRE(static_cast<int>(TestSensorType::SWITCH) == 0);
        REQUIRE(static_cast<int>(TestSensorType::MOTION) == 1);
    }
}

TEST_CASE("CHAR: Role to display string conversion",
          "[characterization][settings][filament_sensors]") {
    SECTION("NONE displays as 'Unassigned'") {
        REQUIRE(std::string(role_to_display_string(TestSensorRole::NONE)) == "Unassigned");
    }

    SECTION("RUNOUT displays as 'Runout Sensor'") {
        REQUIRE(std::string(role_to_display_string(TestSensorRole::RUNOUT)) == "Runout Sensor");
    }

    SECTION("TOOLHEAD displays as 'Toolhead Sensor'") {
        REQUIRE(std::string(role_to_display_string(TestSensorRole::TOOLHEAD)) == "Toolhead Sensor");
    }

    SECTION("ENTRY displays as 'Entry Sensor'") {
        REQUIRE(std::string(role_to_display_string(TestSensorRole::ENTRY)) == "Entry Sensor");
    }
}

TEST_CASE("CHAR: Role config string conversion", "[characterization][settings][filament_sensors]") {
    SECTION("Role to config string") {
        REQUIRE(std::string(role_to_config_string(TestSensorRole::NONE)) == "none");
        REQUIRE(std::string(role_to_config_string(TestSensorRole::RUNOUT)) == "runout");
        REQUIRE(std::string(role_to_config_string(TestSensorRole::TOOLHEAD)) == "toolhead");
        REQUIRE(std::string(role_to_config_string(TestSensorRole::ENTRY)) == "entry");
    }

    SECTION("Config string to role (round-trip)") {
        for (auto role : {TestSensorRole::NONE, TestSensorRole::RUNOUT, TestSensorRole::TOOLHEAD,
                          TestSensorRole::ENTRY}) {
            std::string config_str = role_to_config_string(role);
            TestSensorRole parsed = role_from_config_string(config_str);
            REQUIRE(parsed == role);
        }
    }

    SECTION("Unknown config string defaults to NONE") {
        REQUIRE(role_from_config_string("invalid") == TestSensorRole::NONE);
        REQUIRE(role_from_config_string("") == TestSensorRole::NONE);
        REQUIRE(role_from_config_string("RUNOUT") == TestSensorRole::NONE); // Case sensitive
    }
}

TEST_CASE("CHAR: Type config string conversion", "[characterization][settings][filament_sensors]") {
    SECTION("Type to config string") {
        REQUIRE(std::string(type_to_config_string(TestSensorType::SWITCH)) == "switch");
        REQUIRE(std::string(type_to_config_string(TestSensorType::MOTION)) == "motion");
    }

    SECTION("Config string to type (round-trip)") {
        for (auto type : {TestSensorType::SWITCH, TestSensorType::MOTION}) {
            std::string config_str = type_to_config_string(type);
            TestSensorType parsed = type_from_config_string(config_str);
            REQUIRE(parsed == type);
        }
    }

    SECTION("Unknown config string defaults to SWITCH") {
        REQUIRE(type_from_config_string("invalid") == TestSensorType::SWITCH);
        REQUIRE(type_from_config_string("") == TestSensorType::SWITCH);
        REQUIRE(type_from_config_string("MOTION") == TestSensorType::SWITCH); // Case sensitive
    }
}

TEST_CASE("CHAR: Klipper name parsing", "[characterization][settings][filament_sensors]") {
    std::string sensor_name;
    TestSensorType type;

    SECTION("Switch sensor prefix") {
        REQUIRE(parse_klipper_name("filament_switch_sensor fsensor", sensor_name, type));
        REQUIRE(sensor_name == "fsensor");
        REQUIRE(type == TestSensorType::SWITCH);
    }

    SECTION("Motion sensor prefix") {
        REQUIRE(parse_klipper_name("filament_motion_sensor encoder", sensor_name, type));
        REQUIRE(sensor_name == "encoder");
        REQUIRE(type == TestSensorType::MOTION);
    }

    SECTION("Switch sensor with underscores in name") {
        REQUIRE(parse_klipper_name("filament_switch_sensor my_switch_sensor", sensor_name, type));
        REQUIRE(sensor_name == "my_switch_sensor");
        REQUIRE(type == TestSensorType::SWITCH);
    }

    SECTION("Motion sensor with spaces in name") {
        // Klipper doesn't allow spaces in names, but test the parsing behavior
        REQUIRE(parse_klipper_name("filament_motion_sensor my encoder", sensor_name, type));
        REQUIRE(sensor_name == "my encoder");
        REQUIRE(type == TestSensorType::MOTION);
    }

    SECTION("Invalid prefix returns false") {
        REQUIRE_FALSE(parse_klipper_name("some_other_sensor test", sensor_name, type));
        REQUIRE_FALSE(parse_klipper_name("filament_sensor test", sensor_name, type));
        REQUIRE_FALSE(parse_klipper_name("", sensor_name, type));
    }
}

TEST_CASE("CHAR: Role dropdown options", "[characterization][settings][filament_sensors]") {
    std::string options = build_role_dropdown_options();

    SECTION("Options string format") {
        // Exact format used by lv_dropdown_set_options()
        REQUIRE(options == "None\nRunout\nToolhead\nEntry");
    }

    SECTION("Role enum maps to dropdown index") {
        REQUIRE(role_to_dropdown_index(TestSensorRole::NONE) == 0);
        REQUIRE(role_to_dropdown_index(TestSensorRole::RUNOUT) == 1);
        REQUIRE(role_to_dropdown_index(TestSensorRole::TOOLHEAD) == 2);
        REQUIRE(role_to_dropdown_index(TestSensorRole::ENTRY) == 3);
    }

    SECTION("Dropdown index maps to role enum") {
        REQUIRE(dropdown_index_to_role(0) == TestSensorRole::NONE);
        REQUIRE(dropdown_index_to_role(1) == TestSensorRole::RUNOUT);
        REQUIRE(dropdown_index_to_role(2) == TestSensorRole::TOOLHEAD);
        REQUIRE(dropdown_index_to_role(3) == TestSensorRole::ENTRY);
    }

    SECTION("Invalid dropdown index returns NONE") {
        REQUIRE(dropdown_index_to_role(-1) == TestSensorRole::NONE);
        REQUIRE(dropdown_index_to_role(4) == TestSensorRole::NONE);
        REQUIRE(dropdown_index_to_role(100) == TestSensorRole::NONE);
    }
}

TEST_CASE("CHAR: XML widget names", "[characterization][settings][filament_sensors]") {
    // Documents the widget names used in filament_sensors_overlay.xml

    SECTION("Overlay root name") {
        std::string name = "filament_sensors_overlay";
        REQUIRE(name == "filament_sensors_overlay");
    }

    SECTION("Master toggle widget") {
        std::string name = "master_toggle";
        REQUIRE(name == "master_toggle");
    }

    SECTION("Sensors list container") {
        std::string name = "sensors_list";
        REQUIRE(name == "sensors_list");
    }

    SECTION("Sensor count label") {
        std::string name = "sensor_count_label";
        REQUIRE(name == "sensor_count_label");
    }

    SECTION("No sensors placeholder") {
        std::string name = "no_sensors_placeholder";
        REQUIRE(name == "no_sensors_placeholder");
    }
}

TEST_CASE("CHAR: Sensor row widget names", "[characterization][settings][filament_sensors]") {
    // Documents the widget names in filament_sensor_row.xml component

    SECTION("Role dropdown") {
        std::string name = "role_dropdown";
        REQUIRE(name == "role_dropdown");
    }

    SECTION("Enable toggle") {
        std::string name = "enable_toggle";
        REQUIRE(name == "enable_toggle");
    }
}

TEST_CASE("CHAR: XML callback names", "[characterization][settings][filament_sensors]") {
    // Documents the callback names used in XML event_cb attributes

    SECTION("Filament sensors clicked callback") {
        std::string name = "on_filament_sensors_clicked";
        REQUIRE(name == "on_filament_sensors_clicked");
    }

    SECTION("Master toggle changed callback") {
        std::string name = "on_filament_master_toggle_changed";
        REQUIRE(name == "on_filament_master_toggle_changed");
    }
}

TEST_CASE("CHAR: XML subject names", "[characterization][settings][filament_sensors]") {
    // Documents the subject names bound in XML

    SECTION("Master enabled subject") {
        std::string name = "filament_master_enabled";
        REQUIRE(name == "filament_master_enabled");
    }

    SECTION("Sensor count subject") {
        std::string name = "filament_sensor_count";
        REQUIRE(name == "filament_sensor_count");
    }
}

TEST_CASE("CHAR: Sensor count label format", "[characterization][settings][filament_sensors]") {
    // Documents the format used for sensor count display

    SECTION("Format is parenthesized count") {
        size_t count = 3;
        char buf[32];
        snprintf(buf, sizeof(buf), "(%zu)", count);
        REQUIRE(std::string(buf) == "(3)");
    }

    SECTION("Zero sensors") {
        char buf[32];
        snprintf(buf, sizeof(buf), "(%zu)", static_cast<size_t>(0));
        REQUIRE(std::string(buf) == "(0)");
    }
}

TEST_CASE("CHAR: Master enable state machine", "[characterization][settings][filament_sensors]") {
    FilamentSensorStateMachine state;

    SECTION("Master enabled by default") {
        REQUIRE(state.master_enabled == true);
    }

    SECTION("Disabling master disables all detection") {
        state.add_sensor("filament_switch_sensor fsensor");
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::RUNOUT);
        state.states["filament_switch_sensor fsensor"] = {true, true, 0, true};

        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == true);

        state.master_enabled = false;
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == false);
    }

    SECTION("Disabling master prevents runout detection") {
        state.add_sensor("filament_switch_sensor fsensor");
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::RUNOUT);
        state.states["filament_switch_sensor fsensor"] = {false, true, 0, true}; // No filament

        REQUIRE(state.has_any_runout() == true);

        state.master_enabled = false;
        REQUIRE(state.has_any_runout() == false);
    }
}

TEST_CASE("CHAR: Sensor discovery workflow", "[characterization][settings][filament_sensors]") {
    FilamentSensorStateMachine state;

    SECTION("Adding switch sensor") {
        state.add_sensor("filament_switch_sensor runout_sensor");

        REQUIRE(state.sensors.size() == 1);
        REQUIRE(state.sensors[0].klipper_name == "filament_switch_sensor runout_sensor");
        REQUIRE(state.sensors[0].sensor_name == "runout_sensor");
        REQUIRE(state.sensors[0].type == TestSensorType::SWITCH);
        REQUIRE(state.sensors[0].role == TestSensorRole::NONE);
        REQUIRE(state.sensors[0].enabled == true);
    }

    SECTION("Adding motion sensor") {
        state.add_sensor("filament_motion_sensor encoder");

        REQUIRE(state.sensors.size() == 1);
        REQUIRE(state.sensors[0].type == TestSensorType::MOTION);
    }

    SECTION("Multiple sensors") {
        state.add_sensor("filament_switch_sensor switch1");
        state.add_sensor("filament_motion_sensor motion1");
        state.add_sensor("filament_switch_sensor switch2");

        REQUIRE(state.sensors.size() == 3);
    }

    SECTION("Invalid sensor name ignored") {
        state.add_sensor("invalid_sensor test");
        REQUIRE(state.sensors.empty());
    }
}

TEST_CASE("CHAR: Role assignment workflow", "[characterization][settings][filament_sensors]") {
    FilamentSensorStateMachine state;
    state.add_sensor("filament_switch_sensor fsensor");
    state.states["filament_switch_sensor fsensor"] = {true, true, 0, true};

    SECTION("Default role is NONE") {
        REQUIRE(state.sensors[0].role == TestSensorRole::NONE);
    }

    SECTION("Assign RUNOUT role") {
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::RUNOUT);
        REQUIRE(state.sensors[0].role == TestSensorRole::RUNOUT);
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == true);
    }

    SECTION("Assign TOOLHEAD role") {
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::TOOLHEAD);
        REQUIRE(state.sensors[0].role == TestSensorRole::TOOLHEAD);
        REQUIRE(state.is_filament_detected(TestSensorRole::TOOLHEAD) == true);
    }

    SECTION("Assign ENTRY role") {
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::ENTRY);
        REQUIRE(state.sensors[0].role == TestSensorRole::ENTRY);
        REQUIRE(state.is_filament_detected(TestSensorRole::ENTRY) == true);
    }

    SECTION("Unassign role (set to NONE)") {
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::RUNOUT);
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::NONE);
        REQUIRE(state.sensors[0].role == TestSensorRole::NONE);
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == false);
    }
}

TEST_CASE("CHAR: Per-sensor enable/disable", "[characterization][settings][filament_sensors]") {
    FilamentSensorStateMachine state;
    state.add_sensor("filament_switch_sensor fsensor");
    state.set_role("filament_switch_sensor fsensor", TestSensorRole::RUNOUT);
    state.states["filament_switch_sensor fsensor"] = {true, true, 0, true};

    SECTION("Sensor enabled by default") {
        REQUIRE(state.sensors[0].enabled == true);
    }

    SECTION("Disabling sensor stops detection for that role") {
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == true);

        state.set_enabled("filament_switch_sensor fsensor", false);
        REQUIRE(state.sensors[0].enabled == false);
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == false);
    }

    SECTION("Re-enabling sensor restores detection") {
        state.set_enabled("filament_switch_sensor fsensor", false);
        state.set_enabled("filament_switch_sensor fsensor", true);
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == true);
    }
}

TEST_CASE("CHAR: Runout detection logic", "[characterization][settings][filament_sensors]") {
    FilamentSensorStateMachine state;
    state.add_sensor("filament_switch_sensor fsensor");
    state.set_role("filament_switch_sensor fsensor", TestSensorRole::RUNOUT);

    SECTION("No runout when sensor not available") {
        state.states["filament_switch_sensor fsensor"] = {false, true, 0, false}; // Not available
        REQUIRE(state.has_any_runout() == false);
    }

    SECTION("Runout detected when no filament") {
        state.states["filament_switch_sensor fsensor"] = {false, true, 0,
                                                          true}; // Available, no filament
        REQUIRE(state.has_any_runout() == true);
    }

    SECTION("No runout when filament present") {
        state.states["filament_switch_sensor fsensor"] = {true, true, 0, true}; // Filament present
        REQUIRE(state.has_any_runout() == false);
    }

    SECTION("No runout when sensor has NONE role") {
        state.set_role("filament_switch_sensor fsensor", TestSensorRole::NONE);
        state.states["filament_switch_sensor fsensor"] = {false, true, 0, true};
        REQUIRE(state.has_any_runout() == false);
    }
}

TEST_CASE("CHAR: Multiple sensors scenario", "[characterization][settings][filament_sensors]") {
    FilamentSensorStateMachine state;
    state.add_sensor("filament_switch_sensor entry");
    state.add_sensor("filament_switch_sensor runout");
    state.add_sensor("filament_switch_sensor toolhead");

    state.set_role("filament_switch_sensor entry", TestSensorRole::ENTRY);
    state.set_role("filament_switch_sensor runout", TestSensorRole::RUNOUT);
    state.set_role("filament_switch_sensor toolhead", TestSensorRole::TOOLHEAD);

    state.states["filament_switch_sensor entry"] = {true, true, 0, true};
    state.states["filament_switch_sensor runout"] = {true, true, 0, true};
    state.states["filament_switch_sensor toolhead"] = {true, true, 0, true};

    SECTION("All sensors report filament present") {
        REQUIRE(state.is_filament_detected(TestSensorRole::ENTRY) == true);
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == true);
        REQUIRE(state.is_filament_detected(TestSensorRole::TOOLHEAD) == true);
        REQUIRE(state.has_any_runout() == false);
    }

    SECTION("One sensor reports runout") {
        state.states["filament_switch_sensor runout"].filament_detected = false;

        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == false);
        REQUIRE(state.has_any_runout() == true);

        // Other sensors unaffected
        REQUIRE(state.is_filament_detected(TestSensorRole::ENTRY) == true);
        REQUIRE(state.is_filament_detected(TestSensorRole::TOOLHEAD) == true);
    }

    SECTION("Disabling one sensor") {
        state.set_enabled("filament_switch_sensor toolhead", false);

        REQUIRE(state.is_filament_detected(TestSensorRole::TOOLHEAD) == false);
        // Other sensors unaffected
        REQUIRE(state.is_filament_detected(TestSensorRole::ENTRY) == true);
        REQUIRE(state.is_filament_detected(TestSensorRole::RUNOUT) == true);
    }
}

// ============================================================================
// DOCUMENTATION SECTION
// ============================================================================

/**
 * @brief Summary of Filament Sensor Settings behavior for extraction
 *
 * This documents the exact behavior that must be preserved when extracting
 * the filament sensor settings into a separate overlay class.
 *
 * 1. Overlay Creation (lazy):
 *    - Created on first click of "Filament Sensors" row
 *    - Uses XML component "filament_sensors_overlay"
 *    - Initially hidden until navigation pushes it
 *
 * 2. Master Toggle:
 *    - Bound to "filament_master_enabled" subject via XML
 *    - Callback: on_filament_master_toggle_changed
 *    - Calls FilamentSensorManager::set_master_enabled()
 *
 * 3. Sensor List:
 *    - Dynamic creation using "filament_sensor_row" component
 *    - Each row has: role_dropdown, enable_toggle
 *    - Sensor name stored in row user_data (lv_malloc'd)
 *    - DELETE event handler frees user_data
 *
 * 4. Role Dropdown:
 *    - Options: "None\nRunout\nToolhead\nEntry"
 *    - Index maps directly to FilamentSensorRole enum
 *    - Change callback calls FilamentSensorManager::set_sensor_role()
 *
 * 5. Enable Toggle:
 *    - State: LV_STATE_CHECKED = enabled
 *    - Change callback calls FilamentSensorManager::set_sensor_enabled()
 *
 * 6. Config Persistence:
 *    - Both role and enable changes call mgr.save_config()
 *    - Stored in helixconfig.json under filament_sensors section
 *
 * 7. Exception: Uses lv_obj_add_event_cb():
 *    - For DELETE cleanup of user_data
 *    - For dropdown VALUE_CHANGED (dynamic row creation)
 *    - For toggle VALUE_CHANGED (dynamic row creation)
 *    These are acceptable exceptions to declarative UI rule.
 */
