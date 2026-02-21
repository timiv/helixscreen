// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ams_types.h"
#include "audio_settings_manager.h"
#include "display_settings_manager.h"
#include "input_settings_manager.h"
#include "lvgl/lvgl.h"
#include "safety_settings_manager.h"
#include "subject_managed_panel.h"
#include "system_settings_manager.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace helix {
class MoonrakerClient;

/** @brief Print completion notification mode (Off=0, Notification=1, Alert=2) */
enum class CompletionAlertMode { OFF = 0, NOTIFICATION = 1, ALERT = 2 };

/** @brief Z movement style override (Auto=detect from kinematics, or force) */
enum class ZMovementStyle { AUTO = 0, BED_MOVES = 1, NOZZLE_MOVES = 2 };

/**
 * @brief Application settings manager with reactive UI binding
 *
 * Coordinates persistence (Config), reactive subjects (lv_subject_t), immediate
 * effects (theme changes, Moonraker commands), and user preferences.
 *
 * Domain-specific settings are delegated to specialized managers:
 * - DisplaySettingsManager: dark mode, theme, dim, sleep, brightness, animations, etc.
 * - SystemSettingsManager: language, update channel, telemetry
 * - InputSettingsManager: scroll throw, scroll limit
 * - AudioSettingsManager: sounds, volume, UI sounds, sound theme, completion alerts
 * - SafetySettingsManager: e-stop confirmation, cancel escalation
 *
 * SettingsManager retains ownership of:
 * - LED control (depends on MoonrakerClient)
 * - Z movement style (depends on PrinterState)
 * - External spool info (depends on AMS types)
 * - Printer image (config-only, simple delegation candidate for later)
 *
 * Thread safety: Single-threaded, main LVGL thread only.
 */
class SettingsManager {
  public:
    /**
     * @brief Get singleton instance
     * @return Reference to global SettingsManager
     */
    static SettingsManager& instance();

    // Prevent copying
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    /**
     * @brief Initialize LVGL subjects
     *
     * MUST be called BEFORE creating XML components that bind to settings subjects.
     * Loads initial values from Config and registers subjects with LVGL XML system.
     * Also initializes all domain-specific managers.
     */
    void init_subjects();

    /**
     * @brief Deinitialize LVGL subjects
     *
     * Must be called before lv_deinit() to properly disconnect observers.
     * Called by StaticSubjectRegistry during application shutdown.
     */
    void deinit_subjects();

    /**
     * @brief Set Moonraker client reference for remote commands
     *
     * Required for LED control and other printer-dependent settings.
     * Call after MoonrakerClient is initialized.
     *
     * @param client Pointer to active MoonrakerClient (can be nullptr to disable)
     */
    void set_moonraker_client(MoonrakerClient* client);

