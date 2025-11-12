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

#include "wifi_backend.h"
#include "wifi_backend_mock.h"
#include "spdlog/spdlog.h"

#ifdef __APPLE__
#include "wifi_backend_macos.h"
#else
#include "wifi_backend_wpa_supplicant.h"
#endif

std::unique_ptr<WifiBackend> WifiBackend::create() {
#ifdef __APPLE__
    // macOS: Try CoreWLAN backend first, fallback to mock if unavailable
    spdlog::debug("[WifiBackend] Attempting CoreWLAN backend for macOS");
    auto backend = std::make_unique<WifiBackendMacOS>();
    WiFiError start_result = backend->start();

    if (start_result.success()) {
        spdlog::info("[WifiBackend] CoreWLAN backend started successfully");
        return backend;
    }

    // Fallback to mock (leave disabled - wizard will enable via toggle)
    spdlog::warn("[WifiBackend] CoreWLAN backend failed: {} - falling back to mock",
                start_result.technical_msg);
    spdlog::info("[WifiBackend] Mock backend created");
    return std::make_unique<WifiBackendMock>();
#else
    // Linux: Try wpa_supplicant backend first, fallback to mock if unavailable
    spdlog::debug("[WifiBackend] Attempting wpa_supplicant backend for Linux");
    auto backend = std::make_unique<WifiBackendWpaSupplicant>();
    WiFiError start_result = backend->start();

    if (start_result.success()) {
        spdlog::info("[WifiBackend] wpa_supplicant backend started successfully");
        return backend;
    }

    // Fallback to mock (leave disabled - wizard will enable via toggle)
    spdlog::warn("[WifiBackend] wpa_supplicant backend failed: {} - falling back to mock",
                start_result.technical_msg);
    spdlog::info("[WifiBackend] Mock backend created");
    return std::make_unique<WifiBackendMock>();
#endif
}