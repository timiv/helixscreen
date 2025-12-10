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

#include <string>
#include <vector>

/**
 * @brief Linux Ethernet backend implementation
 *
 * Uses libhv's cross-platform ifconfig() utility to enumerate network
 * interfaces, plus Linux-specific /sys/class/net for detailed status.
 *
 * Interface detection strategy:
 * - Filters for physical Ethernet interfaces (eth*, eno*, enp*, ens*)
 * - Excludes loopback, WiFi, virtual interfaces (docker*, virbr*, etc.)
 * - Checks /sys/class/net/<interface>/operstate for link status
 * - Returns first interface with "up" operstate and valid IP
 *
 * Linux interface naming:
 * - eth0, eth1: Traditional naming (older systems)
 * - eno1: Onboard Ethernet (BIOS/firmware naming)
 * - enp*s*: PCI bus/slot naming (e.g., enp3s0)
 * - ens*: Hot-plug naming scheme
 */
class EthernetBackendLinux : public EthernetBackend {
  public:
    EthernetBackendLinux();
    ~EthernetBackendLinux() override;

    // ========================================================================
    // EthernetBackend Interface Implementation
    // ========================================================================

    bool has_interface() override;
    EthernetInfo get_info() override;

  private:
    /**
     * @brief Check if interface name looks like physical Ethernet
     *
     * @param name Interface name (e.g., "eth0", "enp3s0")
     * @return true if name matches Ethernet pattern
     */
    bool is_ethernet_interface(const std::string& name);

    /**
     * @brief Read interface operstate from sysfs
     *
     * Reads /sys/class/net/<interface>/operstate
     *
     * @param interface Interface name
     * @return "up", "down", "unknown", or empty string on error
     */
    std::string read_operstate(const std::string& interface);

    /**
     * @brief Scan /sys/class/net/ for ethernet interfaces
     *
     * This method scans the sysfs network directory directly, which finds
     * interfaces regardless of their IP address or connection state. This is
     * more reliable than ifconfig() which may not return interfaces without IPs.
     *
     * @return Vector of ethernet interface names (e.g., {"eth0", "enp3s0"})
     */
    std::vector<std::string> scan_sysfs_interfaces();
};