    // =========================================================================
    // APPEARANCE SETTINGS (delegated to DisplaySettingsManager)
    // =========================================================================

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool get_dark_mode() const {
        return DisplaySettingsManager::instance().get_dark_mode();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_dark_mode(bool enabled) {
        DisplaySettingsManager::instance().set_dark_mode(enabled);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool is_dark_mode_available() const {
        return DisplaySettingsManager::instance().is_dark_mode_available();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void on_theme_changed() {
        DisplaySettingsManager::instance().on_theme_changed();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    std::string get_theme_name() const {
        return DisplaySettingsManager::instance().get_theme_name();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_theme_name(const std::string& name) {
        DisplaySettingsManager::instance().set_theme_name(name);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    std::string get_theme_options() const {
        return DisplaySettingsManager::instance().get_theme_options();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    int get_theme_index() const {
        return DisplaySettingsManager::instance().get_theme_index();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_theme_by_index(int index) {
        DisplaySettingsManager::instance().set_theme_by_index(index);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    int get_display_dim_sec() const {
        return DisplaySettingsManager::instance().get_display_dim_sec();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_display_dim_sec(int seconds) {
        DisplaySettingsManager::instance().set_display_dim_sec(seconds);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    int get_display_sleep_sec() const {
        return DisplaySettingsManager::instance().get_display_sleep_sec();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_display_sleep_sec(int seconds) {
        DisplaySettingsManager::instance().set_display_sleep_sec(seconds);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    int get_brightness() const {
        return DisplaySettingsManager::instance().get_brightness();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_brightness(int percent) {
        DisplaySettingsManager::instance().set_brightness(percent);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool has_backlight_control() const {
        return DisplaySettingsManager::instance().has_backlight_control();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool get_sleep_while_printing() const {
        return DisplaySettingsManager::instance().get_sleep_while_printing();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_sleep_while_printing(bool enabled) {
        DisplaySettingsManager::instance().set_sleep_while_printing(enabled);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool get_animations_enabled() const {
        return DisplaySettingsManager::instance().get_animations_enabled();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_animations_enabled(bool enabled) {
        DisplaySettingsManager::instance().set_animations_enabled(enabled);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool get_gcode_3d_enabled() const {
        return DisplaySettingsManager::instance().get_gcode_3d_enabled();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_gcode_3d_enabled(bool enabled) {
        DisplaySettingsManager::instance().set_gcode_3d_enabled(enabled);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    int get_bed_mesh_render_mode() const {
        return DisplaySettingsManager::instance().get_bed_mesh_render_mode();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_bed_mesh_render_mode(int mode) {
        DisplaySettingsManager::instance().set_bed_mesh_render_mode(mode);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    static const char* get_bed_mesh_render_mode_options() {
        return DisplaySettingsManager::get_bed_mesh_render_mode_options();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    std::string get_printer_image() const {
        return DisplaySettingsManager::instance().get_printer_image();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_printer_image(const std::string& id) {
        DisplaySettingsManager::instance().set_printer_image(id);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    bool get_bed_mesh_show_zero_plane() const {
        return DisplaySettingsManager::instance().get_bed_mesh_show_zero_plane();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    int get_gcode_render_mode() const {
        return DisplaySettingsManager::instance().get_gcode_render_mode();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_gcode_render_mode(int mode) {
        DisplaySettingsManager::instance().set_gcode_render_mode(mode);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    static const char* get_gcode_render_mode_options() {
        return DisplaySettingsManager::get_gcode_render_mode_options();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    TimeFormat get_time_format() const {
        return DisplaySettingsManager::instance().get_time_format();
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    void set_time_format(TimeFormat format) {
        DisplaySettingsManager::instance().set_time_format(format);
    }

    /** @deprecated Use DisplaySettingsManager::instance() directly */
    static const char* get_time_format_options() {
        return DisplaySettingsManager::get_time_format_options();
    }

    // =========================================================================
    // LANGUAGE SETTINGS (delegated to SystemSettingsManager)
    // =========================================================================

    /** @deprecated Use SystemSettingsManager::instance() directly */
    std::string get_language() const {
        return SystemSettingsManager::instance().get_language();
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    void set_language(const std::string& lang) {
        SystemSettingsManager::instance().set_language(lang);
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    void set_language_by_index(int index) {
        SystemSettingsManager::instance().set_language_by_index(index);
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    int get_language_index() const {
        return SystemSettingsManager::instance().get_language_index();
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    static const char* get_language_options() {
        return SystemSettingsManager::get_language_options();
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    static std::string language_index_to_code(int index) {
        return SystemSettingsManager::language_index_to_code(index);
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    static int language_code_to_index(const std::string& code) {
        return SystemSettingsManager::language_code_to_index(code);
    }

    // =========================================================================
    // PRINTER SETTINGS (owned by SettingsManager — MoonrakerClient dependency)
    // =========================================================================

    /**
     * @brief Get LED enabled state
     * @return true if LED is on
     */
    bool get_led_enabled() const;

    /**
     * @brief Set LED enabled state
     *
     * Updates subject, sends Moonraker command, and persists startup preference.
     * The LED state is saved as "LED on at start" preference.
     *
     * @param enabled true to turn on, false to turn off
     */
    void set_led_enabled(bool enabled);

    // =========================================================================
    // Z MOVEMENT STYLE (owned by SettingsManager — PrinterState dependency)
    // =========================================================================

    /** @brief Get Z movement style override (Auto/Bed Moves/Nozzle Moves) */
    ZMovementStyle get_z_movement_style() const;

    /** @brief Set Z movement style override and apply to printer state */
    void set_z_movement_style(ZMovementStyle style);

    /** @brief Get dropdown options string "Auto\nBed Moves\nNozzle Moves" */
    static const char* get_z_movement_style_options();

    /** @brief Z movement style subject (integer: 0=Auto, 1=Bed Moves, 2=Nozzle Moves) */
    lv_subject_t* subject_z_movement_style() {
        return &z_movement_style_subject_;
    }

    // =========================================================================
    // FILAMENT SETTINGS (owned by SettingsManager — AMS types dependency)
    // =========================================================================

    /**
     * @brief Get external spool info (bypass/direct spool)
     * @return SlotInfo with external spool data, or nullopt if not set
     */
    std::optional<SlotInfo> get_external_spool_info() const;

    /**
     * @brief Set external spool info (bypass/direct spool)
     * @param info SlotInfo with filament data (slot_index forced to -2)
     */
    void set_external_spool_info(const SlotInfo& info);

    /**
     * @brief Clear external spool info (back to unassigned)
     */
    void clear_external_spool_info();

    // =========================================================================
    // INPUT SETTINGS (delegated to InputSettingsManager)
    // =========================================================================

    /** @deprecated Use InputSettingsManager::instance() directly */
    int get_scroll_throw() const {
        return InputSettingsManager::instance().get_scroll_throw();
    }

    /** @deprecated Use InputSettingsManager::instance() directly */
    void set_scroll_throw(int value) {
        InputSettingsManager::instance().set_scroll_throw(value);
    }

    /** @deprecated Use InputSettingsManager::instance() directly */
    int get_scroll_limit() const {
        return InputSettingsManager::instance().get_scroll_limit();
    }

    /** @deprecated Use InputSettingsManager::instance() directly */
    void set_scroll_limit(int value) {
        InputSettingsManager::instance().set_scroll_limit(value);
    }

    /** @deprecated Use InputSettingsManager::instance() directly */
    bool is_restart_pending() const {
        return InputSettingsManager::instance().is_restart_pending();
    }

    /** @deprecated Use InputSettingsManager::instance() directly */
    void clear_restart_pending() {
        InputSettingsManager::instance().clear_restart_pending();
    }

    // =========================================================================
    // SAFETY SETTINGS (delegated to SafetySettingsManager)
    // =========================================================================

    /** @deprecated Use SafetySettingsManager::instance() directly */
    bool get_estop_require_confirmation() const {
        return SafetySettingsManager::instance().get_estop_require_confirmation();
    }

    /** @deprecated Use SafetySettingsManager::instance() directly */
    void set_estop_require_confirmation(bool require) {
        SafetySettingsManager::instance().set_estop_require_confirmation(require);
    }

    /** @deprecated Use SafetySettingsManager::instance() directly */
    bool get_cancel_escalation_enabled() const {
        return SafetySettingsManager::instance().get_cancel_escalation_enabled();
    }

    /** @deprecated Use SafetySettingsManager::instance() directly */
    void set_cancel_escalation_enabled(bool enabled) {
        SafetySettingsManager::instance().set_cancel_escalation_enabled(enabled);
    }

    /** @deprecated Use SafetySettingsManager::instance() directly */
    int get_cancel_escalation_timeout_seconds() const {
        return SafetySettingsManager::instance().get_cancel_escalation_timeout_seconds();
    }

    /** @deprecated Use SafetySettingsManager::instance() directly */
    void set_cancel_escalation_timeout_seconds(int seconds) {
        SafetySettingsManager::instance().set_cancel_escalation_timeout_seconds(seconds);
    }

    // =========================================================================
    // NOTIFICATION SETTINGS (delegated to AudioSettingsManager)
    // =========================================================================

    /** @deprecated Use AudioSettingsManager::instance() directly */
    bool get_sounds_enabled() const {
        return AudioSettingsManager::instance().get_sounds_enabled();
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    void set_sounds_enabled(bool enabled) {
        AudioSettingsManager::instance().set_sounds_enabled(enabled);
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    int get_volume() const {
        return AudioSettingsManager::instance().get_volume();
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    void set_volume(int volume) {
        AudioSettingsManager::instance().set_volume(volume);
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    bool get_ui_sounds_enabled() const {
        return AudioSettingsManager::instance().get_ui_sounds_enabled();
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    void set_ui_sounds_enabled(bool enabled) {
        AudioSettingsManager::instance().set_ui_sounds_enabled(enabled);
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    std::string get_sound_theme() const {
        return AudioSettingsManager::instance().get_sound_theme();
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    void set_sound_theme(const std::string& name) {
        AudioSettingsManager::instance().set_sound_theme(name);
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    CompletionAlertMode get_completion_alert_mode() const {
        return AudioSettingsManager::instance().get_completion_alert_mode();
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    void set_completion_alert_mode(CompletionAlertMode mode) {
        AudioSettingsManager::instance().set_completion_alert_mode(mode);
    }

    /** @deprecated Use AudioSettingsManager::instance() directly */
    static const char* get_completion_alert_options() {
        return AudioSettingsManager::get_completion_alert_options();
    }

    // =========================================================================
    // TELEMETRY SETTINGS (delegated to SystemSettingsManager)
    // =========================================================================

    /** @deprecated Use SystemSettingsManager::instance() directly */
    bool get_telemetry_enabled() const {
        return SystemSettingsManager::instance().get_telemetry_enabled();
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    void set_telemetry_enabled(bool enabled) {
        SystemSettingsManager::instance().set_telemetry_enabled(enabled);
    }

    // =========================================================================
    // UPDATE SETTINGS (delegated to SystemSettingsManager)
    // =========================================================================

    /** @deprecated Use SystemSettingsManager::instance() directly */
    int get_update_channel() const {
        return SystemSettingsManager::instance().get_update_channel();
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    void set_update_channel(int channel) {
        SystemSettingsManager::instance().set_update_channel(channel);
    }

    /** @deprecated Use SystemSettingsManager::instance() directly */
    static const char* get_update_channel_options() {
        return SystemSettingsManager::get_update_channel_options();
    }

    // =========================================================================
    // DISPLAY DIM/SLEEP OPTIONS (delegated to DisplaySettingsManager)
    // =========================================================================

    /** @deprecated Use DisplaySettingsManager directly */
    static const char* get_display_dim_options() {
        return DisplaySettingsManager::get_display_dim_options();
    }

    /** @deprecated Use DisplaySettingsManager directly */
    static int dim_seconds_to_index(int seconds) {
        return DisplaySettingsManager::dim_seconds_to_index(seconds);
    }

    /** @deprecated Use DisplaySettingsManager directly */
    static int index_to_dim_seconds(int index) {
        return DisplaySettingsManager::index_to_dim_seconds(index);
    }

    /** @deprecated Use DisplaySettingsManager directly */
    static const char* get_display_sleep_options() {
        return DisplaySettingsManager::get_display_sleep_options();
    }

    /** @deprecated Use DisplaySettingsManager directly */
    static int sleep_seconds_to_index(int seconds) {
        return DisplaySettingsManager::sleep_seconds_to_index(seconds);
    }

    /** @deprecated Use DisplaySettingsManager directly */
    static int index_to_sleep_seconds(int index) {
        return DisplaySettingsManager::index_to_sleep_seconds(index);
    }

    // =========================================================================
    // SUBJECT ACCESSORS (for XML binding) — forwarding to domain managers
    // =========================================================================

    /** @deprecated Use DisplaySettingsManager::instance().subject_dark_mode() */
    lv_subject_t* subject_dark_mode() {
        return DisplaySettingsManager::instance().subject_dark_mode();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_dark_mode_available() */
    lv_subject_t* subject_dark_mode_available() {
        return DisplaySettingsManager::instance().subject_dark_mode_available();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_theme_preset() */
    lv_subject_t* subject_theme_preset() {
        return DisplaySettingsManager::instance().subject_theme_preset();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_display_dim() */
    lv_subject_t* subject_display_dim() {
        return DisplaySettingsManager::instance().subject_display_dim();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_display_sleep() */
    lv_subject_t* subject_display_sleep() {
        return DisplaySettingsManager::instance().subject_display_sleep();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_brightness() */
    lv_subject_t* subject_brightness() {
        return DisplaySettingsManager::instance().subject_brightness();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_has_backlight() */
    lv_subject_t* subject_has_backlight() {
        return DisplaySettingsManager::instance().subject_has_backlight();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_sleep_while_printing() */
    lv_subject_t* subject_sleep_while_printing() {
        return DisplaySettingsManager::instance().subject_sleep_while_printing();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_animations_enabled() */
    lv_subject_t* subject_animations_enabled() {
        return DisplaySettingsManager::instance().subject_animations_enabled();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_gcode_3d_enabled() */
    lv_subject_t* subject_gcode_3d_enabled() {
        return DisplaySettingsManager::instance().subject_gcode_3d_enabled();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_bed_mesh_render_mode() */
    lv_subject_t* subject_bed_mesh_render_mode() {
        return DisplaySettingsManager::instance().subject_bed_mesh_render_mode();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_gcode_render_mode() */
    lv_subject_t* subject_gcode_render_mode() {
        return DisplaySettingsManager::instance().subject_gcode_render_mode();
    }

    /** @deprecated Use DisplaySettingsManager::instance().subject_time_format() */
    lv_subject_t* subject_time_format() {
        return DisplaySettingsManager::instance().subject_time_format();
    }

    /** @deprecated Use SystemSettingsManager::instance().subject_language() */
    lv_subject_t* subject_language() {
        return SystemSettingsManager::instance().subject_language();
    }

    /** @brief LED enabled subject (integer: 0=off, 1=on) */
    lv_subject_t* subject_led_enabled() {
        return &led_enabled_subject_;
    }

    /** @deprecated Use AudioSettingsManager::instance().subject_sounds_enabled() */
    lv_subject_t* subject_sounds_enabled() {
        return AudioSettingsManager::instance().subject_sounds_enabled();
    }

    /** @deprecated Use AudioSettingsManager::instance().subject_ui_sounds_enabled() */
    lv_subject_t* subject_ui_sounds_enabled() {
        return AudioSettingsManager::instance().subject_ui_sounds_enabled();
    }

    /** @deprecated Use AudioSettingsManager::instance().subject_volume() */
    lv_subject_t* subject_volume() {
        return AudioSettingsManager::instance().subject_volume();
    }

    /** @deprecated Use AudioSettingsManager::instance().subject_completion_alert() */
    lv_subject_t* subject_completion_alert() {
        return AudioSettingsManager::instance().subject_completion_alert();
    }

    /** @deprecated Use InputSettingsManager::instance().subject_scroll_throw() */
    lv_subject_t* subject_scroll_throw() {
        return InputSettingsManager::instance().subject_scroll_throw();
    }

    /** @deprecated Use InputSettingsManager::instance().subject_scroll_limit() */
    lv_subject_t* subject_scroll_limit() {
        return InputSettingsManager::instance().subject_scroll_limit();
    }

    /** @deprecated Use SafetySettingsManager::instance().subject_estop_require_confirmation() */
    lv_subject_t* subject_estop_require_confirmation() {
        return SafetySettingsManager::instance().subject_estop_require_confirmation();
    }

    /** @deprecated Use SafetySettingsManager::instance().subject_cancel_escalation_enabled() */
    lv_subject_t* subject_cancel_escalation_enabled() {
        return SafetySettingsManager::instance().subject_cancel_escalation_enabled();
    }

    /** @deprecated Use SafetySettingsManager::instance().subject_cancel_escalation_timeout() */
    lv_subject_t* subject_cancel_escalation_timeout() {
        return SafetySettingsManager::instance().subject_cancel_escalation_timeout();
    }

    /** @deprecated Use SystemSettingsManager::instance().subject_update_channel() */
    lv_subject_t* subject_update_channel() {
        return SystemSettingsManager::instance().subject_update_channel();
    }

    /** @deprecated Use SystemSettingsManager::instance().subject_telemetry_enabled() */
    lv_subject_t* subject_telemetry_enabled() {
        return SystemSettingsManager::instance().subject_telemetry_enabled();
    }

  private:
    SettingsManager();
    ~SettingsManager() = default;

    // Subject manager for RAII cleanup
    SubjectManager subjects_;

    // LVGL subjects — only those owned by SettingsManager
    lv_subject_t led_enabled_subject_;
    lv_subject_t z_movement_style_subject_;

    // External references
    MoonrakerClient* moonraker_client_ = nullptr;

    // State
    bool subjects_initialized_ = false;
};

} // namespace helix
