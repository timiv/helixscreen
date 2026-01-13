// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_plugin_status_state.cpp
 * @brief HelixPrint plugin status management extracted from PrinterState
 *
 * Manages plugin installation and phase tracking subjects for UI feature gating.
 * Uses tri-state semantics (-1=unknown, 0=no, 1=yes) to distinguish between
 * "still checking" and "definitely not available" states.
 *
 * Extracted from PrinterState as part of god class decomposition.
 */

#include "printer_plugin_status_state.h"

#include "async_helpers.h"

#include <spdlog/spdlog.h>

namespace helix {

void PrinterPluginStatusState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterPluginStatusState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterPluginStatusState] Initializing subjects (register_xml={})",
                  register_xml);

    // Plugin status subjects use tri-state: -1=unknown, 0=no, 1=yes
    // Unknown state allows UI to show "checking..." vs "not available"
    lv_subject_init_int(&helix_plugin_installed_, -1);
    lv_subject_init_int(&phase_tracking_enabled_, -1);

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&helix_plugin_installed_);
    subjects_.register_subject(&phase_tracking_enabled_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterPluginStatusState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "helix_plugin_installed", &helix_plugin_installed_);
        lv_xml_register_subject(NULL, "phase_tracking_enabled", &phase_tracking_enabled_);
    } else {
        spdlog::debug("[PrinterPluginStatusState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterPluginStatusState] Subjects initialized successfully");
}

void PrinterPluginStatusState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterPluginStatusState] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterPluginStatusState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterPluginStatusState] reset_for_testing: subjects not initialized, "
                      "nothing to reset");
        return;
    }

    spdlog::info(
        "[PrinterPluginStatusState] reset_for_testing: Deinitializing subjects to clear observers");

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterPluginStatusState::set_installed_sync(bool installed) {
    // Synchronous update - caller must ensure this runs on UI thread
    // PrinterState wraps this in helix::async::invoke() and calls
    // update_gcode_modification_visibility() afterward
    lv_subject_set_int(&helix_plugin_installed_, installed ? 1 : 0);
    spdlog::info("[PrinterPluginStatusState] HelixPrint plugin installed: {}", installed);
}

void PrinterPluginStatusState::set_phase_tracking_enabled(bool enabled) {
    // Thread-safe: Use helix::async::invoke to update LVGL subject from any thread
    helix::async::invoke([this, enabled]() {
        lv_subject_set_int(&phase_tracking_enabled_, enabled ? 1 : 0);
        spdlog::info("[PrinterPluginStatusState] Phase tracking enabled: {}", enabled);
    });
}

} // namespace helix
