// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui_panel_bed_mesh.h"

#include "ui_nav.h"

#include "app_globals.h"
#include "bed_mesh_renderer.h"
#include "moonraker_client.h"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstring>
#include <limits>

#include "hv/json.hpp"

using json = nlohmann::json;

// Canvas dimensions (600×400 RGB888)
#define CANVAS_WIDTH 600
#define CANVAS_HEIGHT 400
#define CANVAS_BUFFER_SIZE (CANVAS_WIDTH * CANVAS_HEIGHT * 3)

// Rotation angle ranges
#define ROTATION_X_MIN (-85)
#define ROTATION_X_MAX (-10)
#define ROTATION_X_DEFAULT (-45)
#define ROTATION_Z_MIN 0
#define ROTATION_Z_MAX 360
#define ROTATION_Z_DEFAULT 45

// Static state
static bed_mesh_renderer_t* renderer = nullptr;
static lv_obj_t* canvas = nullptr;
static lv_obj_t* mesh_info_label = nullptr;
static lv_obj_t* rotation_x_label = nullptr;
static lv_obj_t* rotation_y_label = nullptr;
static lv_obj_t* rotation_x_slider = nullptr;
static lv_obj_t* rotation_z_slider = nullptr;
static lv_obj_t* bed_mesh_panel = nullptr;
static lv_obj_t* parent_obj = nullptr;

// Canvas buffer (statically allocated)
static uint8_t canvas_buffer[CANVAS_BUFFER_SIZE];

// Current rotation angles
static int current_rotation_x = ROTATION_X_DEFAULT;
static int current_rotation_z = ROTATION_Z_DEFAULT;

// Reactive subjects for bed mesh data
static lv_subject_t bed_mesh_available;    // 0 = no mesh, 1 = mesh loaded
static lv_subject_t bed_mesh_profile_name; // String: active profile name
static lv_subject_t bed_mesh_dimensions;   // String: "10x10 points"
static lv_subject_t bed_mesh_z_range;      // String: "Z: 0.05 to 0.35mm"

// String buffers for subjects (LVGL requires persistent buffers)
static char profile_name_buf[64] = "";
static char profile_name_prev_buf[64] = "";
static char dimensions_buf[64] = "No mesh data";
static char dimensions_prev_buf[64] = "";
static char z_range_buf[64] = "";
static char z_range_prev_buf[64] = "";

// Cleanup handler for panel deletion
static void panel_delete_cb(lv_event_t* e) {
    (void)e;

    spdlog::debug("[BedMesh] Panel delete event - cleaning up resources");

    // Destroy renderer
    if (renderer) {
        bed_mesh_renderer_destroy(renderer);
        renderer = nullptr;
    }

    // Clear widget pointers
    canvas = nullptr;
    mesh_info_label = nullptr;
    rotation_x_label = nullptr;
    rotation_y_label = nullptr;
    rotation_x_slider = nullptr;
    rotation_z_slider = nullptr;
    bed_mesh_panel = nullptr;
    parent_obj = nullptr;
}

// Slider event handler: X rotation (tilt)
static void rotation_x_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);

    // Read slider value (0-100)
    int32_t slider_value = lv_slider_get_value(slider);

    // Map to rotation angle range (-85 to -10)
    current_rotation_x = ROTATION_X_MIN + (slider_value * (ROTATION_X_MAX - ROTATION_X_MIN)) / 100;

    // Update label
    if (rotation_x_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Tilt: %d°", current_rotation_x);
        lv_label_set_text(rotation_x_label, buf);
    }

    // Update renderer and redraw
    if (renderer) {
        bed_mesh_renderer_set_rotation(renderer, current_rotation_x, current_rotation_z);
        ui_panel_bed_mesh_redraw();
    }

    spdlog::debug("[BedMesh] X rotation updated: {}°", current_rotation_x);
}

