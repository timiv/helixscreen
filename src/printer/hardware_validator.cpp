// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "hardware_validator.h"

#include "ui_nav_manager.h"
#include "ui_panel_settings.h"
#include "ui_toast_manager.h"

#include "config.h"
#include "moonraker_client.h"
#include "printer_capabilities.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// =============================================================================
// HardwareSnapshot Implementation
// =============================================================================

json HardwareSnapshot::to_json() const {
    return json{{"timestamp", timestamp}, {"heaters", heaters},
                {"sensors", sensors},     {"fans", fans},
                {"leds", leds},           {"filament_sensors", filament_sensors}};
}

HardwareSnapshot HardwareSnapshot::from_json(const json& j) {
    HardwareSnapshot snapshot;

    try {
        if (j.contains("timestamp") && j["timestamp"].is_string()) {
            snapshot.timestamp = j["timestamp"].get<std::string>();
        }
        if (j.contains("heaters") && j["heaters"].is_array()) {
            snapshot.heaters = j["heaters"].get<std::vector<std::string>>();
        }
        if (j.contains("sensors") && j["sensors"].is_array()) {
            snapshot.sensors = j["sensors"].get<std::vector<std::string>>();
        }
        if (j.contains("fans") && j["fans"].is_array()) {
            snapshot.fans = j["fans"].get<std::vector<std::string>>();
        }
        if (j.contains("leds") && j["leds"].is_array()) {
            snapshot.leds = j["leds"].get<std::vector<std::string>>();
        }
        if (j.contains("filament_sensors") && j["filament_sensors"].is_array()) {
            snapshot.filament_sensors = j["filament_sensors"].get<std::vector<std::string>>();
        }
    } catch (const std::exception& e) {
        spdlog::warn("[HardwareValidator] Failed to parse snapshot: {}", e.what());
        return HardwareSnapshot{}; // Return empty snapshot on error
    }

    return snapshot;
}

std::vector<std::string> HardwareSnapshot::get_removed(const HardwareSnapshot& current) const {
    std::vector<std::string> removed;

    // Helper to find items in 'old' but not in 'current'
    auto find_removed = [&removed](const std::vector<std::string>& old_list,
                                   const std::vector<std::string>& current_list) {
        for (const auto& item : old_list) {
            auto it = std::find(current_list.begin(), current_list.end(), item);
            if (it == current_list.end()) {
                removed.push_back(item);
            }
        }
    };

    find_removed(heaters, current.heaters);
    find_removed(sensors, current.sensors);
    find_removed(fans, current.fans);
    find_removed(leds, current.leds);
    find_removed(filament_sensors, current.filament_sensors);

    return removed;
}

std::vector<std::string> HardwareSnapshot::get_added(const HardwareSnapshot& current) const {
    std::vector<std::string> added;

    // Helper to find items in 'current' but not in 'old'
    auto find_added = [&added](const std::vector<std::string>& old_list,
                               const std::vector<std::string>& current_list) {
        for (const auto& item : current_list) {
            auto it = std::find(old_list.begin(), old_list.end(), item);
            if (it == old_list.end()) {
                added.push_back(item);
            }
        }
    };

    find_added(heaters, current.heaters);
    find_added(sensors, current.sensors);
    find_added(fans, current.fans);
    find_added(leds, current.leds);
    find_added(filament_sensors, current.filament_sensors);

    return added;
}

// =============================================================================
// HardwareValidator Implementation
// =============================================================================

