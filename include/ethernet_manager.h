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
#include <memory>
#include "ethernet_backend.h"

/**
 * @brief Ethernet Manager - High-level interface for Ethernet status queries
 *
 * Provides simple API for checking Ethernet connectivity and retrieving
 * network information. Uses pluggable backend system:
 * - macOS: EthernetBackendMacOS (libhv ifconfig + native APIs)
 * - Linux: EthernetBackendLinux (libhv ifconfig + sysfs)
 * - Fallback: EthernetBackendMock (simulator/testing)
 *
 * Usage:
 * ```cpp
 * auto manager = std::make_unique<EthernetManager>();
 *
 * if (manager->has_interface()) {
 *     std::string ip = manager->get_ip_address();
 *     if (!ip.empty()) {
 *         // Display "Connected (192.168.1.100)"
 *     }
 * }
 * ```
 *
 * Key features:
 * - Query-only API (no configuration/enable/disable)
 * - Automatic backend selection per platform
 * - Synchronous operations (no async complexity)
 * - Simple error handling
 */
class EthernetManager {
public:
    /**
     * @brief Initialize Ethernet manager with appropriate backend
     *
     * Automatically selects platform-appropriate backend:
     * - macOS: EthernetBackendMacOS
     * - Linux: EthernetBackendLinux
     * - Fallback: EthernetBackendMock (if no interface found)
     */
    EthernetManager();

    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~EthernetManager();

    // ========================================================================
    // Status Queries
    // ========================================================================

    /**
     * @brief Check if any Ethernet interface is present
     *
     * Returns true if Ethernet hardware is detected, regardless of
     * connection status or IP assignment.
     *
     * @return true if at least one Ethernet interface exists
     */
    bool has_interface();

    /**
     * @brief Get detailed Ethernet connection information
     *
     * Returns comprehensive status including interface name, IP address,
     * MAC address, and connection status.
     *
     * @return EthernetInfo struct with current state
     */
    EthernetInfo get_info();

    /**
     * @brief Get Ethernet IP address (convenience method)
     *
     * Returns IP address if connected, empty string otherwise.
     * Useful for quick status display in UI.
     *
     * @return IP address string (e.g., "192.168.1.100"), or empty if not connected
     */
    std::string get_ip_address();

private:
    std::unique_ptr<EthernetBackend> backend_;
};
