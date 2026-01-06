// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime_config.h"

#include "ams_state.h"
#include "app_globals.h"

#include <spdlog/spdlog.h>

#include <cstdlib>

// Global runtime configuration instance
static RuntimeConfig g_runtime_config;

RuntimeConfig* get_runtime_config() {
    return &g_runtime_config;
}

bool RuntimeConfig::should_show_runout_modal() const {
    // If explicitly forced via env var, always show
    if (std::getenv("HELIX_FORCE_RUNOUT_MODAL") != nullptr) {
        return true;
    }

    // Suppress during wizard setup
    if (is_wizard_active()) {
        spdlog::debug("[RuntimeConfig] Suppressing runout modal - wizard active");
        return false;
    }

    // Check AMS state
    auto& ams = AmsState::instance();
    if (ams.is_available()) {
        // AMS present - check bypass state
        // bypass_active=1: external spool (show modal - toolhead sensor matters)
        // bypass_active=0: AMS managing filament (suppress - runout during swaps normal)
        int bypass_active = lv_subject_get_int(ams.get_bypass_active_subject());
        if (bypass_active == 0) {
            spdlog::debug("[RuntimeConfig] Suppressing runout modal - AMS managing filament");
            return false;
        }
        spdlog::debug("[RuntimeConfig] AMS bypass active - showing runout modal");
    }

    // No AMS or AMS with bypass active - show modal
    return true;
}
