// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "moonraker_client_mock_internal.h"

#include <spdlog/spdlog.h>

#include <chrono>

namespace mock_internal {

void register_history_handlers(std::unordered_map<std::string, MethodHandler>& registry) {
    // server.history.list - Get print history
    registry["server.history.list"] =
        [](MoonrakerClientMock* self, const json& params, std::function<void(json)> success_cb,
           std::function<void(const MoonrakerError&)> error_cb) -> bool {
        (void)self;
        (void)error_cb;

        // Generate 20 mock jobs spread across last 30 days
        json jobs = json::array();
        auto now = std::chrono::system_clock::now();

        // Mock job data - realistic filenames and parameters
        struct MockJob {
            const char* filename;
            const char* status;
            const char* filament_type;
            int duration_minutes;
            int filament_mm;
        };

        // 70% completed, 15% cancelled, 15% failed
        // PLA 50%, PETG 25%, ABS 15%, TPU 10%
        std::vector<MockJob> mock_data = {
            {"3DBenchy.gcode", "completed", "PLA", 95, 4500},
            {"calibration_cube.gcode", "completed", "PLA", 45, 2100},
            {"phone_stand_v2.gcode", "completed", "PETG", 180, 28000},
            {"vase_mode_lamp.gcode", "completed", "PLA", 240, 35000},
            {"cable_clip_x10.gcode", "completed", "PLA", 60, 3200},
            {"raspberry_pi_case.gcode", "completed", "PETG", 150, 22000},
            {"gear_bearing.gcode", "cancelled", "PLA", 30, 1500},
            {"lithophane_photo.gcode", "completed", "PLA", 300, 15000},
            {"keyboard_keycap.gcode", "completed", "ABS", 45, 1800},
            {"flexi_rex.gcode", "completed", "TPU", 120, 8500},
            {"headphone_hook.gcode", "completed", "PETG", 90, 12000},
            {"filament_sample.gcode", "error", "ABS", 15, 500},
            {"temp_tower.gcode", "completed", "PLA", 75, 4200},
            {"retraction_test.gcode", "completed", "PLA", 30, 1100},
            {"bracket_mount.gcode", "cancelled", "PETG", 45, 5500},
            {"desk_organizer.gcode", "completed", "PLA", 210, 42000},
            {"cable_chain_x20.gcode", "error", "PLA", 25, 2800},
            {"fan_duct.gcode", "completed", "ABS", 55, 3800},
            {"filament_dryer_spool.gcode", "cancelled", "PETG", 180, 45000},
            {"pen_holder.gcode", "completed", "TPU", 65, 4200},
        };

        int limit = params.value("limit", 50);
        int start = params.value("start", 0);
        double since = params.value("since", 0.0);

        int count = 0;
        for (size_t i = 0; i < mock_data.size() && count < limit; i++) {
            if (static_cast<int>(i) < start)
                continue;

            const auto& m = mock_data[i];

            // Spread jobs across last 30 days
            auto job_time =
                now - std::chrono::hours(24 * (i + 1)) - std::chrono::minutes(i * 37 % 60);
            double start_time = std::chrono::duration<double>(job_time.time_since_epoch()).count();
            double end_time = start_time + m.duration_minutes * 60;

            // Skip if before 'since' filter
            if (since > 0 && start_time < since)
                continue;

            // Generate thumbnail path from filename (strip .gcode, add thumbnail suffix)
            std::string base_name = std::string(m.filename);
            if (base_name.size() > 6 && base_name.substr(base_name.size() - 6) == ".gcode") {
                base_name = base_name.substr(0, base_name.size() - 6);
            }
            std::string thumb_path = ".thumbnails/" + base_name + "-300x300.png";

            // Generate mock UUID and file size for history matching
            std::string mock_uuid =
                fmt::format("mock-uuid-{:08x}", static_cast<unsigned int>(i * 12345 + 67890));
            size_t mock_file_size =
                static_cast<size_t>(m.filament_mm * 10 + 50000); // Reasonable file size

            json job = {{"job_id", fmt::format("mock_job_{:03d}", i)},
                        {"filename", m.filename},
                        {"status", m.status},
                        {"start_time", start_time},
                        {"end_time", end_time},
                        {"print_duration", m.duration_minutes * 60.0},
                        {"total_duration", m.duration_minutes * 60.0 + 120},
                        {"filament_used", static_cast<double>(m.filament_mm)},
                        {"exists", true},
                        {"metadata",
                         {{"filament_type", m.filament_type},
                          {"layer_count", m.duration_minutes * 2},
                          {"layer_height", 0.2},
                          {"first_layer_extr_temp", 210.0},
                          {"first_layer_bed_temp", 60.0},
                          {"uuid", mock_uuid},
                          {"size", mock_file_size},
                          {"thumbnails", json::array({{{"relative_path", thumb_path},
                                                       {"width", 300},
                                                       {"height", 300},
                                                       {"size", 25000}}})}}}};
            jobs.push_back(job);
            count++;
        }

        if (success_cb) {
            json response = {{"result", {{"count", mock_data.size()}, {"jobs", jobs}}}};
            success_cb(response);
        }
        return true;
    };

    // server.history.totals - Get aggregate statistics
    registry["server.history.totals"] =
        [](MoonrakerClientMock* self, const json& params, std::function<void(json)> success_cb,
           std::function<void(const MoonrakerError&)> error_cb) -> bool {
        (void)self;
        (void)params;
        (void)error_cb;

        // Return exactly what real Moonraker provides - no breakdown counts
        // (breakdown should be calculated client-side from job list if needed)
        json response = {{"result",
                          {{"job_totals",
                            {{"total_jobs", 47},
                             {"total_time", 142.5 * 3600},      // 142.5 hours in seconds
                             {"total_filament_used", 245000.0}, // 245m in mm
                             {"longest_job", 5.5 * 3600}}}}}};  // 5.5 hours in seconds

        if (success_cb) {
            success_cb(response);
        }
        return true;
    };

    // server.history.delete_job - Delete a job from history
    registry["server.history.delete_job"] =
        [](MoonrakerClientMock* self, const json& params, std::function<void(json)> success_cb,
           std::function<void(const MoonrakerError&)> error_cb) -> bool {
        (void)self;
        (void)error_cb;
        std::string job_id = params.value("uid", "");
        spdlog::info("[MoonrakerClientMock] Mock delete history job: {}", job_id);

        if (success_cb) {
            json response = {{"result", {{"deleted_jobs", json::array({job_id})}}}};
            success_cb(response);
        }
        return true;
    };
}

} // namespace mock_internal
