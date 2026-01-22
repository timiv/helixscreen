// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_modal.h"

#include "subject_managed_panel.h"

#include <cstdint>
#include <functional>
#include <string>

namespace helix {

/**
 * @brief Map hex color value to human-readable name
 *
 * Uses algorithmic color naming (HSL-based) with special names for
 * preset colors that have non-standard names (Gold, Bronze, Wood, etc.)
 *
 * @param rgb Color as RGB packed uint32_t
 * @return Human-readable color name
 */
std::string get_color_name_from_hex(uint32_t rgb);

} // namespace helix

namespace helix::ui {

/**
 * @file ui_ams_color_picker.h
 * @brief Color picker modal for AMS filament color selection
 *
 * Displays preset swatches and HSV picker for custom colors.
 * Extends ModalBase for RAII lifecycle and backdrop handling.
 *
 * ## Usage:
 * @code
 * helix::ui::AmsColorPicker picker;
 * picker.set_color_callback([](uint32_t rgb, const std::string& name) {
 *     // Handle color selection
 * });
 * picker.show_with_color(parent, initial_color_rgb);
 * @endcode
 */
class AmsColorPicker : public Modal {
  public:
    /**
     * @brief Callback type for color selection
     * @param color_rgb Selected color as RGB packed uint32_t
     * @param color_name Human-readable color name
     */
    using ColorCallback = std::function<void(uint32_t color_rgb, const std::string& color_name)>;

    AmsColorPicker();
    ~AmsColorPicker() override;

    // Non-copyable
    AmsColorPicker(const AmsColorPicker&) = delete;
    AmsColorPicker& operator=(const AmsColorPicker&) = delete;

    // Movable
    AmsColorPicker(AmsColorPicker&& other) noexcept;
    AmsColorPicker& operator=(AmsColorPicker&& other) noexcept;

    /**
     * @brief Show color picker with initial color
     * @param parent Parent screen for the modal
     * @param initial_color Initial color to display (RGB packed)
     * @return true if modal was created successfully
     */
    bool show_with_color(lv_obj_t* parent, uint32_t initial_color);

    /**
     * @brief Set callback for when color is selected
     * @param callback Function to call with selected color
     */
    void set_color_callback(ColorCallback callback);

    /**
     * @brief Set callback for when picker is dismissed (any close - select, cancel, or backdrop)
     * @param callback Function to call on dismiss
     */
    void set_dismiss_callback(std::function<void()> callback);

    // Modal interface
    [[nodiscard]] const char* get_name() const override {
        return "Color Picker";
    }
    [[nodiscard]] const char* component_name() const override {
        return "color_picker";
    }

  protected:
    void on_show() override;
    void on_hide() override;
    void on_cancel() override;

  private:
    // === State ===
    uint32_t selected_color_ = 0x808080;
    ColorCallback color_callback_;
    std::function<void()> dismiss_callback_;

    // === Subjects for XML binding ===
    SubjectManager subjects_;
    lv_subject_t hex_subject_;
    lv_subject_t name_subject_;
    char hex_buf_[16] = {0};
    char name_buf_[64] = {0};
    bool subjects_initialized_ = false;

    // === Observer tracking for cleanup ===
    lv_observer_t* hex_label_observer_ = nullptr;
    lv_observer_t* name_label_observer_ = nullptr;

    // === Internal Methods ===
    void init_subjects();
    void deinit_subjects();
    void update_preview(uint32_t color_rgb, bool from_hsv_picker = false);

    // === Event Handlers (called by static callbacks) ===
    void handle_swatch_clicked(lv_obj_t* swatch);
    void handle_select();

    // === Static Callback Registration ===
    static void register_callbacks();
    static bool callbacks_registered_;

    // === Static Callbacks (traverse widget tree to find modal instance) ===
    static void on_close_cb(lv_event_t* e);
    static void on_swatch_cb(lv_event_t* e);
    static void on_cancel_cb(lv_event_t* e);
    static void on_select_cb(lv_event_t* e);

    /**
     * @brief Find AmsColorPicker instance from event target
     *
     * Traverses parent chain looking for modal root with user_data set.
     * @param e LVGL event
     * @return AmsColorPicker pointer, or nullptr if not found
     */
    static AmsColorPicker* get_instance_from_event(lv_event_t* e);
};

} // namespace helix::ui