HardwareValidationResult HardwareValidator::validate(Config* config, const MoonrakerClient* client,
                                                     const PrinterCapabilities& caps) {
    HardwareValidationResult result;

    if (!client) {
        spdlog::warn("[HardwareValidator] No client provided, skipping validation");
        return result;
    }

    spdlog::info("[HardwareValidator] Starting hardware validation...");

    // Step 1: Check critical hardware exists
    validate_critical_hardware(client, result);

    // Step 2: Check configured hardware exists
    validate_configured_hardware(config, client, caps, result);

    // Step 3: Find newly discovered hardware not in config
    validate_new_hardware(config, client, caps, result);

    // Step 4: Compare against previous session
    auto previous_snapshot = load_session_snapshot(config);
    if (previous_snapshot) {
        auto current_snapshot = create_snapshot(client, caps);
        validate_session_changes(*previous_snapshot, current_snapshot, config, result);
    }

    // Log summary
    if (result.has_issues()) {
        spdlog::info("[HardwareValidator] Validation complete: {} critical, {} expected missing, "
                     "{} new, {} changed",
                     result.critical_missing.size(), result.expected_missing.size(),
                     result.newly_discovered.size(), result.changed_from_last_session.size());
    } else {
        spdlog::info("[HardwareValidator] Validation complete: no issues found");
    }

    return result;
}

// Static callback for toast action button - navigates to Settings and opens overlay
static void on_hardware_toast_view_clicked(void* /*user_data*/) {
    spdlog::debug("[HardwareValidator] Toast 'View' clicked - opening Hardware Health overlay");
    ui_nav_set_active(UI_PANEL_SETTINGS);
    get_global_settings_panel().handle_hardware_health_clicked();
}

void HardwareValidator::notify_user(const HardwareValidationResult& result) {
    if (!result.has_issues()) {
        return;
    }

    std::string message;
    ToastSeverity severity = ToastSeverity::INFO;

    if (result.has_critical()) {
        if (result.critical_missing.size() == 1) {
            message = "Critical hardware missing: " + result.critical_missing[0].hardware_name;
        } else {
            message = std::to_string(result.critical_missing.size()) + " critical hardware issues";
        }
        severity = ToastSeverity::ERROR;
    } else if (!result.expected_missing.empty() || !result.changed_from_last_session.empty()) {
        size_t count = result.expected_missing.size() + result.changed_from_last_session.size();
        message = std::to_string(count) + " configured hardware not found";
        severity = ToastSeverity::WARNING;
    } else {
        message = std::to_string(result.newly_discovered.size()) + " new hardware discovered";
        severity = ToastSeverity::INFO;
    }

    // Show toast with action button to navigate to Hardware Health section
    ui_toast_show_with_action(severity, message.c_str(), "View", on_hardware_toast_view_clicked,
                              nullptr, 8000);
    spdlog::debug("[HardwareValidator] Notified user ({}): {}",
                  severity == ToastSeverity::ERROR     ? "error"
                  : severity == ToastSeverity::WARNING ? "warning"
                                                       : "info",
                  message);
}

void HardwareValidator::save_session_snapshot(Config* config, const MoonrakerClient* client) {
    if (!config || !client) {
        return;
    }

    // Create current snapshot
    PrinterCapabilities dummy_caps; // We'll get filament sensors from client's printer_objects
    auto snapshot = create_snapshot(client, dummy_caps);

    // Generate ISO 8601 timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    snapshot.timestamp = ss.str();

    // Save to config
    try {
        config->set<json>("/hardware_session/last_snapshot", snapshot.to_json());
        config->save();
        spdlog::debug(
            "[HardwareValidator] Saved session snapshot with {} heaters, {} fans, {} leds",
            snapshot.heaters.size(), snapshot.fans.size(), snapshot.leds.size());
    } catch (const std::exception& e) {
        spdlog::warn("[HardwareValidator] Failed to save session snapshot: {}", e.what());
    }
}

HardwareSnapshot HardwareValidator::create_snapshot(const MoonrakerClient* client,
                                                    const PrinterCapabilities& caps) {
    HardwareSnapshot snapshot;

    if (!client) {
        return snapshot;
    }

    snapshot.heaters = client->get_heaters();
    snapshot.sensors = client->get_sensors();
    snapshot.fans = client->get_fans();
    snapshot.leds = client->get_leds();
    snapshot.filament_sensors = caps.get_filament_sensor_names();

    return snapshot;
}

