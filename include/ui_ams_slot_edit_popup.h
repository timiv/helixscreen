// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <lvgl.h>
#include <string>

// Forward declaration
class AmsBackend;

namespace helix::ui {

/**
 * @file ui_ams_slot_edit_popup.h
 * @brief Compact popup for AMS slot configuration
 *
 * Displays a small popup (~180px wide) near a tapped slot with:
 * - Header showing "Slot N"
 * - Tool dropdown (T0, T1, T2... or None)
 * - Backup slot dropdown (for endless spool)
 * - Load button (disabled if slot already loaded)
 * - Unload button (disabled if slot not loaded)
 * - Close button
 *
 * Positioned adjacent to the tapped slot widget.
 *
 * ## Usage:
 * @code
 * helix::ui::AmsSlotEditPopup popup;
 * popup.set_load_callback([](int slot) { backend->load_filament(slot); });
 * popup.set_unload_callback([]() { backend->unload_filament(); });
 * popup.show_for_slot(parent, slot_index, slot_widget, backend);
 * @endcode
 */
class AmsSlotEditPopup {
  public:
    using LoadCallback = std::function<void(int slot_index)>;
    using UnloadCallback = std::function<void()>;

    AmsSlotEditPopup();
    ~AmsSlotEditPopup();

    // Non-copyable
    AmsSlotEditPopup(const AmsSlotEditPopup&) = delete;
    AmsSlotEditPopup& operator=(const AmsSlotEditPopup&) = delete;

    // Movable
    AmsSlotEditPopup(AmsSlotEditPopup&& other) noexcept;
    AmsSlotEditPopup& operator=(AmsSlotEditPopup&& other) noexcept;

    /**
     * @brief Show popup near a slot widget
     * @param parent Parent screen for the popup
     * @param slot_index Slot this popup is for (0-based)
     * @param near_widget Widget to position popup near (typically slot widget)
     * @param backend AMS backend for tool/backup configuration
     * @return true if popup was shown successfully
     */
    bool show_for_slot(lv_obj_t* parent, int slot_index, lv_obj_t* near_widget,
                       AmsBackend* backend);

    /**
     * @brief Hide the popup
     */
    void hide();

    /**
     * @brief Check if popup is currently visible
     */
    [[nodiscard]] bool is_visible() const {
        return popup_ != nullptr;
    }

    /**
     * @brief Get slot index the popup is currently shown for
     */
    [[nodiscard]] int get_slot_index() const {
        return slot_index_;
    }

    /**
     * @brief Set callback for load action
     */
    void set_load_callback(LoadCallback callback);

    /**
     * @brief Set callback for unload action
     */
    void set_unload_callback(UnloadCallback callback);

  private:
    // === State ===
    lv_obj_t* popup_ = nullptr;
    lv_obj_t* parent_ = nullptr;
    int slot_index_ = -1;
    AmsBackend* backend_ = nullptr;
    int total_slots_ = 0;

    // === Callbacks ===
    LoadCallback load_callback_;
    UnloadCallback unload_callback_;

    // === Subjects for button states ===
    lv_subject_t can_load_subject_;
    lv_subject_t can_unload_subject_;
    bool subjects_initialized_ = false;

    // === Dropdown widget pointers ===
    lv_obj_t* tool_dropdown_ = nullptr;
    lv_obj_t* backup_dropdown_ = nullptr;

    // === Event Handlers ===
    void handle_backdrop_clicked();
    void handle_close_clicked();
    void handle_load_clicked();
    void handle_unload_clicked();
    void handle_tool_changed();
    void handle_backup_changed();

    // === Dropdown Configuration ===
    void configure_dropdowns();
    void populate_tool_dropdown();
    void populate_backup_dropdown();
    std::string build_tool_options() const;
    std::string build_backup_options() const;
    int get_current_tool_for_slot() const;
    int get_current_backup_for_slot() const;

    // === Position Calculation ===
    void position_popup_near_widget(lv_obj_t* popup_card, lv_obj_t* near_widget);

    // === Static Callback Registration ===
    static void register_callbacks();
    static bool callbacks_registered_;

    // === Static Callbacks ===
    static void on_backdrop_cb(lv_event_t* e);
    static void on_close_cb(lv_event_t* e);
    static void on_load_cb(lv_event_t* e);
    static void on_unload_cb(lv_event_t* e);
    static void on_tool_changed_cb(lv_event_t* e);
    static void on_backup_changed_cb(lv_event_t* e);

    /**
     * @brief Find popup instance from event target
     */
    static AmsSlotEditPopup* get_instance_from_event(lv_event_t* e);
};

} // namespace helix::ui
