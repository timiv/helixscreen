// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_tool_mapping_overlay.cpp
 * @brief Implementation of AmsToolMappingOverlay
 */

#include "ui_ams_tool_mapping_overlay.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"

#include "ams_backend.h"
#include "ams_state.h"
#include "static_panel_registry.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::ui {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<AmsToolMappingOverlay> g_ams_tool_mapping_overlay;

AmsToolMappingOverlay& get_ams_tool_mapping_overlay() {
    if (!g_ams_tool_mapping_overlay) {
        g_ams_tool_mapping_overlay = std::make_unique<AmsToolMappingOverlay>();
        StaticPanelRegistry::instance().register_destroy(
            "AmsToolMappingOverlay", []() { g_ams_tool_mapping_overlay.reset(); });
    }
    return *g_ams_tool_mapping_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AmsToolMappingOverlay::AmsToolMappingOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

AmsToolMappingOverlay::~AmsToolMappingOverlay() {
    spdlog::debug("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void AmsToolMappingOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // No subjects needed for this overlay currently
    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void AmsToolMappingOverlay::register_callbacks() {
    // No XML callbacks needed - dropdowns use lv_obj_add_event_cb for dynamic rows
    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* AmsToolMappingOverlay::create(lv_obj_t* parent) {
    if (overlay_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    // Create from XML component
    overlay_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "ams_settings_tool_mapping", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Find the rows container
    rows_container_ = lv_obj_find_by_name(overlay_, "tool_rows_container");
    if (!rows_container_) {
        spdlog::error("[{}] Failed to find tool_rows_container", get_name());
    }

    // Find the not supported card
    not_supported_card_ = lv_obj_find_by_name(overlay_, "not_supported_card");

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_;
}

void AmsToolMappingOverlay::show(lv_obj_t* parent_screen) {
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

    // Populate rows from backend
    refresh();

    // Register with NavigationManager for lifecycle callbacks
    NavigationManager::instance().register_overlay_instance(overlay_, this);

    // Push onto navigation stack
    ui_nav_push_overlay(overlay_);
}

void AmsToolMappingOverlay::refresh() {
    if (!overlay_) {
        return;
    }

    clear_rows();
    populate_rows();
}

// ============================================================================
// ROW MANAGEMENT
// ============================================================================

void AmsToolMappingOverlay::clear_rows() {
    for (lv_obj_t* row : tool_rows_) {
        if (row && lv_obj_is_valid(row)) {
            lv_obj_delete(row);
        }
    }
    tool_rows_.clear();
}

void AmsToolMappingOverlay::populate_rows() {
    AmsBackend* backend = AmsState::instance().get_backend();
    if (!backend) {
        spdlog::warn("[{}] No backend available", get_name());
        show_not_supported();
        return;
    }

    auto caps = backend->get_tool_mapping_capabilities();
    if (!caps.supported) {
        spdlog::info("[{}] Tool mapping not supported by backend", get_name());
        show_not_supported();
        return;
    }

    // Get current mapping and system info
    auto mapping = backend->get_tool_mapping();
    auto info = backend->get_system_info();
    int slot_count = info.total_slots;

    if (mapping.empty() || slot_count == 0) {
        spdlog::warn("[{}] Empty tool mapping or zero slots", get_name());
        show_not_supported();
        return;
    }

    // Hide not supported card, show rows container and description
    if (not_supported_card_) {
        lv_obj_add_flag(not_supported_card_, LV_OBJ_FLAG_HIDDEN);
    }
    if (rows_container_) {
        lv_obj_remove_flag(rows_container_, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t* description = lv_obj_find_by_name(overlay_, "description");
    if (description) {
        lv_obj_remove_flag(description, LV_OBJ_FLAG_HIDDEN);
    }

    spdlog::debug("[{}] Creating {} tool rows with {} slots", get_name(), mapping.size(),
                  slot_count);

    // Create a row for each tool
    for (size_t tool_idx = 0; tool_idx < mapping.size(); ++tool_idx) {
        int current_slot = mapping[tool_idx];
        lv_obj_t* row =
            create_tool_row(static_cast<int>(tool_idx), current_slot, slot_count, backend);
        if (row) {
            tool_rows_.push_back(row);
        }
    }
}

lv_obj_t* AmsToolMappingOverlay::create_tool_row(int tool_index, int current_slot, int slot_count,
                                                 AmsBackend* backend) {
    if (!rows_container_) {
        return nullptr;
    }

    // Create row container (card-style)
    lv_obj_t* row = lv_obj_create(rows_container_);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(row, theme_manager_get_color("card_bg"), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(row, theme_manager_get_spacing("border_radius"), LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, theme_manager_get_spacing("space_md"), LV_PART_MAIN);
    lv_obj_set_style_pad_gap(row, theme_manager_get_spacing("space_sm"), LV_PART_MAIN);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Store tool index in user_data for callback
    lv_obj_set_user_data(row, reinterpret_cast<void*>(static_cast<intptr_t>(tool_index)));

    // Tool label (T0, T1, etc.)
    lv_obj_t* label = lv_label_create(row);
    char label_text[8];
    snprintf(label_text, sizeof(label_text), "T%d", tool_index);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, theme_manager_get_color("text_primary"), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, theme_manager_get_font("font_body"), LV_PART_MAIN);
    lv_obj_set_width(label, 40);

    // Color swatch (shows current mapped slot's color)
    lv_obj_t* swatch = lv_obj_create(row);
    lv_obj_set_size(swatch, theme_manager_get_spacing("space_lg"),
                    theme_manager_get_spacing("space_lg"));
    lv_obj_set_style_border_width(swatch, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(swatch, theme_manager_get_color("theme_grey"), LV_PART_MAIN);
    lv_obj_set_style_radius(swatch, theme_manager_get_spacing("space_xs"), LV_PART_MAIN);
    lv_obj_remove_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(swatch, LV_OBJ_FLAG_EVENT_BUBBLE);
    // Set swatch name for later lookup
    lv_obj_set_name(swatch, "color_swatch");

    // Set initial swatch color
    update_row_color_swatch(row, current_slot, backend);

    // Spacer - takes 1 part of flex space
    lv_obj_t* spacer = lv_obj_create(row);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);
    lv_obj_set_height(spacer, 1);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_remove_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    // Dropdown for slot selection - takes 2 parts of flex space
    lv_obj_t* dropdown = lv_dropdown_create(row);

    // Build options string: "Slot 0\nSlot 1\nSlot 2\n..."
    std::string options;
    for (int i = 0; i < slot_count; i++) {
        if (i > 0) {
            options += "\n";
        }

        // Get slot info for label
        auto slot_info = backend->get_slot_info(i);
        if (!slot_info.material.empty()) {
            // Show material name if available
            char slot_label[64];
            snprintf(slot_label, sizeof(slot_label), "Slot %d (%s)", i, slot_info.material.c_str());
            options += slot_label;
        } else {
            options += "Slot " + std::to_string(i);
        }
    }

    lv_dropdown_set_options(dropdown, options.c_str());

    // Set selected to current mapping
    if (current_slot >= 0 && current_slot < slot_count) {
        lv_dropdown_set_selected(dropdown, static_cast<uint32_t>(current_slot));
    }

    // Style the dropdown - flex_grow=2 takes 2/3 of flexible space (spacer has 1)
    lv_obj_set_flex_grow(dropdown, 2);
    lv_obj_set_style_text_font(dropdown, theme_manager_get_font("font_body"), LV_PART_MAIN);

    // Register change callback - use lv_obj_add_event_cb for dynamic widgets
    // (This is acceptable per CLAUDE.md exceptions for dynamic UI)
    lv_obj_add_event_cb(dropdown, on_slot_dropdown_changed, LV_EVENT_VALUE_CHANGED, row);

    return row;
}

void AmsToolMappingOverlay::update_row_color_swatch(lv_obj_t* row, int slot_index,
                                                    AmsBackend* backend) {
    if (!row || !backend) {
        return;
    }

    lv_obj_t* swatch = lv_obj_find_by_name(row, "color_swatch");
    if (!swatch) {
        return;
    }

    // Validate slot_index against actual slot count
    auto info = backend->get_system_info();
    if (slot_index >= 0 && slot_index < info.total_slots) {
        auto slot_info = backend->get_slot_info(slot_index);
        lv_color_t color = lv_color_hex(slot_info.color_rgb);
        lv_obj_set_style_bg_color(swatch, color, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        // No valid slot - show gray
        lv_obj_set_style_bg_color(swatch, theme_manager_get_color("text_secondary"), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(swatch, LV_OPA_50, LV_PART_MAIN);
    }
}

void AmsToolMappingOverlay::show_not_supported() {
    if (rows_container_) {
        lv_obj_add_flag(rows_container_, LV_OBJ_FLAG_HIDDEN);
    }
    // Also hide the description when showing not supported
    lv_obj_t* description = lv_obj_find_by_name(overlay_, "description");
    if (description) {
        lv_obj_add_flag(description, LV_OBJ_FLAG_HIDDEN);
    }
    if (not_supported_card_) {
        lv_obj_remove_flag(not_supported_card_, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void AmsToolMappingOverlay::on_slot_dropdown_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AmsToolMappingOverlay] on_slot_dropdown_changed");

    auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* row = static_cast<lv_obj_t*>(lv_event_get_user_data(e));

    // Validate both objects are still valid (handles refresh during callback)
    if (!dropdown || !lv_obj_is_valid(dropdown) || !row || !lv_obj_is_valid(row)) {
        spdlog::warn("[AmsToolMappingOverlay] Stale callback - objects no longer valid");
    } else {
        // Get tool index from row's user_data
        int tool_index = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(row)));

        // Get selected slot from dropdown
        int selected_slot = static_cast<int>(lv_dropdown_get_selected(dropdown));

        spdlog::info("[AmsToolMappingOverlay] Tool T{} -> Slot {}", tool_index, selected_slot);

        // Call backend to set the mapping
        AmsBackend* backend = AmsState::instance().get_backend();
        if (backend) {
            auto result = backend->set_tool_mapping(tool_index, selected_slot);
            if (result.success()) {
                spdlog::info("[AmsToolMappingOverlay] Tool mapping updated: T{} -> Slot {}",
                             tool_index, selected_slot);

                // Update the color swatch
                get_ams_tool_mapping_overlay().update_row_color_swatch(row, selected_slot, backend);
            } else {
                spdlog::error("[AmsToolMappingOverlay] Failed to set tool mapping: {}",
                              result.user_msg);
            }
        }
    }

    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::ui