std::optional<HardwareSnapshot> HardwareValidator::load_session_snapshot(Config* config) {
    if (!config) {
        return std::nullopt;
    }

    try {
        json& snapshot_json = config->get_json("/hardware_session/last_snapshot");
        if (snapshot_json.is_null() || snapshot_json.empty()) {
            spdlog::debug("[HardwareValidator] No previous session snapshot found");
            return std::nullopt;
        }

        auto snapshot = HardwareSnapshot::from_json(snapshot_json);
        if (snapshot.is_empty()) {
            return std::nullopt;
        }

        spdlog::debug("[HardwareValidator] Loaded previous snapshot from {}", snapshot.timestamp);
        return snapshot;

    } catch (const std::exception& e) {
        spdlog::debug("[HardwareValidator] Failed to load session snapshot: {}", e.what());
        return std::nullopt;
    }
}

bool HardwareValidator::is_hardware_optional(Config* config, const std::string& hardware_name) {
    if (!config) {
        return false;
    }

    try {
        json& optional_list = config->get_json("/hardware/optional");
        if (optional_list.is_null() || !optional_list.is_array()) {
            return false;
        }

        for (const auto& item : optional_list) {
            if (item.is_string() && item.get<std::string>() == hardware_name) {
                return true;
            }
        }
    } catch (const std::exception& e) {
        spdlog::trace("[HardwareValidator] Error checking optional status: {}", e.what());
    }

    return false;
}

void HardwareValidator::set_hardware_optional(Config* config, const std::string& hardware_name,
                                              bool optional) {
    if (!config) {
        return;
    }

    try {
        // Ensure the hardware/optional array exists
        json& optional_list = config->get_json("/hardware/optional");
        if (optional_list.is_null() || !optional_list.is_array()) {
            config->set<json>("/hardware/optional", json::array());
            optional_list = config->get_json("/hardware/optional");
        }

        // Find if already in list
        auto it = std::find(optional_list.begin(), optional_list.end(), hardware_name);
        bool in_list = (it != optional_list.end());

        if (optional && !in_list) {
            // Add to list
            optional_list.push_back(hardware_name);
            spdlog::info("[HardwareValidator] Marked '{}' as optional", hardware_name);
        } else if (!optional && in_list) {
            // Remove from list
            optional_list.erase(it);
            spdlog::info("[HardwareValidator] Unmarked '{}' as optional", hardware_name);
        }

        config->save();

    } catch (const std::exception& e) {
        spdlog::warn("[HardwareValidator] Failed to set optional status: {}", e.what());
    }
}

void HardwareValidator::add_expected_hardware(Config* config, const std::string& hardware_name) {
    if (!config || hardware_name.empty()) {
        return;
    }

    try {
        // Ensure the hardware/expected array exists
        json& expected_list = config->get_json("/hardware/expected");
        if (expected_list.is_null() || !expected_list.is_array()) {
            config->set<json>("/hardware/expected", json::array());
            expected_list = config->get_json("/hardware/expected");
        }

        // Check if already in list
        auto it = std::find(expected_list.begin(), expected_list.end(), hardware_name);
        if (it == expected_list.end()) {
            expected_list.push_back(hardware_name);
            spdlog::info("[HardwareValidator] Added '{}' to expected hardware", hardware_name);
            config->save();
        } else {
            spdlog::debug("[HardwareValidator] '{}' already in expected list", hardware_name);
        }

    } catch (const std::exception& e) {
        spdlog::warn("[HardwareValidator] Failed to add expected hardware: {}", e.what());
    }
}

// =============================================================================
// Private Validation Methods
// =============================================================================

void HardwareValidator::validate_critical_hardware(const MoonrakerClient* client,
                                                   HardwareValidationResult& result) {
    const auto& heaters = client->get_heaters();

    // Check for extruder
    bool has_extruder = false;
    for (const auto& h : heaters) {
        if (h.find("extruder") != std::string::npos) {
            has_extruder = true;
            break;
        }
    }
    if (!has_extruder) {
        result.critical_missing.push_back(
            HardwareIssue::critical("extruder", HardwareType::HEATER,
                                    "No extruder heater found. Check [extruder] in printer.cfg"));
    }

    // Check for heater_bed (note: not all printers have heated beds)
    bool has_bed = contains_name(heaters, "heater_bed");
    if (!has_bed) {
        // This is a warning, not critical - some printers don't have heated beds
        spdlog::debug("[HardwareValidator] No heater_bed found (may be intentional)");
    }
}

