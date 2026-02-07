// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_modal.h"

#include <functional>

/**
 * @file ui_save_z_offset_modal.h
 * @brief Warning dialog for saving Z-offset during print
 *
 * Uses Modal for RAII lifecycle - dialog auto-hides when object is destroyed.
 * SAVE_CONFIG restarts Klipper and will CANCEL any active print!
 * Shows a strong warning with cancel/confirm options.
 *
 * @example
 *   save_z_offset_modal_.set_on_confirm([this]() { execute_save_config(); });
 *   save_z_offset_modal_.show(lv_screen_active());
 */

/**
 * @brief Warning modal for saving Z-offset during print
 *
 * Derives from Modal base class for RAII lifecycle management.
 * Provides callback mechanism for handling user confirmation.
 */
class SaveZOffsetModal : public Modal {
  public:
    using ConfirmCallback = std::function<void()>;

    /**
     * @brief Get human-readable name for logging
     * @return "Save Z-Offset"
     */
    const char* get_name() const override {
        return "Save Z-Offset";
    }

    /**
     * @brief Get XML component name for lv_xml_create()
     * @return "save_z_offset_modal"
     */
    const char* component_name() const override {
        return "save_z_offset_modal";
    }

    /**
     * @brief Set callback to invoke when user confirms save
     * @param cb Callback function (typically executes SAVE_CONFIG)
     */
    void set_on_confirm(ConfirmCallback cb) {
        on_confirm_cb_ = std::move(cb);
    }

  protected:
    /**
     * @brief Called after modal is created and visible
     *
     * Wires up the "Save & Restart" (ok) and "Cancel" buttons.
     */
    void on_show() override {
        wire_ok_button("btn_primary");       // "Save & Restart" button
        wire_cancel_button("btn_secondary"); // "Cancel" button
    }

    /**
     * @brief Called when user clicks "Save & Restart" button
     *
     * Invokes the confirm callback if set, then hides the modal.
     */
    void on_ok() override {
        if (on_confirm_cb_) {
            on_confirm_cb_();
        }
        hide();
    }

  private:
    ConfirmCallback on_confirm_cb_;
};
