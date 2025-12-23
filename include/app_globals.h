// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl.h"

// Forward declarations
class MoonrakerClient;
class MoonrakerAPI;
class MoonrakerManager;
class PrinterState;

/**
 * @brief Get global MoonrakerClient instance
 * @return Pointer to global MoonrakerClient (may be nullptr if not initialized)
 */
MoonrakerClient* get_moonraker_client();

/**
 * @brief Set global MoonrakerClient instance (called by main.cpp during init)
 * @param client Pointer to MoonrakerClient instance
 */
void set_moonraker_client(MoonrakerClient* client);

/**
 * @brief Get global MoonrakerAPI instance
 * @return Pointer to global MoonrakerAPI (may be nullptr if not initialized)
 */
MoonrakerAPI* get_moonraker_api();

/**
 * @brief Set global MoonrakerAPI instance (called by main.cpp during init)
 * @param api Pointer to MoonrakerAPI instance
 */
void set_moonraker_api(MoonrakerAPI* api);

/**
 * @brief Get global MoonrakerManager instance
 * @return Pointer to global MoonrakerManager (may be nullptr if not initialized)
 */
MoonrakerManager* get_moonraker_manager();

/**
 * @brief Set global MoonrakerManager instance (called by Application during init)
 * @param manager Pointer to MoonrakerManager instance
 */
void set_moonraker_manager(MoonrakerManager* manager);

/**
 * @brief Get global PrinterState singleton instance
 *
 * Returns a reference to the singleton PrinterState instance.
 * The instance is created on first call and persists for the lifetime of the program.
 * Thread-safe initialization guaranteed by C++11 static local variable semantics.
 *
 * @return Reference to singleton PrinterState (always valid)
 */
PrinterState& get_printer_state();

/**
 * @brief Get the global notification subject
 *
 * Any module can emit notifications by calling:
 * ```cpp
 * NotificationData notif = {severity, title, message, show_modal};
 * lv_subject_set_pointer(&get_notification_subject(), &notif);
 * ```
 *
 * @return Reference to the global notification subject
 */
lv_subject_t& get_notification_subject();

/**
 * @brief Initialize all global subjects
 *
 * Must be called during app initialization after LVGL is initialized.
 * Initializes reactive subjects used throughout the application.
 */
void app_globals_init_subjects();

/**
 * @brief Store original command-line arguments for restart capability
 *
 * Must be called early in main() before any argument processing.
 * Required for app_request_restart() to work.
 *
 * @param argc Argument count from main()
 * @param argv Argument vector from main()
 */
void app_store_argv(int argc, char** argv);

/**
 * @brief Request clean application shutdown
 *
 * Sets a flag that the main event loop checks. When set, the main loop
 * will exit cleanly, allowing proper cleanup (spdlog shutdown, etc.).
 * Use this instead of exit() or _Exit() for graceful termination.
 */
void app_request_quit();

/**
 * @brief Request application restart
 *
 * Forks a new process and exec's the same binary with the same arguments.
 * The new process starts fresh while the current process exits cleanly.
 * On embedded (systemd), this provides seamless restart. On macOS for
 * development, the new window appears and the old one closes.
 *
 * Requires app_store_argv() to have been called during startup.
 */
void app_request_restart();

/**
 * @brief Request application restart for theme change
 *
 * Like app_request_restart(), but modifies arguments for theme switch:
 * - Removes any --dark or --light flags (so saved config is used)
 * - Replaces -p/--panel argument with "-p settings" (return to settings)
 * - Preserves all other arguments (--test, -s, -v, etc.)
 *
 * Call this after saving the new theme to config.
 */
void app_request_restart_for_theme();

/**
 * @brief Check if quit has been requested
 * @return true if app_request_quit() or app_request_restart() was called
 */
bool app_quit_requested();