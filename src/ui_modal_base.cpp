// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_modal_base.h"

#include "ui_event_safety.h"

#include <spdlog/spdlog.h>

#include <utility>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ModalBase::ModalBase() = default;

ModalBase::~ModalBase() {
    // RAII: auto-hide if still visible
    // Note: Cannot call get_name() here as derived class is already destroyed
    if (modal_) {
        spdlog::debug("[ModalBase] Destructor called while visible - hiding");
        // Hide immediately without calling virtual on_hide()
        lv_obj_add_flag(modal_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_delete(modal_);
        modal_ = nullptr;
    }
}

// ============================================================================
// MOVE SEMANTICS
// ============================================================================

ModalBase::ModalBase(ModalBase&& other) noexcept
    : modal_(other.modal_), parent_(other.parent_), alignment_(other.alignment_),
      backdrop_opa_(other.backdrop_opa_), close_on_backdrop_click_(other.close_on_backdrop_click_),
      close_on_esc_(other.close_on_esc_) {
    // Clear source to prevent double-cleanup
    other.modal_ = nullptr;
    other.parent_ = nullptr;
}

ModalBase& ModalBase::operator=(ModalBase&& other) noexcept {
    if (this != &other) {
        // Hide our modal first (direct cleanup, avoid virtual calls in move)
        if (modal_) {
            lv_obj_add_flag(modal_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_delete(modal_);
            modal_ = nullptr;
        }

        // Move state
        modal_ = other.modal_;
        parent_ = other.parent_;
        alignment_ = other.alignment_;
        backdrop_opa_ = other.backdrop_opa_;
        close_on_backdrop_click_ = other.close_on_backdrop_click_;
        close_on_esc_ = other.close_on_esc_;

        // Clear source
        other.modal_ = nullptr;
        other.parent_ = nullptr;
    }
    return *this;
}

// ============================================================================
// CORE LIFECYCLE
// ============================================================================

bool ModalBase::show(lv_obj_t* parent, const char** attrs) {
    if (modal_) {
        spdlog::warn("[{}] show() called while already visible - hiding first", get_name());
        hide();
    }

    parent_ = parent ? parent : lv_screen_active();

    spdlog::info("[{}] Showing modal", get_name());

    // Register event callbacks for XML components
    lv_xml_register_event_cb(nullptr, "on_modal_ok_clicked", ok_button_cb);
    lv_xml_register_event_cb(nullptr, "on_modal_cancel_clicked", cancel_button_cb);

    // Create modal from XML
    modal_ = static_cast<lv_obj_t*>(lv_xml_create(parent_, get_xml_component_name(), attrs));

    if (!modal_) {
        spdlog::error("[{}] Failed to create modal from XML component '{}'", get_name(),
                      get_xml_component_name());
        return false;
    }

    // Apply positioning
    lv_obj_align(modal_, alignment_, 0, 0);

    // Set backdrop opacity (modal root should be full-screen backdrop)
    lv_obj_set_style_bg_opa(modal_, backdrop_opa_, LV_PART_MAIN);

    // Wire up backdrop click handler
    if (close_on_backdrop_click_) {
        lv_obj_add_event_cb(modal_, backdrop_click_cb, LV_EVENT_CLICKED, this);
    }

    // Wire up ESC key handler
    if (close_on_esc_) {
        lv_obj_add_event_cb(modal_, esc_key_cb, LV_EVENT_KEY, this);
        // Make modal focusable to receive key events
        lv_obj_add_flag(modal_, LV_OBJ_FLAG_CLICKABLE);
        lv_group_t* group = lv_group_get_default();
        if (group) {
            lv_group_add_obj(group, modal_);
        }
    }

    // Bring to foreground
    lv_obj_move_foreground(modal_);

    // Call hook for subclass customization
    on_show();

    spdlog::debug("[{}] Modal shown successfully", get_name());
    return true;
}

void ModalBase::hide() {
    if (!modal_) {
        return; // Already hidden, safe to call multiple times
    }

    spdlog::info("[{}] Hiding modal", get_name());

    // Call hook before destruction
    on_hide();

    // Hide immediately for instant visual feedback
    lv_obj_add_flag(modal_, LV_OBJ_FLAG_HIDDEN);

    // Delete the modal (safe even from event callbacks - LVGL defers if needed)
    lv_obj_delete(modal_);
    modal_ = nullptr;

    spdlog::debug("[{}] Modal hidden", get_name());
}

// ============================================================================
// HELPER METHODS
// ============================================================================

lv_obj_t* ModalBase::find_widget(const char* name) {
    if (!modal_ || !name) {
        return nullptr;
    }
    return lv_obj_find_by_name(modal_, name);
}

void ModalBase::wire_ok_button(const char* name) {
    lv_obj_t* btn = find_widget(name);
    if (btn) {
        // Event callback now wired via XML - just set user_data for callback
        lv_obj_set_user_data(btn, this);
        spdlog::trace("[{}] Wired Ok button '{}'", get_name(), name);
    } else {
        spdlog::warn("[{}] Ok button '{}' not found", get_name(), name);
    }
}

void ModalBase::wire_cancel_button(const char* name) {
    lv_obj_t* btn = find_widget(name);
    if (btn) {
        // Event callback now wired via XML - just set user_data for callback
        lv_obj_set_user_data(btn, this);
        spdlog::trace("[{}] Wired Cancel button '{}'", get_name(), name);
    } else {
        spdlog::warn("[{}] Cancel button '{}' not found", get_name(), name);
    }
}

// ============================================================================
// STATIC EVENT HANDLERS
// ============================================================================

void ModalBase::backdrop_click_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[ModalBase] backdrop_click_cb");

    auto* self = static_cast<ModalBase*>(lv_event_get_user_data(e));
    if (self) {
        // Only close if clicking directly on backdrop, not on child widgets
        lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
        lv_obj_t* current_target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));

        if (target == current_target) {
            spdlog::debug("[{}] Backdrop clicked - closing", self->get_name());
            self->hide();
        }
    }

    LVGL_SAFE_EVENT_CB_END();
}

void ModalBase::esc_key_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[ModalBase] esc_key_cb");

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) {
        auto* self = static_cast<ModalBase*>(lv_event_get_user_data(e));
        if (self) {
            spdlog::debug("[{}] ESC key pressed - closing", self->get_name());
            self->on_cancel(); // Use on_cancel() for ESC (allows override)
        }
    }

    LVGL_SAFE_EVENT_CB_END();
}

void ModalBase::ok_button_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[ModalBase] ok_button_cb");

    auto* self = static_cast<ModalBase*>(lv_event_get_user_data(e));
    if (self) {
        spdlog::debug("[{}] Ok button clicked", self->get_name());
        self->on_ok();
    }

    LVGL_SAFE_EVENT_CB_END();
}

void ModalBase::cancel_button_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[ModalBase] cancel_button_cb");

    auto* self = static_cast<ModalBase*>(lv_event_get_user_data(e));
    if (self) {
        spdlog::debug("[{}] Cancel button clicked", self->get_name());
        self->on_cancel();
    }

    LVGL_SAFE_EVENT_CB_END();
}