void HardwareValidator::validate_configured_hardware(Config* config, const MoonrakerClient* client,
                                                     const PrinterCapabilities& caps,
                                                     HardwareValidationResult& result) {
    if (!config) {
        return;
    }

    const auto& heaters = client->get_heaters();
    const auto& fans = client->get_fans();
    const auto& leds = client->get_leds();
    const auto& filament_sensors = caps.get_filament_sensor_names();

    // Check configured heater (bed)
    try {
        std::string bed_name = config->get<std::string>(config->df() + "heater/bed", "heater_bed");
        if (!bed_name.empty() && !contains_name(heaters, bed_name)) {
            bool is_optional = is_hardware_optional(config, bed_name);
            result.expected_missing.push_back(HardwareIssue::warning(
                bed_name, HardwareType::HEATER, "Configured bed heater not found", is_optional));
        }
    } catch (...) {
        // Config key doesn't exist, that's fine
    }

    // Check configured heater (hotend)
    try {
        std::string hotend_name =
            config->get<std::string>(config->df() + "heater/hotend", "extruder");
        if (!hotend_name.empty() && !contains_name(heaters, hotend_name)) {
            bool is_optional = is_hardware_optional(config, hotend_name);
            result.expected_missing.push_back(
                HardwareIssue::warning(hotend_name, HardwareType::HEATER,
                                       "Configured hotend heater not found", is_optional));
        }
    } catch (...) {
    }

    // Check configured fan (part cooling)
    try {
        std::string part_fan = config->get<std::string>(config->df() + "fan/part", "fan");
        if (!part_fan.empty() && !contains_name(fans, part_fan)) {
            bool is_optional = is_hardware_optional(config, part_fan);
            result.expected_missing.push_back(HardwareIssue::warning(
                part_fan, HardwareType::FAN, "Configured part cooling fan not found", is_optional));
        }
    } catch (...) {
    }

    // Check configured fan (hotend)
    try {
        std::string hotend_fan = config->get<std::string>(config->df() + "fan/hotend", "");
        if (!hotend_fan.empty() && !contains_name(fans, hotend_fan)) {
            bool is_optional = is_hardware_optional(config, hotend_fan);
            result.expected_missing.push_back(HardwareIssue::warning(
                hotend_fan, HardwareType::FAN, "Configured hotend fan not found", is_optional));
        }
    } catch (...) {
    }

    // Check configured LED strip
    try {
        std::string led_strip = config->get<std::string>(config->df() + "led/strip", "");
        if (!led_strip.empty() && !contains_name(leds, led_strip)) {
            bool is_optional = is_hardware_optional(config, led_strip);
            result.expected_missing.push_back(HardwareIssue::warning(
                led_strip, HardwareType::LED, "Configured LED strip not found", is_optional));
        }
    } catch (...) {
    }

    // Check configured filament sensors
    try {
        json& sensors_config = config->get_json(config->df() + "filament_sensors/sensors");
        if (sensors_config.is_array()) {
            for (const auto& sensor : sensors_config) {
                if (sensor.is_object() && sensor.contains("name")) {
                    std::string sensor_name = sensor["name"].get<std::string>();
                    if (!contains_name(filament_sensors, sensor_name)) {
                        bool is_optional = is_hardware_optional(config, sensor_name);
                        result.expected_missing.push_back(HardwareIssue::warning(
                            sensor_name, HardwareType::FILAMENT_SENSOR,
                            "Configured filament sensor not found", is_optional));
                    }
                }
            }
        }
    } catch (...) {
    }
}

