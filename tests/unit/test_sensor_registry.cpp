// SPDX-License-Identifier: GPL-3.0-or-later
#include "../catch_amalgamated.hpp"
#include "sensor_registry.h"

using namespace helix::sensors;

// Mock sensor manager for testing
class MockSensorManager : public ISensorManager {
public:
    std::string name_;
    bool discovered_ = false;
    bool status_updated_ = false;
    nlohmann::json last_status_;

    explicit MockSensorManager(std::string name) : name_(std::move(name)) {}

    std::string category_name() const override { return name_; }

    void discover(const std::vector<std::string>& objects) override {
        discovered_ = true;
    }

    void update_from_status(const nlohmann::json& status) override {
        status_updated_ = true;
        last_status_ = status;
    }

    void load_config(const nlohmann::json& config) override {}
    nlohmann::json save_config() const override { return {}; }
};

TEST_CASE("SensorRegistry registers managers", "[sensors]") {
    SensorRegistry registry;

    auto mock = std::make_unique<MockSensorManager>("test");
    auto* mock_ptr = mock.get();

    registry.register_manager("test", std::move(mock));

    REQUIRE(registry.get_manager("test") == mock_ptr);
    REQUIRE(registry.get_manager("nonexistent") == nullptr);
}

TEST_CASE("SensorRegistry routes discover to all managers", "[sensors]") {
    SensorRegistry registry;

    auto mock1 = std::make_unique<MockSensorManager>("sensor1");
    auto mock2 = std::make_unique<MockSensorManager>("sensor2");
    auto* ptr1 = mock1.get();
    auto* ptr2 = mock2.get();

    registry.register_manager("sensor1", std::move(mock1));
    registry.register_manager("sensor2", std::move(mock2));

    std::vector<std::string> objects = {"filament_switch_sensor foo"};
    registry.discover_all(objects);

    REQUIRE(ptr1->discovered_);
    REQUIRE(ptr2->discovered_);
}

TEST_CASE("SensorRegistry routes status updates to all managers", "[sensors]") {
    SensorRegistry registry;

    auto mock = std::make_unique<MockSensorManager>("test");
    auto* mock_ptr = mock.get();

    registry.register_manager("test", std::move(mock));

    nlohmann::json status = {{"filament_switch_sensor foo", {{"filament_detected", true}}}};
    registry.update_all_from_status(status);

    REQUIRE(mock_ptr->status_updated_);
    REQUIRE(mock_ptr->last_status_ == status);
}
