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

#include "ethernet_manager.h"
#include <spdlog/spdlog.h>

EthernetManager::EthernetManager() {
    spdlog::debug("[EthernetManager] Initializing Ethernet manager");

    // Create appropriate backend for this platform
    backend_ = EthernetBackend::create();

    if (!backend_) {
        spdlog::error("[EthernetManager] Failed to create backend");
        return;
    }

    spdlog::info("[EthernetManager] Ethernet manager initialized");
}

EthernetManager::~EthernetManager() {
    // Use fprintf - spdlog may be destroyed during static cleanup
    fprintf(stderr, "[EthernetManager] Shutting down Ethernet manager\n");
    backend_.reset();
}

bool EthernetManager::has_interface() {
    if (!backend_) {
        spdlog::warn("[EthernetManager] Backend not initialized");
        return false;
    }

    return backend_->has_interface();
}

EthernetInfo EthernetManager::get_info() {
    if (!backend_) {
        spdlog::warn("[EthernetManager] Backend not initialized");
        EthernetInfo info;
        info.status = "Backend error";
        return info;
    }

    return backend_->get_info();
}

std::string EthernetManager::get_ip_address() {
    EthernetInfo info = get_info();

    if (info.connected) {
        return info.ip_address;
    }

    return "";
}
