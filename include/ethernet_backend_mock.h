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
 * @brief Mock Ethernet backend for simulator and testing
 *
 * Provides fake Ethernet functionality with static data:
 * - Always reports interface as available
 * - Returns fixed IP address (192.168.1.150)
 * - Connected status
 * - Fake MAC address
 *
 * Perfect for:
 * - macOS/simulator development
 * - UI testing without real Ethernet hardware
 * - Automated testing scenarios
 * - Fallback when platform backends fail
 */
class EthernetBackendMock : public EthernetBackend {
public:
    EthernetBackendMock();
    ~EthernetBackendMock() override;

    // ========================================================================
    // EthernetBackend Interface Implementation
    // ========================================================================

    bool has_interface() override;
    EthernetInfo get_info() override;
};
