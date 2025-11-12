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

#include "ethernet_backend.h"

/**
 * @brief macOS Ethernet backend implementation
 *
 * Uses libhv's cross-platform ifconfig() utility to enumerate network
 * interfaces and detect Ethernet connectivity.
 *
 * Interface detection strategy:
 * - Filters for Ethernet interfaces (en1, en2, en3, etc.)
 * - Excludes loopback and common WiFi interfaces (en0 on some Macs)
 * - Returns first interface with valid IP address
 * - Falls back to any en* interface if found
 *
 * Note: macOS network interfaces are named differently from Linux:
 * - en0: Often WiFi (but sometimes Ethernet on Mac Minis/iMacs)
 * - en1, en2, en3: Typically Thunderbolt/USB Ethernet adapters
 * - en4+: Additional adapters
 */
class EthernetBackendMacOS : public EthernetBackend {
public:
    EthernetBackendMacOS();
    ~EthernetBackendMacOS() override;

    // ========================================================================
    // EthernetBackend Interface Implementation
    // ========================================================================

    bool has_interface() override;
    EthernetInfo get_info() override;

private:
    /**
     * @brief Check if interface name looks like Ethernet
     *
     * @param name Interface name (e.g., "en0", "en1")
     * @return true if name matches Ethernet pattern
     */
    bool is_ethernet_interface(const std::string& name);
};
