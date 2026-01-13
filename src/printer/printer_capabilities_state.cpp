// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_capabilities_state.cpp
 * @brief Printer capabilities state management extracted from PrinterState
 *
 * Manages capability subjects that control UI feature visibility based on
 * hardware detection and user overrides. Extracted from PrinterState as
 * part of god class decomposition.
 */

#include "printer_capabilities_state.h"

#include "async_helpers.h"

#include <spdlog/spdlog.h>

namespace helix {

void PrinterCapabilitiesState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterCapabilitiesState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterCapabilitiesState] Initializing subjects (register_xml={})",
                  register_xml);

    // Printer capability subjects (all default to 0=not available)
    lv_subject_init_int(&printer_has_qgl_, 0);
    lv_subject_init_int(&printer_has_z_tilt_, 0);
    lv_subject_init_int(&printer_has_bed_mesh_, 0);
    lv_subject_init_int(&printer_has_nozzle_clean_, 0);
    lv_subject_init_int(&printer_has_probe_, 0);
    lv_subject_init_int(&printer_has_heater_bed_, 0);
    lv_subject_init_int(&printer_has_led_, 0);
    lv_subject_init_int(&printer_has_accelerometer_, 0);
    lv_subject_init_int(&printer_has_spoolman_, 0);
    lv_subject_init_int(&printer_has_speaker_, 0);
    lv_subject_init_int(&printer_has_timelapse_, 0);
    lv_subject_init_int(&printer_has_purge_line_, 0);
    lv_subject_init_int(&printer_has_firmware_retraction_, 0);
    lv_subject_init_int(&printer_bed_moves_, 0); // 0=gantry moves, 1=bed moves (cartesian)

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&printer_has_qgl_);
    subjects_.register_subject(&printer_has_z_tilt_);
    subjects_.register_subject(&printer_has_bed_mesh_);
    subjects_.register_subject(&printer_has_nozzle_clean_);
    subjects_.register_subject(&printer_has_probe_);
    subjects_.register_subject(&printer_has_heater_bed_);
    subjects_.register_subject(&printer_has_led_);
    subjects_.register_subject(&printer_has_accelerometer_);
    subjects_.register_subject(&printer_has_spoolman_);
    subjects_.register_subject(&printer_has_speaker_);
    subjects_.register_subject(&printer_has_timelapse_);
    subjects_.register_subject(&printer_has_purge_line_);
    subjects_.register_subject(&printer_has_firmware_retraction_);
    subjects_.register_subject(&printer_bed_moves_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterCapabilitiesState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "printer_has_qgl", &printer_has_qgl_);
        lv_xml_register_subject(NULL, "printer_has_z_tilt", &printer_has_z_tilt_);
        lv_xml_register_subject(NULL, "printer_has_bed_mesh", &printer_has_bed_mesh_);
        lv_xml_register_subject(NULL, "printer_has_nozzle_clean", &printer_has_nozzle_clean_);
        lv_xml_register_subject(NULL, "printer_has_probe", &printer_has_probe_);
        lv_xml_register_subject(NULL, "printer_has_heater_bed", &printer_has_heater_bed_);
        lv_xml_register_subject(NULL, "printer_has_led", &printer_has_led_);
        lv_xml_register_subject(NULL, "printer_has_accelerometer", &printer_has_accelerometer_);
        lv_xml_register_subject(NULL, "printer_has_spoolman", &printer_has_spoolman_);
        lv_xml_register_subject(NULL, "printer_has_speaker", &printer_has_speaker_);
        lv_xml_register_subject(NULL, "printer_has_timelapse", &printer_has_timelapse_);
        lv_xml_register_subject(NULL, "printer_has_purge_line", &printer_has_purge_line_);
        lv_xml_register_subject(NULL, "printer_has_firmware_retraction",
                                &printer_has_firmware_retraction_);
        lv_xml_register_subject(NULL, "printer_bed_moves", &printer_bed_moves_);
    } else {
        spdlog::debug("[PrinterCapabilitiesState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterCapabilitiesState] Subjects initialized successfully");
}

