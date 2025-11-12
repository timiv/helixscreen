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

#include "ethernet_backend_macos.h"
#include "ifconfig.h"  // libhv's cross-platform ifconfig utility
#include <spdlog/spdlog.h>
#include <algorithm>

EthernetBackendMacOS::EthernetBackendMacOS() {
    spdlog::debug("[EthernetMacOS] macOS backend created");
}

EthernetBackendMacOS::~EthernetBackendMacOS() {
    // Use fprintf - spdlog may be destroyed during static cleanup
    fprintf(stderr, "[EthernetMacOS] macOS backend destroyed\n");
}

bool EthernetBackendMacOS::is_ethernet_interface(const std::string& name) {
    // macOS Ethernet interfaces typically start with "en"
    // Common patterns:
    // - en0: Often WiFi, but sometimes Ethernet (Mac Minis, iMacs with built-in Ethernet)
    // - en1, en2, en3, ...: Thunderbolt Ethernet, USB adapters, etc.
    //
    // We'll accept all "en*" interfaces and let has_interface()/get_info()
    // determine if they're actually Ethernet based on connectivity.
    //
    // Exclude: bridge*, utun* (VPN tunnels), awdl* (Apple Wireless Direct Link)

    if (name.compare(0, 2, "en") == 0) {
        // Could be Ethernet
        return true;
    }

    return false;
}

bool EthernetBackendMacOS::has_interface() {
    std::vector<ifconfig_t> interfaces;
    int result = ifconfig(interfaces);

    if (result != 0) {
        spdlog::warn("[EthernetMacOS] ifconfig() failed with code: {}", result);
        return false;
    }

    // Look for any Ethernet-like interface
    for (const auto& iface : interfaces) {
        std::string name = iface.name;
        if (is_ethernet_interface(name)) {
            spdlog::debug("[EthernetMacOS] Found Ethernet interface: {}", name);
            return true;
        }
    }

    spdlog::debug("[EthernetMacOS] No Ethernet interface found");
    return false;
}

EthernetInfo EthernetBackendMacOS::get_info() {
    EthernetInfo info;

    std::vector<ifconfig_t> interfaces;
    int result = ifconfig(interfaces);

    if (result != 0) {
        spdlog::error("[EthernetMacOS] ifconfig() failed with code: {}", result);
        info.status = "Error querying interfaces";
        return info;
    }

    // Strategy: Find first Ethernet interface with valid IP
    // Preference order:
    // 1. First en* with valid IP (not 0.0.0.0 or 127.0.0.1)
    // 2. First en* interface found (even without IP)

    ifconfig_t* first_ethernet = nullptr;
    ifconfig_t* connected_ethernet = nullptr;

    for (auto& iface : interfaces) {
        std::string name = iface.name;
        std::string ip = iface.ip;

        if (!is_ethernet_interface(name)) {
            continue;
        }

        // Remember first Ethernet interface
        if (!first_ethernet) {
            first_ethernet = &iface;
        }

        // Check if it has a valid IP
        if (!ip.empty() && ip != "0.0.0.0" && ip != "127.0.0.1") {
            connected_ethernet = &iface;
            break;  // Found connected interface, use it
        }
    }

    // Use connected interface if found, otherwise use first interface
    ifconfig_t* selected = connected_ethernet ? connected_ethernet : first_ethernet;

    if (!selected) {
        // No Ethernet interface found
        info.status = "No Ethernet interface";
        spdlog::debug("[EthernetMacOS] No Ethernet interface found");
        return info;
    }

    // Populate info from selected interface
    info.interface = selected->name;
    info.ip_address = selected->ip;
    info.mac_address = selected->mac;

    // Determine connection status
    if (!info.ip_address.empty() && info.ip_address != "0.0.0.0" && info.ip_address != "127.0.0.1") {
        info.connected = true;
        info.status = "Connected";
        spdlog::info("[EthernetMacOS] Ethernet connected: {} ({})", info.interface, info.ip_address);
    } else {
        info.connected = false;
        info.status = "No connection";
        spdlog::debug("[EthernetMacOS] Ethernet interface {} has no IP", info.interface);
    }

    return info;
}
