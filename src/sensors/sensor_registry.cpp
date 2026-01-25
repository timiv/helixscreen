// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensor_registry.h"
#include <spdlog/spdlog.h>

namespace helix::sensors {

void SensorRegistry::register_manager(std::string category, std::unique_ptr<ISensorManager> manager) {
    if (!manager) {
        spdlog::warn("[SensorRegistry] Attempted to register null manager for category '{}'", category);
        return;
    }
    spdlog::info("[SensorRegistry] Registering sensor manager: {}", category);
    managers_[std::move(category)] = std::move(manager);
}

ISensorManager* SensorRegistry::get_manager(const std::string& category) const {
    auto it = managers_.find(category);
    if (it != managers_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void SensorRegistry::discover_all(const std::vector<std::string>& klipper_objects) {
    for (auto& [category, manager] : managers_) {
        manager->discover(klipper_objects);
    }
}

void SensorRegistry::update_all_from_status(const nlohmann::json& status) {
    for (auto& [category, manager] : managers_) {
        manager->update_from_status(status);
    }
}

void SensorRegistry::load_config(const nlohmann::json& root_config) {
    if (!root_config.contains("sensors")) {
        return;
    }

    const auto& sensors_config = root_config["sensors"];
    for (auto& [category, manager] : managers_) {
        if (sensors_config.contains(category)) {
            manager->load_config(sensors_config[category]);
        }
    }
}

nlohmann::json SensorRegistry::save_config() const {
    nlohmann::json result;
    nlohmann::json sensors_config;

    for (const auto& [category, manager] : managers_) {
        sensors_config[category] = manager->save_config();
    }

    result["sensors"] = sensors_config;
    return result;
}

}  // namespace helix::sensors
