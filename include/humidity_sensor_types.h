// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace helix::sensors {

/// @brief Role assigned to a humidity sensor
enum class HumiditySensorRole {
    NONE = 0,    ///< Discovered but not assigned to a role
    CHAMBER = 1, ///< Used for monitoring chamber humidity
    DRYER = 2,   ///< Used for monitoring filament dryer humidity
};

/// @brief Type of humidity sensor hardware
enum class HumiditySensorType {
    BME280 = 1, ///< BME280 sensor (humidity, pressure, temperature)
    HTU21D = 2, ///< HTU21D sensor (humidity, temperature)
};

/// @brief Configuration for a humidity sensor
struct HumiditySensorConfig {
    std::string klipper_name; ///< Full Klipper name (e.g., "bme280 chamber")
    std::string sensor_name;  ///< Short name (e.g., "chamber")
    HumiditySensorType type = HumiditySensorType::BME280;
    HumiditySensorRole role = HumiditySensorRole::NONE;
    bool enabled = true;

    HumiditySensorConfig() = default;

    HumiditySensorConfig(std::string klipper_name_, std::string sensor_name_, HumiditySensorType type_)
        : klipper_name(std::move(klipper_name_)), sensor_name(std::move(sensor_name_)), type(type_) {}
};

/// @brief Runtime state for a humidity sensor
struct HumiditySensorState {
    float humidity = 0.0f;    ///< Humidity percentage (0-100)
    float pressure = 0.0f;    ///< Pressure in hPa (BME280 only, 0 for HTU21D)
    float temperature = 0.0f; ///< Temperature in degrees C
    bool available = false;   ///< Sensor available in current config
};

/// @brief Convert role enum to config string
/// @param role The role to convert
/// @return Config-safe string for JSON storage
[[nodiscard]] inline std::string humidity_role_to_string(HumiditySensorRole role) {
    switch (role) {
    case HumiditySensorRole::NONE:
        return "none";
    case HumiditySensorRole::CHAMBER:
        return "chamber";
    case HumiditySensorRole::DRYER:
        return "dryer";
    default:
        return "none";
    }
}

/// @brief Parse role string to enum
/// @param str The config string to parse
/// @return Parsed role, or NONE if unrecognized
[[nodiscard]] inline HumiditySensorRole humidity_role_from_string(const std::string& str) {
    if (str == "chamber")
        return HumiditySensorRole::CHAMBER;
    if (str == "dryer")
        return HumiditySensorRole::DRYER;
    return HumiditySensorRole::NONE;
}

/// @brief Convert role to display string
/// @param role The role to convert
/// @return Human-readable role name for UI display
[[nodiscard]] inline std::string humidity_role_to_display_string(HumiditySensorRole role) {
    switch (role) {
    case HumiditySensorRole::NONE:
        return "Unassigned";
    case HumiditySensorRole::CHAMBER:
        return "Chamber";
    case HumiditySensorRole::DRYER:
        return "Dryer";
    default:
        return "Unassigned";
    }
}

/// @brief Convert type enum to config string
/// @param type The type to convert
/// @return Config-safe string
[[nodiscard]] inline std::string humidity_type_to_string(HumiditySensorType type) {
    switch (type) {
    case HumiditySensorType::BME280:
        return "bme280";
    case HumiditySensorType::HTU21D:
        return "htu21d";
    default:
        return "bme280";
    }
}

/// @brief Parse type string to enum
/// @param str The config string to parse
/// @return Parsed type, defaults to BME280 if unrecognized
[[nodiscard]] inline HumiditySensorType humidity_type_from_string(const std::string& str) {
    if (str == "htu21d")
        return HumiditySensorType::HTU21D;
    return HumiditySensorType::BME280;
}

} // namespace helix::sensors