void PrinterCapabilitiesState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterCapabilitiesState] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterCapabilitiesState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterCapabilitiesState] reset_for_testing: subjects not initialized, "
                      "nothing to reset");
        return;
    }

    spdlog::info(
        "[PrinterCapabilitiesState] reset_for_testing: Deinitializing subjects to clear observers");

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterCapabilitiesState::set_hardware(const PrinterHardwareDiscovery& hardware,
                                            const CapabilityOverrides& overrides) {
    // Update subjects using effective values (auto-detect + user overrides)
    // This allows users to force-enable features that weren't detected
    // (e.g., heat soak macro without chamber heater) or force-disable
    // features they don't want to see in the UI.
    lv_subject_set_int(&printer_has_qgl_, overrides.has_qgl() ? 1 : 0);
    lv_subject_set_int(&printer_has_z_tilt_, overrides.has_z_tilt() ? 1 : 0);
    lv_subject_set_int(&printer_has_bed_mesh_, overrides.has_bed_mesh() ? 1 : 0);
    lv_subject_set_int(&printer_has_nozzle_clean_, overrides.has_nozzle_clean() ? 1 : 0);

    // Hardware capabilities (no user override support yet - set directly from detection)
    lv_subject_set_int(&printer_has_probe_, hardware.has_probe() ? 1 : 0);
    lv_subject_set_int(&printer_has_heater_bed_, hardware.has_heater_bed() ? 1 : 0);
    lv_subject_set_int(&printer_has_led_, hardware.has_led() ? 1 : 0);
    lv_subject_set_int(&printer_has_accelerometer_, hardware.has_accelerometer() ? 1 : 0);

    // Speaker capability (for M300 audio feedback)
    lv_subject_set_int(&printer_has_speaker_, hardware.has_speaker() ? 1 : 0);

    // Timelapse capability (Moonraker-Timelapse plugin)
    lv_subject_set_int(&printer_has_timelapse_, hardware.has_timelapse() ? 1 : 0);

    // Firmware retraction capability (for G10/G11 retraction settings)
    lv_subject_set_int(&printer_has_firmware_retraction_,
                       hardware.has_firmware_retraction() ? 1 : 0);

    // Spoolman requires async check - default to 0, updated separately via set_spoolman_available()

    spdlog::info("[PrinterCapabilitiesState] Hardware set: probe={}, heater_bed={}, LED={}, "
                 "accelerometer={}, speaker={}, timelapse={}, fw_retraction={}",
                 hardware.has_probe(), hardware.has_heater_bed(), hardware.has_led(),
                 hardware.has_accelerometer(), hardware.has_speaker(), hardware.has_timelapse(),
                 hardware.has_firmware_retraction());
    spdlog::info("[PrinterCapabilitiesState] Hardware set (with overrides): {}",
                 overrides.summary());
}

void PrinterCapabilitiesState::set_spoolman_available(bool available) {
    // Thread-safe: Use helix::async::invoke to update LVGL subject from any thread
    helix::async::invoke([this, available]() {
        lv_subject_set_int(&printer_has_spoolman_, available ? 1 : 0);
        spdlog::info("[PrinterCapabilitiesState] Spoolman availability set: {}", available);
    });
}

void PrinterCapabilitiesState::set_purge_line(bool has_purge_line) {
    lv_subject_set_int(&printer_has_purge_line_, has_purge_line ? 1 : 0);
    spdlog::debug("[PrinterCapabilitiesState] Purge line capability set: {}", has_purge_line);
}

void PrinterCapabilitiesState::set_bed_moves(bool bed_moves) {
    int new_value = bed_moves ? 1 : 0;
    // Only log when value actually changes (this gets called frequently from status updates)
    if (lv_subject_get_int(&printer_bed_moves_) != new_value) {
        lv_subject_set_int(&printer_bed_moves_, new_value);
        spdlog::info("[PrinterCapabilitiesState] Bed moves on Z: {}", bed_moves);
    }
}

} // namespace helix
