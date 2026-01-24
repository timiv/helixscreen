// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_device_actions_overlay.cpp
 * @brief Implementation of AmsDeviceActionsOverlay
 */

#include "ui_ams_device_actions_overlay.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"

#include "ams_backend.h"
#include "ams_state.h"
#include "ams_types.h"
#include "static_panel_registry.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>

namespace helix::ui {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<AmsDeviceActionsOverlay> g_ams_device_actions_overlay;

AmsDeviceActionsOverlay& get_ams_device_actions_overlay() {
    if (!g_ams_device_actions_overlay) {
        g_ams_device_actions_overlay = std::make_unique<AmsDeviceActionsOverlay>();
        StaticPanelRegistry::instance().register_destroy(
            "AmsDeviceActionsOverlay", []() { g_ams_device_actions_overlay.reset(); });
    }
    return *g_ams_device_actions_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AmsDeviceActionsOverlay::AmsDeviceActionsOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

AmsDeviceActionsOverlay::~AmsDeviceActionsOverlay() {
    if (subjects_initialized_ && lv_is_initialized()) {
        lv_subject_deinit(&status_subject_);
    }
    spdlog::debug("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void AmsDeviceActionsOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // Initialize status subject with default text
    snprintf(status_buf_, sizeof(status_buf_), "Ready");
    lv_subject_init_string(&status_subject_, status_buf_, nullptr, sizeof(status_buf_),
                           status_buf_);
    lv_xml_register_subject(nullptr, "ams_device_actions_status", &status_subject_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void AmsDeviceActionsOverlay::register_callbacks() {
    // Register button callbacks
    lv_xml_register_event_cb(nullptr, "on_device_action_clicked", on_action_clicked);
    lv_xml_register_event_cb(nullptr, "on_device_actions_back_clicked", on_back_clicked);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* AmsDeviceActionsOverlay::create(lv_obj_t* parent) {
    if (overlay_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    // Create from XML component
    overlay_ =
        static_cast<lv_obj_t*>(lv_xml_create(parent, "ams_settings_device_actions", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Find sections container for dynamic content
    sections_container_ = lv_obj_find_by_name(overlay_, "sections_container");
    if (!sections_container_) {
        spdlog::warn("[{}] sections_container not found in XML, using overlay root", get_name());
        sections_container_ = overlay_;
    }

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_;
}

void AmsDeviceActionsOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    parent_screen_ = parent_screen;

    // Ensure subjects and callbacks are initialized
    if (!subjects_initialized_) {
        init_subjects();
        register_callbacks();
    }

    // Lazy create overlay
    if (!overlay_ && parent_screen_) {
        create(parent_screen_);
    }

    if (!overlay_) {
        spdlog::error("[{}] Cannot show - overlay not created", get_name());
        return;
    }

    // Refresh content from backend
    refresh();

    // Register with NavigationManager for lifecycle callbacks
    NavigationManager::instance().register_overlay_instance(overlay_, this);

    // Push onto navigation stack
    ui_nav_push_overlay(overlay_);
}

void AmsDeviceActionsOverlay::refresh() {
    if (!overlay_) {
        return;
    }

    spdlog::debug("[{}] Refreshing from backend", get_name());

    // Get backend
    AmsBackend* backend = AmsState::instance().get_backend();
    if (!backend) {
        snprintf(status_buf_, sizeof(status_buf_), "No AMS connected");
        lv_subject_copy_string(&status_subject_, status_buf_);
        clear_sections();
        return;
    }

    // Query sections and actions from backend
    cached_sections_ = backend->get_device_sections();
    cached_actions_ = backend->get_device_actions();

    spdlog::debug("[{}] Got {} sections and {} actions from backend", get_name(),
                  cached_sections_.size(), cached_actions_.size());

    // Clear existing UI
    clear_sections();

    if (cached_sections_.empty()) {
        snprintf(status_buf_, sizeof(status_buf_), "No device actions available");
        lv_subject_copy_string(&status_subject_, status_buf_);
        return;
    }

    // Sort sections by display order
    std::sort(cached_sections_.begin(), cached_sections_.end(),
              [](const auto& a, const auto& b) { return a.display_order < b.display_order; });

    // Create UI for each section (filtered if section_filter_ is set)
    for (const auto& section : cached_sections_) {
        // Apply filter if set
        if (!section_filter_.empty() && section.id != section_filter_) {
            continue;
        }

        // Count actions in this section
        int action_count = 0;
        for (const auto& action : cached_actions_) {
            if (action.section == section.id) {
                action_count++;
            }
        }

        // Skip empty sections
        if (action_count == 0) {
            spdlog::debug("[{}] Skipping empty section: {}", get_name(), section.id);
            continue;
        }

        create_section_ui(sections_container_, section);
    }

    // Update status
    snprintf(status_buf_, sizeof(status_buf_), "Ready");
    lv_subject_copy_string(&status_subject_, status_buf_);
}

void AmsDeviceActionsOverlay::set_filter(const std::string& section_id) {
    section_filter_ = section_id;
    spdlog::debug("[{}] Filter set to: '{}'", get_name(),
                  section_filter_.empty() ? "(all)" : section_filter_.c_str());

    // Refresh if overlay exists
    if (overlay_) {
        refresh();
    }
}

// ============================================================================
// SECTION/ACTION UI CREATION
// ============================================================================

void AmsDeviceActionsOverlay::create_section_ui(lv_obj_t* parent,
                                                const helix::printer::DeviceSection& section) {
    spdlog::debug("[{}] Creating section UI: {} ({})", get_name(), section.label, section.id);

    // Create section card container
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, theme_manager_get_color("card_bg"), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, theme_manager_get_spacing("border_radius"), 0);
    lv_obj_set_style_pad_all(card, theme_manager_get_spacing("space_md"), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Section label
    lv_obj_t* label = lv_label_create(card);
    lv_label_set_text(label, section.label.c_str());
    lv_obj_set_style_text_color(label, theme_manager_get_color("text_primary"), 0);
    lv_obj_set_style_pad_bottom(label, theme_manager_get_spacing("space_sm"), 0);

    // Create actions container
    lv_obj_t* actions_container = lv_obj_create(card);
    lv_obj_set_width(actions_container, LV_PCT(100));
    lv_obj_set_height(actions_container, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(actions_container, 0, 0);
    lv_obj_set_style_bg_opa(actions_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(actions_container, 0, 0);
    lv_obj_set_flex_flow(actions_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(actions_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(actions_container, theme_manager_get_spacing("space_sm"), 0);
    lv_obj_remove_flag(actions_container, LV_OBJ_FLAG_SCROLLABLE);

    // Add actions for this section
    for (const auto& action : cached_actions_) {
        if (action.section == section.id) {
            create_action_control(actions_container, action);
        }
    }
}

void AmsDeviceActionsOverlay::create_action_control(lv_obj_t* parent,
                                                    const helix::printer::DeviceAction& action) {
    spdlog::debug("[{}] Creating action control: {} (type={})", get_name(), action.label,
                  helix::printer::action_type_to_string(action.type));

    // Create row container for action
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(row, theme_manager_get_spacing("space_xs"), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    switch (action.type) {
    case helix::printer::ActionType::BUTTON: {
        // Create action button spanning full width
        lv_obj_t* btn = lv_button_create(row);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, 44);

        // Button label
        lv_obj_t* btn_label = lv_label_create(btn);
        lv_label_set_text(btn_label, action.label.c_str());
        lv_obj_center(btn_label);

        // Store action ID in vector, pass index as user_data
        action_ids_.push_back(action.id);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(action_ids_.size() - 1));

        // Register click callback
        lv_obj_add_event_cb(btn, on_action_clicked, LV_EVENT_CLICKED, nullptr);

        // Handle disabled state
        if (!action.enabled) {
            lv_obj_add_state(btn, LV_STATE_DISABLED);
            if (!action.disable_reason.empty()) {
                spdlog::debug("[{}] Action '{}' disabled: {}", get_name(), action.id,
                              action.disable_reason);
            }
        }
        break;
    }

    case helix::printer::ActionType::TOGGLE: {
        // Label on left
        lv_obj_t* label = lv_label_create(row);
        lv_label_set_text(label, action.label.c_str());
        lv_obj_set_style_text_color(label, theme_manager_get_color("text_primary"), 0);

        // Switch on right (placeholder - full implementation later)
        lv_obj_t* sw = lv_switch_create(row);

        // Try to get current value
        try {
            if (action.current_value.has_value()) {
                bool val = std::any_cast<bool>(action.current_value);
                if (val) {
                    lv_obj_add_state(sw, LV_STATE_CHECKED);
                }
            }
        } catch (const std::bad_any_cast&) {
            spdlog::warn("[{}] Failed to cast toggle value for {}", get_name(), action.id);
        }

        spdlog::debug("[{}] Toggle '{}' created (callback not yet implemented)", get_name(),
                      action.id);
        break;
    }

    case helix::printer::ActionType::INFO: {
        // Label on left
        lv_obj_t* label = lv_label_create(row);
        lv_label_set_text(label, action.label.c_str());
        lv_obj_set_style_text_color(label, theme_manager_get_color("text_primary"), 0);

        // Value on right
        lv_obj_t* value_label = lv_label_create(row);
        lv_obj_set_style_text_color(value_label, theme_manager_get_color("text_secondary"), 0);
        try {
            if (action.current_value.has_value()) {
                std::string val = std::any_cast<std::string>(action.current_value);
                if (!action.unit.empty()) {
                    val += " " + action.unit;
                }
                lv_label_set_text(value_label, val.c_str());
            } else {
                lv_label_set_text(value_label, "-");
            }
        } catch (const std::bad_any_cast&) {
            lv_label_set_text(value_label, "-");
        }
        break;
    }

    case helix::printer::ActionType::SLIDER:
    case helix::printer::ActionType::DROPDOWN:
        // Placeholder for future implementation
        {
            lv_obj_t* label = lv_label_create(row);
            std::string text = action.label + " (coming soon)";
            lv_label_set_text(label, text.c_str());
            lv_obj_set_style_text_color(label, theme_manager_get_color("text_secondary"), 0);
            spdlog::debug("[{}] {} control '{}' placeholder created", get_name(),
                          helix::printer::action_type_to_string(action.type), action.id);
        }
        break;
    }
}

void AmsDeviceActionsOverlay::clear_sections() {
    if (!sections_container_) {
        return;
    }

    // Clear action ID storage
    action_ids_.clear();

    lv_obj_clean(sections_container_);
    spdlog::debug("[{}] Cleared sections", get_name());
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void AmsDeviceActionsOverlay::on_action_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AmsDeviceActionsOverlay] on_action_clicked");

    auto* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!btn || !lv_obj_is_valid(btn)) {
        spdlog::warn("[AmsDeviceActionsOverlay] on_action_clicked: invalid target");
    } else {
        // Get action ID from vector using index stored in user_data
        auto& overlay = get_ams_device_actions_overlay();
        auto index = reinterpret_cast<size_t>(lv_obj_get_user_data(btn));
        if (index >= overlay.action_ids_.size()) {
            spdlog::warn("[AmsDeviceActionsOverlay] Invalid action index: {}", index);
        } else {
            const std::string& action_id = overlay.action_ids_[index];
            spdlog::info("[AmsDeviceActionsOverlay] Action clicked: {}", action_id);

            // Execute via backend
            AmsBackend* backend = AmsState::instance().get_backend();
            if (!backend) {
                spdlog::warn("[AmsDeviceActionsOverlay] No backend available for action");
            } else {
                AmsError result = backend->execute_device_action(action_id);
                if (result.success()) {
                    spdlog::info("[AmsDeviceActionsOverlay] Action '{}' executed successfully",
                                 action_id);
                    // Update status
                    snprintf(overlay.status_buf_, sizeof(overlay.status_buf_), "Executed: %s",
                             action_id.c_str());
                    lv_subject_copy_string(&overlay.status_subject_, overlay.status_buf_);
                } else {
                    spdlog::error("[AmsDeviceActionsOverlay] Action '{}' failed: {}", action_id,
                                  result.technical_msg);
                    // Update status with error
                    snprintf(overlay.status_buf_, sizeof(overlay.status_buf_), "Failed: %s",
                             result.user_msg.c_str());
                    lv_subject_copy_string(&overlay.status_subject_, overlay.status_buf_);
                }
            }
        }
    }

    LVGL_SAFE_EVENT_CB_END();
}

void AmsDeviceActionsOverlay::on_back_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AmsDeviceActionsOverlay] on_back_clicked");
    LV_UNUSED(e);

    spdlog::debug("[AmsDeviceActionsOverlay] Back button clicked");
    ui_nav_go_back();

    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::ui