void HardwareValidator::validate_new_hardware(Config* config, const MoonrakerClient* client,
                                              const PrinterCapabilities& caps,
                                              HardwareValidationResult& result) {
    const auto& leds = client->get_leds();

    // Check for LEDs not in config
    // Only suggest if user hasn't configured any LED yet
    std::string configured_led;
    if (config) {
        try {
            configured_led = config->get<std::string>(config->df() + "led/strip", "");
        } catch (...) {
        }
    }

    if (configured_led.empty() && !leds.empty()) {
        // User has no LED configured but printer has some
        // Suggest the first "main" LED (prefer ones with "chamber", "case", "light" in name)
        std::string suggested;
        for (const auto& led : leds) {
            std::string lower = led;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("chamber") != std::string::npos ||
                lower.find("case") != std::string::npos ||
                lower.find("light") != std::string::npos) {
                suggested = led;
                break;
            }
        }
        if (suggested.empty() && !leds.empty()) {
            suggested = leds[0];
        }

        if (!suggested.empty()) {
            result.newly_discovered.push_back(
                HardwareIssue::info(suggested, HardwareType::LED,
                                    "LED strip available. Add to config for lighting control?"));
        }
    }

    // Check for filament sensors not in config
    const auto& discovered_sensors = caps.get_filament_sensor_names();
    std::vector<std::string> configured_names;

    if (config) {
        try {
            json& sensors_config = config->get_json(config->df() + "filament_sensors/sensors");
            if (sensors_config.is_array()) {
                for (const auto& sensor : sensors_config) {
                    if (sensor.is_object() && sensor.contains("name")) {
                        configured_names.push_back(sensor["name"].get<std::string>());
                    }
                }
            }
        } catch (...) {
            // No config, configured_names stays empty
        }
    }

    // Find sensors in discovery but not in config
    for (const auto& sensor : discovered_sensors) {
        if (!contains_name(configured_names, sensor)) {
            result.newly_discovered.push_back(HardwareIssue::info(
                sensor, HardwareType::FILAMENT_SENSOR,
                "Filament sensor available. Add to config for runout detection?"));
        }
    }
}

void HardwareValidator::validate_session_changes(const HardwareSnapshot& previous,
                                                 const HardwareSnapshot& current, Config* config,
                                                 HardwareValidationResult& result) {
    // Find hardware that was present before but is now missing
    auto removed = previous.get_removed(current);

    for (const auto& name : removed) {
        // Don't duplicate if already in expected_missing
        bool already_reported = false;
        for (const auto& issue : result.expected_missing) {
            if (issue.hardware_name == name) {
                already_reported = true;
                break;
            }
        }

        if (!already_reported) {
            bool is_optional = is_hardware_optional(config, name);
            if (!is_optional) {
                HardwareType type = guess_hardware_type(name);
                result.changed_from_last_session.push_back(HardwareIssue::warning(
                    name, type, "Hardware was present in previous session but is now missing",
                    false));
            }
        }
    }

    spdlog::debug("[HardwareValidator] Session comparison: {} removed, {} added since {}",
                  removed.size(), previous.get_added(current).size(), previous.timestamp);
}

// =============================================================================
// Helper Methods
// =============================================================================

bool HardwareValidator::contains_name(const std::vector<std::string>& vec,
                                      const std::string& name) {
    // Case-insensitive comparison
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    for (const auto& item : vec) {
        std::string lower_item = item;
        std::transform(lower_item.begin(), lower_item.end(), lower_item.begin(), ::tolower);
        if (lower_item == lower_name) {
            return true;
        }
    }
    return false;
}

HardwareType HardwareValidator::guess_hardware_type(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower.find("extruder") != std::string::npos ||
        lower.find("heater_bed") != std::string::npos ||
        lower.find("heater_generic") != std::string::npos) {
        return HardwareType::HEATER;
    }

    if (lower.find("temperature_sensor") != std::string::npos ||
        lower.find("temperature_fan") != std::string::npos) {
        return HardwareType::SENSOR;
    }

    if (lower.find("fan") != std::string::npos) {
        return HardwareType::FAN;
    }

    if (lower.find("neopixel") != std::string::npos || lower.find("led") != std::string::npos ||
        lower.find("dotstar") != std::string::npos) {
        return HardwareType::LED;
    }

    if (lower.find("filament") != std::string::npos) {
        return HardwareType::FILAMENT_SENSOR;
    }

    return HardwareType::OTHER;
}