// Slider event handler: Z rotation (spin)
static void rotation_z_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);

    // Read slider value (0-100)
    int32_t slider_value = lv_slider_get_value(slider);

    // Map to rotation angle range (0 to 360)
    current_rotation_z = (slider_value * 360) / 100;

    // Update label
    if (rotation_y_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Spin: %d°", current_rotation_z);
        lv_label_set_text(rotation_y_label, buf);
    }

    // Update renderer and redraw
    if (renderer) {
        bed_mesh_renderer_set_rotation(renderer, current_rotation_x, current_rotation_z);
        ui_panel_bed_mesh_redraw();
    }

    spdlog::debug("[BedMesh] Z rotation updated: {}°", current_rotation_z);
}

// Back button event handler
static void back_button_cb(lv_event_t* e) {
    (void)e;

    // Use navigation history to go back to previous panel
    if (!ui_nav_go_back()) {
        // Fallback: If navigation history is empty, manually hide panel
        if (bed_mesh_panel) {
            lv_obj_add_flag(bed_mesh_panel, LV_OBJ_FLAG_HIDDEN);
        }

        // Show settings panel (typical parent)
        if (parent_obj) {
            lv_obj_t* settings_launcher = lv_obj_find_by_name(parent_obj, "settings_panel");
            if (settings_launcher) {
                lv_obj_clear_flag(settings_launcher, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Create test mesh data (10x10 dome/bowl shape)
static void create_test_mesh_data(std::vector<std::vector<float>>& mesh_data) {
    const int rows = 10;
    const int cols = 10;

    mesh_data.clear();
    mesh_data.resize(rows);

    // Create dome shape: highest in center, lower at edges
    float center_x = cols / 2.0f;
    float center_y = rows / 2.0f;
    float max_radius = std::min(center_x, center_y);

    for (int row = 0; row < rows; row++) {
        mesh_data[row].resize(cols);
        for (int col = 0; col < cols; col++) {
            // Distance from center
            float dx = col - center_x;
            float dy = row - center_y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Dome shape: height decreases with distance from center
            // Z values from 0.0 to 0.5mm (realistic bed mesh range)
            float normalized_dist = dist / max_radius;
            float height = 0.5f * (1.0f - normalized_dist * normalized_dist);

            mesh_data[row][col] = height;
        }
    }

    spdlog::info("[BedMesh] Created test mesh data: {}x{} dome shape", rows, cols);
}

// Update UI subjects when bed mesh data changes
static void on_bed_mesh_update(const MoonrakerClient::BedMeshProfile& mesh) {
    if (mesh.probed_matrix.empty()) {
        lv_subject_set_int(&bed_mesh_available, 0);
        lv_subject_copy_string(&bed_mesh_dimensions, "No mesh data");
        lv_subject_copy_string(&bed_mesh_z_range, "");
        spdlog::warn("[BedMesh] No mesh data available");
        return;
    }

    // Update subjects
    lv_subject_set_int(&bed_mesh_available, 1);

    // Update profile name
    lv_subject_copy_string(&bed_mesh_profile_name, mesh.name.c_str());

    // Format and update dimensions
    snprintf(dimensions_buf, sizeof(dimensions_buf), "%dx%d points", mesh.x_count, mesh.y_count);
    lv_subject_copy_string(&bed_mesh_dimensions, dimensions_buf);

    // Calculate Z range
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const auto& row : mesh.probed_matrix) {
        for (float z : row) {
            min_z = std::min(min_z, z);
            max_z = std::max(max_z, z);
        }
    }

    // Format and update Z range
    snprintf(z_range_buf, sizeof(z_range_buf), "Z: %.3f to %.3f mm", min_z, max_z);
    lv_subject_copy_string(&bed_mesh_z_range, z_range_buf);

    // Update renderer with new mesh data
    ui_panel_bed_mesh_set_data(mesh.probed_matrix);

    spdlog::info("[BedMesh] Mesh updated: {} ({}x{}, Z: {:.3f} to {:.3f})", mesh.name, mesh.x_count,
                 mesh.y_count, min_z, max_z);
}

void ui_panel_bed_mesh_init_subjects() {
    lv_subject_init_int(&bed_mesh_available, 0);
    lv_subject_init_string(&bed_mesh_profile_name, profile_name_buf, profile_name_prev_buf,
                           sizeof(profile_name_buf), "");
    lv_subject_init_string(&bed_mesh_dimensions, dimensions_buf, dimensions_prev_buf,
                           sizeof(dimensions_buf), "No mesh data");
    lv_subject_init_string(&bed_mesh_z_range, z_range_buf, z_range_prev_buf, sizeof(z_range_buf),
                           "");

    spdlog::debug("[BedMesh] Subjects initialized");
}

void ui_panel_bed_mesh_setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    bed_mesh_panel = panel;
    parent_obj = parent_screen;

    spdlog::info("[BedMesh] Setting up event handlers...");

    // Find canvas widget
    canvas = lv_obj_find_by_name(panel, "bed_mesh_canvas");
    if (!canvas) {
        spdlog::error("[BedMesh] Canvas widget not found in XML");
        return;
    }

    // Find info label
    mesh_info_label = lv_obj_find_by_name(panel, "mesh_info_label");
    if (!mesh_info_label) {
        spdlog::warn("[BedMesh] Info label not found in XML");
    }

    // Find rotation labels
    rotation_x_label = lv_obj_find_by_name(panel, "rotation_x_label");
    if (!rotation_x_label) {
        spdlog::warn("[BedMesh] X rotation label not found in XML");
    }

    rotation_y_label = lv_obj_find_by_name(panel, "rotation_y_label");
    if (!rotation_y_label) {
        spdlog::warn("[BedMesh] Z rotation label not found in XML");
    }

    // Find rotation sliders
    rotation_x_slider = lv_obj_find_by_name(panel, "rotation_x_slider");
    if (rotation_x_slider) {
        lv_slider_set_range(rotation_x_slider, 0, 100);
        // Map default angle to slider value: (-45 - (-85)) / ((-10) - (-85)) = 40/75 ≈ 53
        int32_t default_x_value =
            ((ROTATION_X_DEFAULT - ROTATION_X_MIN) * 100) / (ROTATION_X_MAX - ROTATION_X_MIN);
        lv_slider_set_value(rotation_x_slider, default_x_value, LV_ANIM_OFF);
        lv_obj_add_event_cb(rotation_x_slider, rotation_x_slider_cb, LV_EVENT_VALUE_CHANGED,
                            nullptr);
        spdlog::debug("[BedMesh] X rotation slider configured (default: {})", default_x_value);
    } else {
        spdlog::warn("[BedMesh] X rotation slider not found in XML");
    }

    rotation_z_slider = lv_obj_find_by_name(panel, "rotation_z_slider");
    if (rotation_z_slider) {
        lv_slider_set_range(rotation_z_slider, 0, 100);
        // Map default angle to slider value: 45 / 360 * 100 ≈ 12.5
        int32_t default_z_value = (ROTATION_Z_DEFAULT * 100) / 360;
        lv_slider_set_value(rotation_z_slider, default_z_value, LV_ANIM_OFF);
        lv_obj_add_event_cb(rotation_z_slider, rotation_z_slider_cb, LV_EVENT_VALUE_CHANGED,
                            nullptr);
        spdlog::debug("[BedMesh] Z rotation slider configured (default: {})", default_z_value);
    } else {
        spdlog::warn("[BedMesh] Z rotation slider not found in XML");
    }

    // Find and setup back button
    lv_obj_t* back_btn = lv_obj_find_by_name(panel, "back_button");
    if (back_btn) {
        lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, nullptr);
        spdlog::debug("[BedMesh] Back button configured");
    } else {
        spdlog::warn("[BedMesh] Back button not found in XML");
    }

    // Initialize canvas buffer
    lv_canvas_set_buffer(canvas, canvas_buffer, CANVAS_WIDTH, CANVAS_HEIGHT,
                         LV_COLOR_FORMAT_RGB888);
    spdlog::debug("[BedMesh] Canvas buffer allocated: {}x{} RGB888 ({} bytes)", CANVAS_WIDTH,
                  CANVAS_HEIGHT, CANVAS_BUFFER_SIZE);

    // Create renderer
    renderer = bed_mesh_renderer_create();
    if (!renderer) {
        spdlog::error("[BedMesh] Failed to create renderer");
        return;
    }
    spdlog::debug("[BedMesh] Renderer created");

    // Set initial rotation angles
    bed_mesh_renderer_set_rotation(renderer, current_rotation_x, current_rotation_z);

    // Update rotation labels with initial values
    if (rotation_x_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Tilt: %d°", current_rotation_x);
        lv_label_set_text(rotation_x_label, buf);
    }

    if (rotation_y_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Spin: %d°", current_rotation_z);
        lv_label_set_text(rotation_y_label, buf);
    }

    // Register Moonraker callback for bed mesh updates
    MoonrakerClient* client = get_moonraker_client();
    if (client) {
        client->register_notify_update([client](json notification) {
            // Check if this notification contains bed_mesh updates
            if (notification.contains("params") && notification["params"].is_array() &&
                !notification["params"].empty()) {
                const json& params = notification["params"][0];
                if (params.contains("bed_mesh") && params["bed_mesh"].is_object()) {
                    // Mesh data was updated - refresh UI
                    on_bed_mesh_update(client->get_active_bed_mesh());
                }
            }
        });
        spdlog::debug("[BedMesh] Registered Moonraker callback for mesh updates");
    }

    // Load initial mesh data from MoonrakerClient
    if (client && client->has_bed_mesh()) {
        on_bed_mesh_update(client->get_active_bed_mesh());
    } else {
        // Fall back to test mesh if no real data
        spdlog::warn("[BedMesh] No mesh data from Moonraker, using test mesh");
        std::vector<std::vector<float>> test_mesh;
        create_test_mesh_data(test_mesh);
        ui_panel_bed_mesh_set_data(test_mesh);
    }

    // Register cleanup handler
    lv_obj_add_event_cb(panel, panel_delete_cb, LV_EVENT_DELETE, nullptr);

    spdlog::info("[BedMesh] Setup complete!");
}

void ui_panel_bed_mesh_set_data(const std::vector<std::vector<float>>& mesh_data) {
    if (!renderer) {
        spdlog::error("[BedMesh] Cannot set mesh data - renderer not initialized");
        return;
    }

    if (mesh_data.empty() || mesh_data[0].empty()) {
        spdlog::error("[BedMesh] Invalid mesh data - empty rows or columns");
        return;
    }

    int rows = mesh_data.size();
    int cols = mesh_data[0].size();

    // Convert std::vector to C-style array for renderer API
    std::vector<const float*> row_pointers(rows);
    for (int i = 0; i < rows; i++) {
        row_pointers[i] = mesh_data[i].data();
    }

    // Set mesh data in renderer
    if (!bed_mesh_renderer_set_mesh_data(renderer, row_pointers.data(), rows, cols)) {
        spdlog::error("[BedMesh] Failed to set mesh data in renderer");
        return;
    }

    spdlog::info("[BedMesh] Mesh data loaded: {}x{}", rows, cols);

    // Update info label
    if (mesh_info_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Mesh: %dx%d points", rows, cols);
        lv_label_set_text(mesh_info_label, buf);
    }

    // Render the mesh
    ui_panel_bed_mesh_redraw();
}

void ui_panel_bed_mesh_redraw() {
    if (!renderer || !canvas) {
        spdlog::warn("[BedMesh] Cannot redraw - renderer or canvas not initialized");
        return;
    }

    // Clear canvas
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);

    // Render mesh
    if (!bed_mesh_renderer_render(renderer, canvas)) {
        spdlog::error("[BedMesh] Render failed");
        return;
    }

    spdlog::debug("[BedMesh] Render complete");
}
