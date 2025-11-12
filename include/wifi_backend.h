// Copyright 2025 HelixScreen
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

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief WiFi operation result with detailed error information
 */
enum class WiFiResult {
    SUCCESS = 0,               ///< Operation succeeded
    PERMISSION_DENIED,         ///< Insufficient permissions (socket access, etc.)
    HARDWARE_NOT_AVAILABLE,    ///< No WiFi hardware detected
    SERVICE_NOT_RUNNING,       ///< wpa_supplicant/network service not running
    INTERFACE_DOWN,            ///< WiFi interface is down/disabled
    RF_KILL_BLOCKED,           ///< WiFi blocked by RF-kill (hardware/software)
    CONNECTION_FAILED,         ///< Failed to connect to wpa_supplicant/service
    TIMEOUT,                   ///< Operation timed out
    AUTHENTICATION_FAILED,     ///< Wrong password or authentication error
    NETWORK_NOT_FOUND,         ///< Specified network not in range
    INVALID_PARAMETERS,        ///< Invalid SSID, password, or other parameters
    BACKEND_ERROR,             ///< Internal backend error
    NOT_INITIALIZED,           ///< Backend not started/initialized
    UNKNOWN_ERROR              ///< Unexpected error condition
};

/**
 * @brief Detailed error information for WiFi operations
 */
struct WiFiError {
    WiFiResult result;           ///< Primary error code
    std::string technical_msg;   ///< Technical details for logging/debugging
    std::string user_msg;        ///< User-friendly message for UI display
    std::string suggestion;      ///< Suggested action for user (optional)

    WiFiError(WiFiResult r = WiFiResult::SUCCESS,
              const std::string& tech = "",
              const std::string& user = "",
              const std::string& suggest = "")
        : result(r), technical_msg(tech), user_msg(user), suggestion(suggest) {}

    bool success() const { return result == WiFiResult::SUCCESS; }
    operator bool() const { return success(); }
};

/**
 * @brief Utility class for creating user-friendly WiFi error messages
 */
class WiFiErrorHelper {
public:
    /**
     * @brief Create permission denied error with helpful suggestions
     */
    static WiFiError permission_denied(const std::string& technical_detail) {
        return WiFiError(
            WiFiResult::PERMISSION_DENIED,
            technical_detail,
            "Permission denied - unable to access WiFi controls",
            "Try running as administrator or check user permissions"
        );
    }

    /**
     * @brief Create hardware not available error
     */
    static WiFiError hardware_not_available() {
        return WiFiError(
            WiFiResult::HARDWARE_NOT_AVAILABLE,
            "No WiFi interfaces detected",
            "No WiFi hardware found",
            "Check that WiFi hardware is installed and enabled"
        );
    }

    /**
     * @brief Create service not running error
     */
    static WiFiError service_not_running(const std::string& service_name) {
        return WiFiError(
            WiFiResult::SERVICE_NOT_RUNNING,
            service_name + " service not running or not accessible",
            "WiFi service unavailable",
            "Check that WiFi services are enabled and running"
        );
    }

    /**
     * @brief Create RF-kill blocked error
     */
    static WiFiError rf_kill_blocked() {
        return WiFiError(
            WiFiResult::RF_KILL_BLOCKED,
            "WiFi blocked by RF-kill (hardware or software switch)",
            "WiFi is disabled",
            "Check WiFi hardware switch or enable WiFi in system settings"
        );
    }

    /**
     * @brief Create interface down error
     */
    static WiFiError interface_down(const std::string& interface_name) {
        return WiFiError(
            WiFiResult::INTERFACE_DOWN,
            "WiFi interface " + interface_name + " is down",
            "WiFi interface is disabled",
            "Enable the WiFi interface in network settings"
        );
    }

    /**
     * @brief Create connection failed error
     */
    static WiFiError connection_failed(const std::string& technical_detail) {
        return WiFiError(
            WiFiResult::CONNECTION_FAILED,
            technical_detail,
            "Failed to connect to WiFi system",
            "Check that WiFi services are running and try again"
        );
    }

    /**
     * @brief Create authentication failed error
     */
    static WiFiError authentication_failed(const std::string& ssid) {
        return WiFiError(
            WiFiResult::AUTHENTICATION_FAILED,
            "Authentication failed for network: " + ssid,
            "Incorrect password or network authentication failed",
            "Verify the password and try again"
        );
    }

    /**
     * @brief Create network not found error
     */
    static WiFiError network_not_found(const std::string& ssid) {
        return WiFiError(
            WiFiResult::NETWORK_NOT_FOUND,
            "Network not found: " + ssid,
            "Network '" + ssid + "' is not in range",
            "Move closer to the network or check the network name"
        );
    }

    /**
     * @brief Create success result
     */
    static WiFiError success() {
        return WiFiError(WiFiResult::SUCCESS);
    }
};

/**
 * @brief WiFi network information
 */
