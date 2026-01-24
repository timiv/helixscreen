// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "action_prompt_modal.h"

#include "theme_manager.h"

#include <spdlog/spdlog.h>

namespace helix::ui {

// Static member initialization
bool ActionPromptModal::callbacks_registered_ = false;

// ============================================================================
// Construction / Destruction
// ============================================================================

ActionPromptModal::ActionPromptModal() {
    spdlog::debug("[ActionPromptModal] Constructed");
}

ActionPromptModal::~ActionPromptModal() {
    // Modal destructor will call hide() if visible
    // Note: No spdlog here - logger may be destroyed before us during shutdown [L010]
}

ActionPromptModal::ActionPromptModal(ActionPromptModal&& other) noexcept
    : Modal(std::move(other)), prompt_data_(std::move(other.prompt_data_)),
      gcode_callback_(std::move(other.gcode_callback_)),
      created_buttons_(std::move(other.created_buttons_)),
      created_text_labels_(std::move(other.created_text_labels_)) {}

ActionPromptModal& ActionPromptModal::operator=(ActionPromptModal&& other) noexcept {
    if (this != &other) {
        Modal::operator=(std::move(other));
        prompt_data_ = std::move(other.prompt_data_);
        gcode_callback_ = std::move(other.gcode_callback_);
        created_buttons_ = std::move(other.created_buttons_);
        created_text_labels_ = std::move(other.created_text_labels_);
    }
    return *this;
}

// ============================================================================
// Public API
// ============================================================================

void ActionPromptModal::set_gcode_callback(GcodeCallback callback) {
    gcode_callback_ = std::move(callback);
}

bool ActionPromptModal::show_prompt(lv_obj_t* parent, const PromptData& data) {
    // Register callbacks once (idempotent) - BEFORE creating XML [L013]
    register_callbacks();

    // Store prompt data
    prompt_data_ = data;

    // Show the modal via Modal base class
    if (!Modal::show(parent)) {
        return false;
    }

    // Store 'this' in modal's user_data for callback traversal
    lv_obj_set_user_data(dialog_, this);

    spdlog::info("[ActionPromptModal] Shown with title: {}", prompt_data_.title);
    return true;
}

// ============================================================================
// Modal Hooks
// ============================================================================

void ActionPromptModal::on_show() {
    populate_content();
}

void ActionPromptModal::on_hide() {
    clear_dynamic_content();
    spdlog::debug("[ActionPromptModal] on_hide()");
}

// ============================================================================
// Content Population
// ============================================================================

void ActionPromptModal::populate_content() {
    if (!dialog_) {
        return;
    }

    // Set title
    lv_obj_t* title_label = find_widget("title");
    if (title_label) {
        lv_label_set_text(title_label, prompt_data_.title.c_str());
    }

    // Create text lines
    create_text_lines();

    // Create buttons
    create_buttons();
}

void ActionPromptModal::create_text_lines() {
    lv_obj_t* content_container = find_widget("content_container");
    if (!content_container) {
        spdlog::warn("[ActionPromptModal] content_container not found");
        return;
    }

    // Create a label for each text line
    for (const auto& line : prompt_data_.text_lines) {
        lv_obj_t* label = lv_label_create(content_container);
        lv_label_set_text(label, line.c_str());
        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

        // Apply body text styling
        lv_obj_set_style_text_font(label, theme_manager_get_font("font_body"), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, theme_manager_get_color("text_primary"), LV_PART_MAIN);

        created_text_labels_.push_back(label);
    }

    // Hide content container if no text lines
    if (prompt_data_.text_lines.empty()) {
        lv_obj_add_flag(content_container, LV_OBJ_FLAG_HIDDEN);
    }
}

void ActionPromptModal::create_buttons() {
    lv_obj_t* button_container = find_widget("button_container");
    lv_obj_t* footer_container = find_widget("footer_container");
    lv_obj_t* footer_divider = find_widget("footer_divider");

    if (!button_container) {
        spdlog::warn("[ActionPromptModal] button_container not found");
        return;
    }

    bool has_footer_buttons = false;
    bool has_regular_buttons = false;

    // Create buttons based on PromptData
    for (const auto& btn : prompt_data_.buttons) {
        if (btn.is_footer) {
            if (footer_container) {
                create_button(btn, footer_container);
                has_footer_buttons = true;
            }
        } else {
            create_button(btn, button_container);
            has_regular_buttons = true;
        }
    }

    // Show/hide footer based on whether there are footer buttons
    if (footer_container) {
        if (has_footer_buttons) {
            lv_obj_remove_flag(footer_container, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(footer_container, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (footer_divider) {
        if (has_footer_buttons) {
            lv_obj_remove_flag(footer_divider, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(footer_divider, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide button container if no regular buttons
    if (!has_regular_buttons) {
        lv_obj_add_flag(button_container, LV_OBJ_FLAG_HIDDEN);
    }
}

void ActionPromptModal::create_button(const PromptButton& btn, lv_obj_t* container) {
    lv_obj_t* button = lv_button_create(container);

    // For footer buttons, use flex_grow for equal sizing
    if (btn.is_footer) {
        lv_obj_set_height(button, theme_manager_get_spacing("space_xl") * 2 + 10); // ~button_height
        lv_obj_set_flex_grow(button, 1);
    } else {
        // Regular buttons with padding
        lv_obj_set_size(button, LV_SIZE_CONTENT, theme_manager_get_spacing("space_xl") * 2);
        lv_obj_set_style_pad_left(button, theme_manager_get_spacing("space_lg"), LV_PART_MAIN);
        lv_obj_set_style_pad_right(button, theme_manager_get_spacing("space_lg"), LV_PART_MAIN);
    }

    // Apply button styling
    lv_obj_set_style_radius(button, 8, LV_PART_MAIN); // #border_radius
    lv_obj_set_style_border_width(button, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(button, 0, LV_PART_MAIN);

    // Apply button color based on color hint
    lv_color_t bg_color = get_button_color(btn.color);
    lv_obj_set_style_bg_color(button, bg_color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);

    // Create label inside button
    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, btn.label.c_str());
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, theme_manager_get_font("font_body"), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);

    // Store the gcode in user_data for callback
    // We store a pointer to the string in prompt_data_.buttons, so it remains valid
    const std::string* gcode_ptr = nullptr;
    for (size_t i = 0; i < prompt_data_.buttons.size(); ++i) {
        if (prompt_data_.buttons[i].label == btn.label &&
            prompt_data_.buttons[i].gcode == btn.gcode) {
            gcode_ptr = prompt_data_.buttons[i].gcode.empty() ? &prompt_data_.buttons[i].label
                                                              : &prompt_data_.buttons[i].gcode;
            break;
        }
    }

    lv_obj_set_user_data(button, const_cast<std::string*>(gcode_ptr));

    // Add click callback
    lv_obj_add_event_cb(button, on_button_cb, LV_EVENT_CLICKED, this);

    created_buttons_.push_back(button);

    spdlog::debug("[ActionPromptModal] Created button: {} (gcode: {}, color: {})", btn.label,
                  btn.gcode.empty() ? btn.label : btn.gcode,
                  btn.color.empty() ? "primary" : btn.color);
}

lv_color_t ActionPromptModal::get_button_color(const std::string& color_name) {
    // Map Klipper color hints to design tokens
    if (color_name == "primary" || color_name.empty()) {
        return theme_manager_get_color("primary_color");
    } else if (color_name == "secondary") {
        return theme_manager_get_color("secondary_color");
    } else if (color_name == "info") {
        return theme_manager_get_color("info_color");
    } else if (color_name == "warning") {
        return theme_manager_get_color("warning_color");
    } else if (color_name == "error") {
        return theme_manager_get_color("error_color");
    }

    // Unknown color - default to primary
    spdlog::debug("[ActionPromptModal] Unknown color '{}', using primary", color_name);
    return theme_manager_get_color("primary_color");
}

void ActionPromptModal::clear_dynamic_content() {
    // Clear created buttons (LVGL handles deletion when parent is deleted,
    // but we clear our tracking vectors)
    created_buttons_.clear();
    created_text_labels_.clear();
}

// ============================================================================
// Event Handler
// ============================================================================

void ActionPromptModal::handle_button_click(const std::string& gcode) {
    spdlog::info("[ActionPromptModal] Button clicked, gcode: {}", gcode);

    // Call the gcode callback if set
    if (gcode_callback_) {
        gcode_callback_(gcode);
    }

    // Close the modal
    hide();
}

// ============================================================================
// Static Callback Registration
// ============================================================================

void ActionPromptModal::register_callbacks() {
    if (callbacks_registered_) {
        return;
    }

    // Note: We use lv_obj_add_event_cb directly for buttons since they're created
    // dynamically. We don't register XML callbacks here.

    callbacks_registered_ = true;
    spdlog::debug("[ActionPromptModal] Callbacks registered");
}

// ============================================================================
// Static Callbacks
// ============================================================================

ActionPromptModal* ActionPromptModal::get_instance_from_event(lv_event_t* e) {
    // For dynamically created buttons, we pass 'this' as user_data to the event
    return static_cast<ActionPromptModal*>(lv_event_get_user_data(e));
}

void ActionPromptModal::on_button_cb(lv_event_t* e) {
    auto* self = get_instance_from_event(e);
    if (!self) {
        spdlog::warn("[ActionPromptModal] Could not find instance from button event");
        return;
    }

    // Get the gcode from the button's user_data
    auto* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* gcode_ptr = static_cast<std::string*>(lv_obj_get_user_data(target));

    if (gcode_ptr) {
        self->handle_button_click(*gcode_ptr);
    } else {
        spdlog::warn("[ActionPromptModal] Button has no gcode attached");
        self->hide();
    }
}

} // namespace helix::ui
