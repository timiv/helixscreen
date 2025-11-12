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

#include "ethernet_backend.h"
#include "ethernet_backend_mock.h"
#include <spdlog/spdlog.h>

#ifdef __APPLE__
#include "ethernet_backend_macos.h"
#else
#include "ethernet_backend_linux.h"
#endif

std::unique_ptr<EthernetBackend> EthernetBackend::create() {
#ifdef __APPLE__
    // macOS: Try native backend first, fallback to mock if no interface
    spdlog::debug("[EthernetBackend] Creating macOS backend");
    auto backend = std::make_unique<EthernetBackendMacOS>();

    if (backend->has_interface()) {
        spdlog::info("[EthernetBackend] macOS backend initialized (interface found)");
        return backend;
    }

    // No Ethernet interface found, use mock
    spdlog::warn("[EthernetBackend] No Ethernet interface found - using mock backend");
    return std::make_unique<EthernetBackendMock>();
#else
    // Linux: Try native backend first, fallback to mock if no interface
    spdlog::debug("[EthernetBackend] Creating Linux backend");
    auto backend = std::make_unique<EthernetBackendLinux>();

    if (backend->has_interface()) {
        spdlog::info("[EthernetBackend] Linux backend initialized (interface found)");
        return backend;
    }

    // No Ethernet interface found, use mock
    spdlog::warn("[EthernetBackend] No Ethernet interface found - using mock backend");
    return std::make_unique<EthernetBackendMock>();
#endif
}
