// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ams_backend.h"

#include "ams_backend_afc.h"
#include "ams_backend_happy_hare.h"
#include "ams_backend_mock.h"
#include "ams_backend_valgace.h"
#include "moonraker_api.h"
#include "runtime_config.h"

#include <spdlog/spdlog.h>

#include <cstdlib>

// Helper to create mock backend with optional features
static std::unique_ptr<AmsBackendMock> create_mock_with_features(int gate_count) {
    auto mock = std::make_unique<AmsBackendMock>(gate_count);

    // Enable mock dryer if requested via environment variable
    const char* dryer_env = std::getenv("HELIX_MOCK_DRYER");
    if (dryer_env && (std::string(dryer_env) == "1" || std::string(dryer_env) == "true")) {
        mock->set_dryer_enabled(true);
        spdlog::info("[AMS Backend] Mock dryer enabled via HELIX_MOCK_DRYER");
    }

    // Enable realistic multi-phase operations if requested
    const char* realistic_env = std::getenv("HELIX_MOCK_AMS_REALISTIC");
    if (realistic_env &&
        (std::string(realistic_env) == "1" || std::string(realistic_env) == "true")) {
        mock->set_realistic_mode(true);
        spdlog::info("[AMS Backend] Mock realistic mode enabled via HELIX_MOCK_AMS_REALISTIC");
    }

    return mock;
}

std::unique_ptr<AmsBackend> AmsBackend::create(AmsType detected_type) {
    const auto& config = get_runtime_config();

    // Check if mock mode is requested
    if (config.should_mock_ams()) {
        spdlog::debug("[AMS Backend] Creating mock backend with {} gates (mock mode enabled)",
                      config.mock_ams_gate_count);
        return create_mock_with_features(config.mock_ams_gate_count);
    }

    // Without API/client dependencies, we can only return mock backends
    switch (detected_type) {
    case AmsType::HAPPY_HARE:
        spdlog::warn("[AMS Backend] Happy Hare detected but no API/client provided - using mock");
        return std::make_unique<AmsBackendMock>(config.mock_ams_gate_count);

    case AmsType::AFC:
        spdlog::warn("[AMS Backend] AFC detected but no API/client provided - using mock");
        return std::make_unique<AmsBackendMock>(config.mock_ams_gate_count);

    case AmsType::VALGACE:
        spdlog::warn("[AMS Backend] ValgACE detected but no API/client provided - using mock");
        return std::make_unique<AmsBackendMock>(config.mock_ams_gate_count);

    case AmsType::NONE:
    default:
        spdlog::debug("[AMS Backend] No AMS detected");
        return nullptr;
    }
}

std::unique_ptr<AmsBackend> AmsBackend::create(AmsType detected_type, MoonrakerAPI* api,
                                               MoonrakerClient* client) {
    const auto& config = get_runtime_config();

    // Check if mock mode is requested
    if (config.should_mock_ams()) {
        spdlog::debug("[AMS Backend] Creating mock backend with {} gates (mock mode enabled)",
                      config.mock_ams_gate_count);
        return create_mock_with_features(config.mock_ams_gate_count);
    }

    switch (detected_type) {
    case AmsType::HAPPY_HARE:
        if (!api || !client) {
            spdlog::error("[AMS Backend] Happy Hare requires MoonrakerAPI and MoonrakerClient");
            return nullptr;
        }
        spdlog::debug("[AMS Backend] Creating Happy Hare backend");
        return std::make_unique<AmsBackendHappyHare>(api, client);

    case AmsType::AFC:
        if (!api || !client) {
            spdlog::error("[AMS Backend] AFC requires MoonrakerAPI and MoonrakerClient");
            return nullptr;
        }
        spdlog::debug("[AMS Backend] Creating AFC backend");
        return std::make_unique<AmsBackendAfc>(api, client);

    case AmsType::VALGACE:
        if (!api || !client) {
            spdlog::error("[AMS Backend] ValgACE requires MoonrakerAPI and MoonrakerClient");
            return nullptr;
        }
        spdlog::debug("[AMS Backend] Creating ValgACE backend");
        return std::make_unique<AmsBackendValgACE>(api, client);

    case AmsType::NONE:
    default:
        spdlog::debug("[AMS Backend] No AMS detected");
        return nullptr;
    }
}
