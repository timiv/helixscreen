// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_calibration_state.cpp
 * @brief Calibration and configuration state management extracted from PrinterState
 *
 * Manages firmware retraction, manual probe, and motor state subjects.
 * Uses centimillimeters and microns for integer precision with floating-point mm values.
 *
 * Extracted from PrinterState as part of god class decomposition.
 */

#include "printer_calibration_state.h"

#include "unit_conversions.h"

#include <spdlog/spdlog.h>

namespace helix {

void PrinterCalibrationState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterCalibrationState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterCalibrationState] Initializing subjects (register_xml={})",
                  register_xml);

    // Firmware retraction settings (defaults: disabled)
    lv_subject_init_int(&retract_length_, 0);         // 0 = disabled
    lv_subject_init_int(&retract_speed_, 20);         // 20 mm/s default
    lv_subject_init_int(&unretract_extra_length_, 0); // 0mm extra
    lv_subject_init_int(&unretract_speed_, 10);       // 10 mm/s default

    // Manual probe subjects (for Z-offset calibration)
    lv_subject_init_int(&manual_probe_active_, 0);     // 0=inactive, 1=active
    lv_subject_init_int(&manual_probe_z_position_, 0); // Z position in microns

    // Motor enabled state (from idle_timeout.state - defaults to enabled/Ready)
    lv_subject_init_int(&motors_enabled_, 1); // 1=enabled (Ready/Printing), 0=disabled (Idle)

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&retract_length_);
    subjects_.register_subject(&retract_speed_);
    subjects_.register_subject(&unretract_extra_length_);
    subjects_.register_subject(&unretract_speed_);
    subjects_.register_subject(&manual_probe_active_);
    subjects_.register_subject(&manual_probe_z_position_);
    subjects_.register_subject(&motors_enabled_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterCalibrationState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "retract_length", &retract_length_);
        lv_xml_register_subject(NULL, "retract_speed", &retract_speed_);
        lv_xml_register_subject(NULL, "unretract_extra_length", &unretract_extra_length_);
        lv_xml_register_subject(NULL, "unretract_speed", &unretract_speed_);
        lv_xml_register_subject(NULL, "manual_probe_active", &manual_probe_active_);
        lv_xml_register_subject(NULL, "manual_probe_z_position", &manual_probe_z_position_);
        lv_xml_register_subject(NULL, "motors_enabled", &motors_enabled_);
    } else {
        spdlog::debug("[PrinterCalibrationState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterCalibrationState] Subjects initialized successfully");
}

void PrinterCalibrationState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterCalibrationState] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterCalibrationState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterCalibrationState] reset_for_testing: subjects not initialized, "
                      "nothing to reset");
        return;
    }

    spdlog::info(
        "[PrinterCalibrationState] reset_for_testing: Deinitializing subjects to clear observers");

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterCalibrationState::update_from_status(const nlohmann::json& status) {
    // Update manual probe state (for Z-offset calibration)
    // Klipper's manual_probe object is active during PROBE_CALIBRATE and Z_ENDSTOP_CALIBRATE
    if (status.contains("manual_probe")) {
        const auto& mp = status["manual_probe"];

        if (mp.contains("is_active") && mp["is_active"].is_boolean()) {
            bool is_active = mp["is_active"].get<bool>();
            int old_active = lv_subject_get_int(&manual_probe_active_);
            int new_active = is_active ? 1 : 0;

            if (old_active != new_active) {
                lv_subject_set_int(&manual_probe_active_, new_active);
                spdlog::info("[PrinterCalibrationState] Manual probe active: {} -> {}",
                             old_active != 0, is_active);
            }
        }

        if (mp.contains("z_position") && mp["z_position"].is_number()) {
            // Store as microns (multiply by 1000) for integer subject with 0.001mm resolution
            double z_mm = mp["z_position"].get<double>();
            int z_microns = static_cast<int>(z_mm * 1000.0);
            lv_subject_set_int(&manual_probe_z_position_, z_microns);
            spdlog::trace("[PrinterCalibrationState] Manual probe Z: {:.3f}mm", z_mm);
        }
    }

    // Update motor enabled state from idle_timeout
    // idle_timeout.state: "Ready" or "Printing" = motors enabled, "Idle" = motors disabled
    if (status.contains("idle_timeout")) {
        const auto& it = status["idle_timeout"];

        if (it.contains("state") && it["state"].is_string()) {
            std::string timeout_state = it["state"].get<std::string>();
            // Motors are enabled when state is "Ready" or "Printing", disabled when "Idle"
            int new_enabled = (timeout_state == "Ready" || timeout_state == "Printing") ? 1 : 0;
            int old_enabled = lv_subject_get_int(&motors_enabled_);

            if (old_enabled != new_enabled) {
                lv_subject_set_int(&motors_enabled_, new_enabled);
                spdlog::info("[PrinterCalibrationState] Motors {}: idle_timeout.state='{}'",
                             new_enabled ? "enabled" : "disabled", timeout_state);
            }
        }
    }

    // Parse firmware_retraction settings (G10/G11 retraction parameters)
    if (status.contains("firmware_retraction")) {
        const auto& fr = status["firmware_retraction"];

        if (fr.contains("retract_length") && fr["retract_length"].is_number()) {
            // Store as centimillimeters (x100) to preserve 0.01mm precision
            int centimm = helix::units::json_to_centimm(fr, "retract_length");
            lv_subject_set_int(&retract_length_, centimm);
            spdlog::trace("[PrinterCalibrationState] Retract length: {:.2f}mm",
                          helix::units::from_centimm(centimm));
        }

        if (fr.contains("retract_speed") && fr["retract_speed"].is_number()) {
            int speed = static_cast<int>(fr["retract_speed"].get<double>());
            lv_subject_set_int(&retract_speed_, speed);
            spdlog::trace("[PrinterCalibrationState] Retract speed: {}mm/s", speed);
        }

        if (fr.contains("unretract_extra_length") && fr["unretract_extra_length"].is_number()) {
            int centimm = helix::units::json_to_centimm(fr, "unretract_extra_length");
            lv_subject_set_int(&unretract_extra_length_, centimm);
            spdlog::trace("[PrinterCalibrationState] Unretract extra: {:.2f}mm",
                          helix::units::from_centimm(centimm));
        }

        if (fr.contains("unretract_speed") && fr["unretract_speed"].is_number()) {
            int speed = static_cast<int>(fr["unretract_speed"].get<double>());
            lv_subject_set_int(&unretract_speed_, speed);
            spdlog::trace("[PrinterCalibrationState] Unretract speed: {}mm/s", speed);
        }
    }
}

} // namespace helix
