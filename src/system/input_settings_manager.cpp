// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "input_settings_manager.h"

#include "config.h"
#include "spdlog/spdlog.h"
#include "static_subject_registry.h"

#include <algorithm>

using namespace helix;

InputSettingsManager& InputSettingsManager::instance() {
    static InputSettingsManager instance;
    return instance;
}

InputSettingsManager::InputSettingsManager() {
    spdlog::trace("[InputSettingsManager] Constructor");
}

void InputSettingsManager::init_subjects() {
    if (subjects_initialized_) {
        spdlog::debug("[InputSettingsManager] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[InputSettingsManager] Initializing subjects");

    Config* config = Config::get_instance();

    // Scroll throw (default: 25, range 5-50)
    int scroll_throw = config->get<int>("/input/scroll_throw", 25);
    scroll_throw = std::max(5, std::min(50, scroll_throw));
    UI_MANAGED_SUBJECT_INT(scroll_throw_subject_, scroll_throw, "settings_scroll_throw", subjects_);

    // Scroll limit (default: 10, range 1-20)
    int scroll_limit = config->get<int>("/input/scroll_limit", 10);
    scroll_limit = std::max(1, std::min(20, scroll_limit));
    UI_MANAGED_SUBJECT_INT(scroll_limit_subject_, scroll_limit, "settings_scroll_limit", subjects_);

    subjects_initialized_ = true;

    // Self-register cleanup with StaticSubjectRegistry
    StaticSubjectRegistry::instance().register_deinit(
        "InputSettingsManager", []() { InputSettingsManager::instance().deinit_subjects(); });

    spdlog::debug("[InputSettingsManager] Subjects initialized: scroll_throw={}, scroll_limit={}",
                  scroll_throw, scroll_limit);
}

void InputSettingsManager::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::trace("[InputSettingsManager] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
    spdlog::trace("[InputSettingsManager] Subjects deinitialized");
}

// =============================================================================
// GETTERS / SETTERS
// =============================================================================

int InputSettingsManager::get_scroll_throw() const {
    return lv_subject_get_int(const_cast<lv_subject_t*>(&scroll_throw_subject_));
}

void InputSettingsManager::set_scroll_throw(int value) {
    // Clamp to valid range (5-50)
    int clamped = std::max(5, std::min(50, value));
    spdlog::info("[InputSettingsManager] set_scroll_throw({})", clamped);

    // 1. Update subject
    lv_subject_set_int(&scroll_throw_subject_, clamped);

    // 2. Persist
    Config* config = Config::get_instance();
    config->set<int>("/input/scroll_throw", clamped);
    config->save();

    // 3. Mark restart needed (this setting only takes effect on startup)
    restart_pending_ = true;
    spdlog::debug("[InputSettingsManager] Scroll throw set to {} (restart required)", clamped);
}

int InputSettingsManager::get_scroll_limit() const {
    return lv_subject_get_int(const_cast<lv_subject_t*>(&scroll_limit_subject_));
}

void InputSettingsManager::set_scroll_limit(int value) {
    // Clamp to valid range (1-20)
    int clamped = std::max(1, std::min(20, value));
    spdlog::info("[InputSettingsManager] set_scroll_limit({})", clamped);

    // 1. Update subject
    lv_subject_set_int(&scroll_limit_subject_, clamped);

    // 2. Persist
    Config* config = Config::get_instance();
    config->set<int>("/input/scroll_limit", clamped);
    config->save();

    // 3. Mark restart needed (this setting only takes effect on startup)
    restart_pending_ = true;
    spdlog::debug("[InputSettingsManager] Scroll limit set to {} (restart required)", clamped);
}