struct WiFiNetwork {
    std::string ssid;           ///< Network name (SSID)
    int signal_strength;        ///< Signal strength (0-100 percentage)
    bool is_secured;            ///< True if network requires password
    std::string security_type;  ///< Security type ("WPA2", "WPA3", "WEP", "Open")

    WiFiNetwork() : signal_strength(0), is_secured(false) {}

    WiFiNetwork(const std::string& ssid_, int strength, bool secured, const std::string& security = "")
        : ssid(ssid_), signal_strength(strength), is_secured(secured), security_type(security) {}
};

/**
 * @brief Abstract WiFi backend interface
 *
 * Provides a clean, platform-agnostic API for WiFi operations.
 * Concrete implementations handle platform-specific details:
 * - WifiBackendWpaSupplicant: Linux wpa_supplicant integration
 * - WifiBackendMock: Simulator mode with fake data
 *
 * Design principles:
 * - Hide all backend-specific formats/commands from WiFiManager
 * - Provide async operations with event-based completion
 * - Thread-safe operations where needed
 * - Clean error handling with meaningful messages
 */
class WifiBackend {
public:
    virtual ~WifiBackend() = default;

    /**
     * @brief Connection status information
     */
    struct ConnectionStatus {
        bool connected;           ///< True if connected to a network
        std::string ssid;         ///< Connected network name
        std::string bssid;        ///< Access point MAC address
        std::string ip_address;   ///< Current IP address
        int signal_strength;      ///< Signal strength (0-100%)
    };

    // ========================================================================
    // Lifecycle Management
    // ========================================================================

    /**
     * @brief Initialize and start the WiFi backend
     *
     * Establishes connection to underlying WiFi system (wpa_supplicant, mock, etc.)
     * and starts any background processing threads.
     *
     * @return WiFiError with detailed status information
     */
    virtual WiFiError start() = 0;

    /**
     * @brief Stop the WiFi backend
     *
     * Cleanly shuts down background threads and connections.
     */
    virtual void stop() = 0;

    /**
     * @brief Check if backend is currently running/initialized
     *
     * @return true if backend is active and ready for operations
     */
    virtual bool is_running() const = 0;

    // ========================================================================
    // Event System
    // ========================================================================

    /**
     * @brief Register callback for WiFi events
     *
     * Events are delivered asynchronously and may arrive from background threads.
     * Ensure thread safety in callback implementations.
     *
     * Standard event types:
     * - "SCAN_COMPLETE" - Network scan finished
     * - "CONNECTED" - Successfully connected to network
     * - "DISCONNECTED" - Disconnected from network
     * - "AUTH_FAILED" - Authentication failed (wrong password, etc.)
     *
     * @param name Event type identifier
     * @param callback Handler function
     */
    virtual void register_event_callback(const std::string& name,
                                        std::function<void(const std::string&)> callback) = 0;

    // ========================================================================
    // Network Scanning
    // ========================================================================

    /**
     * @brief Trigger network scan (async)
     *
     * Initiates scan for available WiFi networks. Results delivered via
     * "SCAN_COMPLETE" event. Use get_scan_results() to retrieve networks.
     *
     * @return WiFiError with detailed status information
     */
    virtual WiFiError trigger_scan() = 0;

    /**
     * @brief Get scan results
     *
     * Returns networks discovered by the most recent scan.
     * Call after receiving "SCAN_COMPLETE" event for up-to-date results.
     *
     * @param[out] networks Vector to populate with discovered networks
     * @return WiFiError with detailed status information
     */
    virtual WiFiError get_scan_results(std::vector<WiFiNetwork>& networks) = 0;

    // ========================================================================
    // Connection Management
    // ========================================================================

    /**
     * @brief Connect to network (async)
     *
     * Initiates connection to specified network. Results delivered via
     * "CONNECTED" event (success) or "AUTH_FAILED"/"DISCONNECTED" (failure).
     *
     * @param ssid Network name
     * @param password Password (empty string for open networks)
     * @return WiFiError with detailed status information
     */
    virtual WiFiError connect_network(const std::string& ssid, const std::string& password) = 0;

    /**
     * @brief Disconnect from current network
     *
     * @return WiFiError with detailed status information
     */
    virtual WiFiError disconnect_network() = 0;

    // ========================================================================
    // Status Queries
    // ========================================================================

    /**
     * @brief Get current connection status
     *
     * @return ConnectionStatus struct with current state
     */
    virtual ConnectionStatus get_status() = 0;

    // ========================================================================
    // Factory Methods
    // ========================================================================

    /**
     * @brief Create appropriate backend for current platform
     *
     * - Linux: WifiBackendWpaSupplicant (real wpa_supplicant integration)
     * - macOS: WifiBackendMock (simulator with fake data)
     *
     * @return Unique pointer to backend instance
     */
    static std::unique_ptr<WifiBackend> create();
};