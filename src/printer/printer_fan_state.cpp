// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_fan_state.cpp
 * @brief Fan state management extracted from PrinterState
 *
 * Manages fan subjects including main part-cooling fan speed, multi-fan
 * tracking with per-fan subjects, and fan metadata for UI display.
 * Extracted from PrinterState as part of god class decomposition.
 */

#include "printer_fan_state.h"

#include "device_display_name.h"
#include "unit_conversions.h"

#include <spdlog/spdlog.h>

namespace helix {

void PrinterFanState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterFanState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterFanState] Initializing subjects (register_xml={})", register_xml);

    // Fan subjects
    lv_subject_init_int(&fan_speed_, 0);
    lv_subject_init_int(&fans_version_, 0);

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&fan_speed_);
    subjects_.register_subject(&fans_version_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterFanState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "fan_speed", &fan_speed_);
        lv_xml_register_subject(NULL, "fans_version", &fans_version_);
    } else {
        spdlog::debug("[PrinterFanState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterFanState] Subjects initialized successfully");
}

void PrinterFanState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterFanState] Deinitializing subjects");

    // Deinit per-fan speed subjects (unique_ptr handles memory, we just need to deinit)
    for (auto& [name, subject_ptr] : fan_speed_subjects_) {
        if (subject_ptr) {
            lv_subject_deinit(subject_ptr.get());
        }
    }
    fan_speed_subjects_.clear();

    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterFanState::update_from_status(const nlohmann::json& status) {
    // Update main part-cooling fan speed
    if (status.contains("fan")) {
        const auto& fan = status["fan"];

        if (fan.contains("speed") && fan["speed"].is_number()) {
            int speed_pct = units::json_to_percent(fan, "speed");
            lv_subject_set_int(&fan_speed_, speed_pct);

            // Also update multi-fan tracking
            double speed = fan["speed"].get<double>();
            update_fan_speed("fan", speed);
        }
    }

    // Check for other fan types in the status update
    // Moonraker sends fan objects as top-level keys: "heater_fan hotend_fan", "fan_generic xyz"
    for (const auto& [key, value] : status.items()) {
        // Skip non-fan objects
        if (key.rfind("heater_fan ", 0) == 0 || key.rfind("fan_generic ", 0) == 0 ||
            key.rfind("controller_fan ", 0) == 0) {
            if (value.is_object() && value.contains("speed") && value["speed"].is_number()) {
                double speed = value["speed"].get<double>();
                update_fan_speed(key, speed);
            }
        }
    }
}

void PrinterFanState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug(
            "[PrinterFanState] reset_for_testing: subjects not initialized, nothing to reset");
        return;
    }

    spdlog::info("[PrinterFanState] reset_for_testing: Deinitializing subjects to clear observers");

    // Deinit per-fan speed subjects (unique_ptr handles memory, we just need to deinit)
    for (auto& [name, subject_ptr] : fan_speed_subjects_) {
        if (subject_ptr) {
            lv_subject_deinit(subject_ptr.get());
        }
    }
    fan_speed_subjects_.clear();

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

FanType PrinterFanState::classify_fan_type(const std::string& object_name) {
    if (object_name == "fan") {
        return FanType::PART_COOLING;
    } else if (object_name.rfind("heater_fan ", 0) == 0) {
        return FanType::HEATER_FAN;
    } else if (object_name.rfind("controller_fan ", 0) == 0) {
        return FanType::CONTROLLER_FAN;
    } else {
        return FanType::GENERIC_FAN;
    }
}

bool PrinterFanState::is_fan_controllable(FanType type) {
    return type == FanType::PART_COOLING || type == FanType::GENERIC_FAN;
}

void PrinterFanState::init_fans(const std::vector<std::string>& fan_objects) {
    // Deinit existing per-fan subjects before clearing (unique_ptr handles memory)
    for (auto& [name, subject_ptr] : fan_speed_subjects_) {
        if (subject_ptr) {
            lv_subject_deinit(subject_ptr.get());
        }
    }
    fan_speed_subjects_.clear();

    fans_.clear();
    fans_.reserve(fan_objects.size());

    // Reserve map capacity to prevent rehashing during insertion
    // (unique_ptr makes this less critical, but still good practice)
    fan_speed_subjects_.reserve(fan_objects.size());

    for (const auto& obj_name : fan_objects) {
        FanInfo info;
        info.object_name = obj_name;
        info.display_name = get_display_name(obj_name, DeviceType::FAN);
        info.type = classify_fan_type(obj_name);
        info.is_controllable = is_fan_controllable(info.type);
        info.speed_percent = 0;

        spdlog::debug("[PrinterFanState] Registered fan: {} -> \"{}\" (type={}, controllable={})",
                      obj_name, info.display_name, static_cast<int>(info.type),
                      info.is_controllable);
        fans_.push_back(std::move(info));

        // Create per-fan speed subject for reactive UI updates (heap-allocated to survive rehash)
        auto subject_ptr = std::make_unique<lv_subject_t>();
        lv_subject_init_int(subject_ptr.get(), 0);
        fan_speed_subjects_.emplace(obj_name, std::move(subject_ptr));
        spdlog::debug("[PrinterFanState] Created speed subject for fan: {}", obj_name);
    }

    // Initialize and bump version to notify UI
    lv_subject_set_int(&fans_version_, lv_subject_get_int(&fans_version_) + 1);
    spdlog::info("[PrinterFanState] Initialized {} fans with {} speed subjects (version {})",
                 fans_.size(), fan_speed_subjects_.size(), lv_subject_get_int(&fans_version_));
}

void PrinterFanState::update_fan_speed(const std::string& object_name, double speed) {
    int speed_pct = units::to_percent(speed);

    for (auto& fan : fans_) {
        if (fan.object_name == object_name) {
            if (fan.speed_percent != speed_pct) {
                fan.speed_percent = speed_pct;

                // Fire per-fan subject for reactive UI updates
                auto it = fan_speed_subjects_.find(object_name);
                if (it != fan_speed_subjects_.end() && it->second) {
                    lv_subject_set_int(it->second.get(), speed_pct);
                    spdlog::trace("[PrinterFanState] Fan {} speed updated to {}%", object_name,
                                  speed_pct);
                }
            }
            return;
        }
    }
    // Fan not in list - this is normal during initial status before discovery
}

lv_subject_t* PrinterFanState::get_fan_speed_subject(const std::string& object_name) {
    auto it = fan_speed_subjects_.find(object_name);
    if (it != fan_speed_subjects_.end() && it->second) {
        return it->second.get();
    }
    return nullptr;
}

} // namespace helix
