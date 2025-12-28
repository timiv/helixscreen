// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_panel_base.h"

#include "moonraker_domain_service.h" // For BedMeshProfile

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Bed mesh visualization panel with TinyGL 3D renderer
 *
 * Interactive 3D visualization of printer bed mesh height maps with touch-drag
 * rotation, color-coded height mapping, profile switching, and statistics.
 *
 * Features:
 * - Mainsail-style two-card layout (Current Mesh stats + Profiles list)
 * - Profile management: load, rename, delete, calibrate
 * - SAVE_CONFIG prompt after modifications
 *
 * @see ui_bed_mesh.h for TinyGL widget API
 */

// Maximum number of profiles displayed in UI
constexpr int BED_MESH_MAX_PROFILES = 5;

class BedMeshPanel : public PanelBase {
  public:
    BedMeshPanel(PrinterState& printer_state, MoonrakerAPI* api);
    ~BedMeshPanel() override;

    void init_subjects() override;
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;
    const char* get_name() const override {
        return "Bed Mesh Panel";
    }
    const char* get_xml_component_name() const override {
        return "bed_mesh_panel";
    }

    /**
     * @brief Load mesh data and render
     * @param mesh_data 2D vector of height values (row-major order)
     */
    void set_mesh_data(const std::vector<std::vector<float>>& mesh_data);

    /** @brief Force redraw of bed mesh visualization */
    void redraw();

    // Profile operations (called from XML event callbacks)
    void load_profile(int index);
    void delete_profile(int index);
    void rename_profile(int index);
    void start_calibration();

    // Modal actions
    void show_calibrate_modal();
    void show_rename_modal(const std::string& profile_name);
    void show_delete_confirm_modal(const std::string& profile_name);
    void show_save_config_modal();
    void hide_all_modals();

    // Modal callback action helpers (called from free function callbacks)
    void confirm_delete_profile();
    void decline_save_config();
    void confirm_save_config();
    void start_calibration_with_name(const std::string& profile_name);
    void confirm_rename(const std::string& new_name);

  private:
    // ========== Current Mesh Stats Subjects ==========
    lv_subject_t bed_mesh_available_;
    lv_subject_t bed_mesh_profile_name_;
    lv_subject_t bed_mesh_dimensions_;
    lv_subject_t bed_mesh_max_label_; // "Max [x, y]"
    lv_subject_t bed_mesh_max_value_; // "z mm"
    lv_subject_t bed_mesh_min_label_; // "Min [x, y]"
    lv_subject_t bed_mesh_min_value_; // "z mm"
    lv_subject_t bed_mesh_variance_;

    char profile_name_buf_[64];
    char dimensions_buf_[64];
    char max_label_buf_[48];
    char max_value_buf_[32];
    char min_label_buf_[48];
    char min_value_buf_[32];
    char variance_buf_[64];

    // ========== Profile List Subjects (5 profiles max) ==========
    lv_subject_t bed_mesh_profile_count_;

    std::array<lv_subject_t, BED_MESH_MAX_PROFILES> profile_name_subjects_;
    std::array<lv_subject_t, BED_MESH_MAX_PROFILES> profile_range_subjects_;
    std::array<lv_subject_t, BED_MESH_MAX_PROFILES> profile_active_subjects_;

    std::array<std::array<char, 64>, BED_MESH_MAX_PROFILES> profile_name_bufs_;
    std::array<std::array<char, 32>, BED_MESH_MAX_PROFILES> profile_range_bufs_;

    // Profile names stored for operations
    std::array<std::string, BED_MESH_MAX_PROFILES> profile_names_;

    // ========== Modal State Subjects (NOT visibility - internal state) ==========
    lv_subject_t bed_mesh_calibrating_;     // 0=idle, 1=calibrating (controls form vs spinner)
    lv_subject_t bed_mesh_rename_old_name_; // Display the old name in rename modal

    char rename_old_name_buf_[64];

    // ========== Modal Widget Pointers (uses ui_modal_show pattern) ==========
    lv_obj_t* calibrate_modal_widget_ = nullptr;
    lv_obj_t* rename_modal_widget_ = nullptr;
    lv_obj_t* save_config_modal_widget_ = nullptr;
    lv_obj_t* delete_modal_widget_ = nullptr;

    // ========== UI Widget Pointers ==========
    lv_obj_t* canvas_ = nullptr;
    lv_obj_t* profile_dropdown_ = nullptr;
    lv_obj_t* calibrate_name_input_ = nullptr;
    lv_obj_t* rename_name_input_ = nullptr;

    // ========== State ==========
    std::string pending_delete_profile_;
    std::string pending_rename_old_;
    std::string pending_rename_new_;
    enum class PendingOperation { None, Delete, Rename };
    PendingOperation pending_operation_ = PendingOperation::None;

    // Destruction flag for async callback safety [L012]
    // Shared with WebSocket callbacks to detect when panel is destroyed
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);

    // ========== Private Methods ==========
    void register_event_callbacks();
    void setup_profile_dropdown();
    void setup_moonraker_subscription();
    void on_mesh_update_internal(const BedMeshProfile& mesh);
    void update_profile_list_subjects();
    void update_info_subjects(const std::vector<std::vector<float>>& mesh_data, int cols, int rows);

    // Calculate range (variance) for a profile
    float calculate_profile_range(const std::string& profile_name);

    // Profile operation implementations
    void execute_delete_profile(const std::string& name);
    void execute_rename_profile(const std::string& old_name, const std::string& new_name);
    void execute_calibration(const std::string& profile_name);
    void execute_save_config();

    static void on_panel_delete(lv_event_t* e);
    static void on_profile_dropdown_changed(lv_event_t* e);
};

// Global instance accessor (needed by main.cpp)
BedMeshPanel& get_global_bed_mesh_panel();
