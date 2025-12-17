// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

#include <string>
#include <vector>

/**
 * @file ui_modal_base.h
 * @brief Abstract base class for modal dialogs with RAII lifecycle
 *
 * Provides shared infrastructure for modals including:
 * - RAII lifecycle (destructor auto-hides if visible)
 * - Backdrop click-to-close
 * - ESC key handling
 * - Standard Ok/Cancel button wiring
 * - Move semantics support
 *
 * ## Usage Pattern:
 *
 * @code
 * class MyModal : public ModalBase {
 * public:
 *     bool show(lv_obj_t* parent, const std::string& message) {
 *         message_ = message;
 *         const char* attrs[] = {"message", message_.c_str(), nullptr};
 *         return ModalBase::show(parent, attrs);
 *     }
 *
 *     const char* get_name() const override { return "My Modal"; }
 *     const char* get_xml_component_name() const override { return "my_modal"; }
 *
 * protected:
 *     void on_show() override {
 *         wire_ok_button();  // Wires btn_ok to on_ok()
 *     }
 *
 *     void on_ok() override {
 *         // Custom logic before closing
 *         ModalBase::on_ok();  // Calls hide()
 *     }
 *
 * private:
 *     std::string message_;
 * };
 * @endcode
 *
 * ## Lifecycle:
 *
 * 1. Construct modal object (lightweight, no LVGL objects created)
 * 2. Call show() to create and display the modal
 * 3. Modal is visible, user interacts
 * 4. hide() called (via button, backdrop, ESC, or destructor)
 * 5. LVGL objects deleted, modal object can be reused or destroyed
 *
 * @see PanelBase for the panel equivalent
 */
class ModalBase {
  public:
    ModalBase();
    virtual ~ModalBase();

    // Non-copyable (LVGL objects cannot be shared)
    ModalBase(const ModalBase&) = delete;
    ModalBase& operator=(const ModalBase&) = delete;

    // Movable (transfers ownership)
    ModalBase(ModalBase&& other) noexcept;
    ModalBase& operator=(ModalBase&& other) noexcept;

    //
    // === Core Lifecycle ===
    //

    /**
     * @brief Show the modal dialog
     *
     * Creates the modal from XML and displays it. The modal remains visible
     * until hide() is called (via button, backdrop click, ESC, or destructor).
     *
     * @param parent Parent object (usually lv_screen_active())
     * @param attrs Optional XML attributes (NULL-terminated array)
     * @return true if modal was created successfully
     */
    bool show(lv_obj_t* parent, const char** attrs = nullptr);

    /**
     * @brief Hide and destroy the modal
     *
     * Calls on_hide() hook, then deletes LVGL objects. Safe to call
     * multiple times or when not visible.
     */
    void hide();

    /**
     * @brief Check if modal is currently visible
     * @return true if modal is showing
     */
    bool is_visible() const {
        return modal_ != nullptr;
    }

    /**
     * @brief Get the modal's root LVGL object
     * @return Modal object, or nullptr if not visible
     */
    lv_obj_t* get_modal() const {
        return modal_;
    }

    //
    // === Pure Virtual (must implement) ===
    //

    /**
     * @brief Get human-readable modal name for logging
     * @return Modal name (e.g., "Tip Detail Modal")
     */
    virtual const char* get_name() const = 0;

    /**
     * @brief Get XML component name for lv_xml_create()
     * @return Component name (e.g., "tip_detail_dialog")
     */
    virtual const char* get_xml_component_name() const = 0;

    //
    // === Optional Hooks ===
    //

    /**
     * @brief Called after modal is created and visible
     *
     * Override to wire up buttons, set focus, start animations.
     * Default implementation does nothing.
     */
    virtual void on_show() {}

    /**
     * @brief Called before modal is destroyed
     *
     * Override to save state, cleanup resources.
     * Default implementation does nothing.
     */
    virtual void on_hide() {}

    /**
     * @brief Called when Ok/confirm button is clicked
     *
     * Default implementation calls hide().
     * Override to add validation or custom behavior.
     */
    virtual void on_ok() {
        hide();
    }

    /**
     * @brief Called when Cancel button is clicked
     *
     * Default implementation calls hide().
     * Override to add confirmation or custom behavior.
     */
    virtual void on_cancel() {
        hide();
    }

  protected:
    //
    // === Modal State ===
    //

    lv_obj_t* modal_ = nullptr;
    lv_obj_t* parent_ = nullptr;

    //
    // === Configuration (set before show()) ===
    //

    lv_align_t alignment_ = LV_ALIGN_CENTER;
    uint8_t backdrop_opa_ = 200;
    bool close_on_backdrop_click_ = true;
    bool close_on_esc_ = true;

    //
    // === Helper Methods ===
    //

    /**
     * @brief Find a named widget within the modal
     * @param name Widget name attribute
     * @return Widget pointer, or nullptr if not found
     */
    lv_obj_t* find_widget(const char* name);

    /**
     * @brief Wire up an Ok button to call on_ok()
     * @param name Button name (default: "btn_ok")
     */
    void wire_ok_button(const char* name = "btn_ok");

    /**
     * @brief Wire up a Cancel button to call on_cancel()
     * @param name Button name (default: "btn_cancel")
     */
    void wire_cancel_button(const char* name = "btn_cancel");

    //
    // === Public Static Event Handlers (for XML registration) ===
    //

    static void ok_button_cb(lv_event_t* e);
    static void cancel_button_cb(lv_event_t* e);

  private:
    //
    // === Private Static Event Handlers ===
    //

    static void backdrop_click_cb(lv_event_t* e);
    static void esc_key_cb(lv_event_t* e);
};
