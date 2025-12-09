// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ui_bed_mesh.h"
#include "ui_card.h"
#include "ui_component_header_bar.h"
#include "ui_component_keypad.h"
#include "ui_dialog.h"
#include "ui_emergency_stop.h"
#include "ui_error_reporting.h"
#include "ui_fatal_error.h"
#include "ui_fonts.h"
#include "ui_gcode_viewer.h"
#include "ui_gradient_canvas.h"
#include "ui_icon.h"
#include "ui_icon_loader.h"
#include "ui_keyboard.h"
#include "ui_modal.h"
#include "ui_nav.h"
#include "ui_notification.h"
#include "ui_observer_guard.h"
#include "ui_panel_advanced.h"
#include "ui_panel_bed_mesh.h"
#include "ui_panel_calibration_pid.h"
#include "ui_panel_calibration_zoffset.h"
#include "ui_panel_controls.h"
#include "ui_panel_extrusion.h"
#include "ui_panel_fan.h"
#include "ui_panel_filament.h"
#include "ui_panel_gcode_test.h"
#include "ui_panel_glyphs.h"
#include "ui_panel_home.h"
#include "ui_panel_motion.h"
#include "ui_panel_notification_history.h"
#include "ui_panel_print_select.h"
#include "ui_panel_print_status.h"
#include "ui_panel_settings.h"
#include "ui_panel_step_test.h"
#include "ui_panel_temp_control.h"
#include "ui_panel_test.h"
#include "ui_severity_card.h"
#include "ui_spinner.h"
#include "ui_status_bar.h"
#include "ui_switch.h"
#include "ui_text.h"
#include "ui_text_input.h"
#include "ui_theme.h"
#include "ui_toast.h"
#include "ui_utils.h"
#include "ui_wizard.h"
#include "ui_wizard_wifi.h"

#include "app_globals.h"
#include "config.h"
#include "display_backend.h"
#include "gcode_file_modifier.h"
#include "logging_init.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/libs/svg/lv_svg_decoder.h"
#include "lvgl/src/xml/lv_xml.h"
#include "moonraker_api.h"
#include "moonraker_api_mock.h"
#include "moonraker_client.h"
#include "moonraker_client_mock.h"
#include "printer_state.h"
#include "runtime_config.h"
#include "settings_manager.h"
#include "sound_manager.h"
#include "tips_manager.h"
#include "usb_backend_mock.h"
#include "usb_manager.h"

#include <spdlog/spdlog.h>

#ifdef HELIX_DISPLAY_SDL
#include <SDL.h>
#endif
#include "cli_args.h"
#include "helix_timing.h"
#include "lvgl_init.h"
#include "print_completion.h"
#include "screenshot.h"
#include "splash_screen.h"
#include "xml_registration.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <queue>
#include <signal.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Forward declarations for panel global accessor functions
class HomePanel;
class ControlsPanel;
class MotionPanel;
class SettingsPanel;
class FilamentPanel;
class AdvancedPanel;
class PrintSelectPanel;
class PrintStatusPanel;
class ExtrusionPanel;
class BedMeshPanel;
class StepTestPanel;
class TestPanel;
class GlyphsPanel;
class GcodeTestPanel;

HomePanel& get_global_home_panel();
ControlsPanel& get_global_controls_panel();
MotionPanel& get_global_motion_panel();
SettingsPanel& get_global_settings_panel();
FilamentPanel& get_global_filament_panel();
AdvancedPanel& get_global_advanced_panel();
void init_global_advanced_panel(PrinterState& printer_state, MoonrakerAPI* api);
PrintSelectPanel* get_print_select_panel(PrinterState& printer_state, MoonrakerAPI* api);
PrintStatusPanel& get_global_print_status_panel();
ExtrusionPanel& get_global_extrusion_panel();
BedMeshPanel& get_global_bed_mesh_panel();
StepTestPanel& get_global_step_test_panel();
TestPanel& get_global_test_panel();
GlyphsPanel& get_global_glyphs_panel();
GcodeTestPanel* get_gcode_test_panel(PrinterState& printer_state, MoonrakerAPI* api);

// Ensure we're running from the project root directory.
// If the executable is in build/bin/, change to the project root so relative paths work.
static void ensure_project_root_cwd() {
    char exe_path[PATH_MAX];

#ifdef __APPLE__
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        return; // Failed to get path, assume CWD is correct
    }
    // Resolve symlinks
    char resolved[PATH_MAX];
    if (realpath(exe_path, resolved)) {
        strncpy(exe_path, resolved, PATH_MAX - 1);
        exe_path[PATH_MAX - 1] = '\0';
    }
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        return; // Failed to get path, assume CWD is correct
    }
    exe_path[len] = '\0';
#else
    return; // Unsupported platform, assume CWD is correct
#endif

    // Get directory containing executable
    char* last_slash = strrchr(exe_path, '/');
    if (!last_slash)
        return;
    *last_slash = '\0';

    // Check if we're in build/bin/ and go up two levels
    size_t dir_len = strlen(exe_path);
    const char* suffix = "/build/bin";
    size_t suffix_len = strlen(suffix);

    if (dir_len >= suffix_len && strcmp(exe_path + dir_len - suffix_len, suffix) == 0) {
        // Strip /build/bin to get project root
        exe_path[dir_len - suffix_len] = '\0';

        if (chdir(exe_path) == 0) {
            spdlog::debug("Changed working directory to: {}", exe_path);
        }
    }
}

// LVGL context is local to main() - see helix::LvglContext

// Screen dimensions (configurable via command line, default to small = 800x480)
static int SCREEN_WIDTH = UI_SCREEN_SMALL_W;
static int SCREEN_HEIGHT = UI_SCREEN_SMALL_H;

// Local instances (registered with app_globals via setters)
// Note: PrinterState is now a singleton accessed via get_printer_state()
// Using unique_ptr for RAII - raw pointers are passed to app_globals for access
static std::unique_ptr<MoonrakerClient> moonraker_client;
static std::unique_ptr<MoonrakerAPI> moonraker_api;
static std::unique_ptr<TempControlPanel> temp_control_panel;
static std::unique_ptr<UsbManager> usb_manager;

// Panels that need MoonrakerAPI - stored as pointers for deferred set_api() call
static PrintSelectPanel* print_select_panel = nullptr;
static PrintStatusPanel* print_status_panel = nullptr;
static MotionPanel* motion_panel = nullptr;
static ExtrusionPanel* extrusion_panel = nullptr;
static BedMeshPanel* bed_mesh_panel = nullptr;

// Runtime configuration
static RuntimeConfig g_runtime_config;

// Logging configuration (parsed before Config system is available)
// NOTE: Non-static to allow access from cli_args.cpp
std::string g_log_dest_cli; // CLI override for log destination
std::string g_log_file_cli; // CLI override for log file path

// Thread-safe queue for Moonraker notifications (cross-thread communication)
static std::queue<json> notification_queue;
static std::mutex notification_mutex;

// Overlay panel tracking for proper lifecycle management
struct OverlayPanels {
    lv_obj_t* motion = nullptr;
    lv_obj_t* nozzle_temp = nullptr;
    lv_obj_t* bed_temp = nullptr;
    lv_obj_t* extrusion = nullptr;
    lv_obj_t* print_status = nullptr;
} static overlay_panels;

// Pending navigation for -p flag (deferred until after Moonraker connection)
// This prevents race conditions where panels are shown before data is available.
struct PendingNavigation {
    bool has_pending = false;
    int initial_panel = -1;
    bool show_motion = false;
    bool show_nozzle_temp = false;
    bool show_bed_temp = false;
    bool show_extrusion = false;
    bool show_fan = false;
    bool show_print_status = false;
    bool show_bed_mesh = false;
    bool show_zoffset = false;
    bool show_pid = false;
    bool show_file_detail = false;
} static g_pending_nav;

// Print completion notification observer - stored here, initialized from
// helix::init_print_completion_observer()
static ObserverGuard print_completion_observer;

const RuntimeConfig& get_runtime_config() {
    return g_runtime_config;
}

RuntimeConfig* get_mutable_runtime_config() {
    return &g_runtime_config;
}

// Forward declarations
static void initialize_moonraker_client(Config* config);

// Execute deferred navigation after Moonraker connection is established.
// This is called from the main loop when a "_navigate_pending" notification is received.
// Running on main thread ensures LVGL thread safety.
static void execute_pending_navigation() {
    if (!g_pending_nav.has_pending) {
        return;
    }

    spdlog::info("[Navigation] Executing deferred navigation after connection established");
    g_pending_nav.has_pending = false;

    lv_obj_t* screen = lv_display_get_screen_active(NULL);
    if (!screen) {
        spdlog::error("[Navigation] No active screen for deferred navigation");
        return;
    }

    // Navigate to initial panel if requested
    if (g_pending_nav.initial_panel >= 0) {
        spdlog::debug("[Navigation] Setting active panel: {}", g_pending_nav.initial_panel);
        ui_nav_set_active(static_cast<ui_panel_id_t>(g_pending_nav.initial_panel));
    }

    // Show deferred overlay panels
    if (g_pending_nav.show_motion) {
        spdlog::debug("[Navigation] Opening deferred motion overlay");
        overlay_panels.motion = (lv_obj_t*)lv_xml_create(screen, "motion_panel", nullptr);
        if (overlay_panels.motion) {
            get_global_motion_panel().setup(overlay_panels.motion, screen);
            ui_nav_push_overlay(overlay_panels.motion);
        }
    }
    if (g_pending_nav.show_nozzle_temp) {
        spdlog::debug("[Navigation] Opening deferred nozzle temp overlay");
        overlay_panels.nozzle_temp = (lv_obj_t*)lv_xml_create(screen, "nozzle_temp_panel", nullptr);
        if (overlay_panels.nozzle_temp && temp_control_panel) {
            temp_control_panel->setup_nozzle_panel(overlay_panels.nozzle_temp, screen);
            ui_nav_push_overlay(overlay_panels.nozzle_temp);
        }
    }
    if (g_pending_nav.show_bed_temp) {
        spdlog::debug("[Navigation] Opening deferred bed temp overlay");
        overlay_panels.bed_temp = (lv_obj_t*)lv_xml_create(screen, "bed_temp_panel", nullptr);
        if (overlay_panels.bed_temp && temp_control_panel) {
            temp_control_panel->setup_bed_panel(overlay_panels.bed_temp, screen);
            ui_nav_push_overlay(overlay_panels.bed_temp);
        }
    }
    if (g_pending_nav.show_extrusion) {
        spdlog::debug("[Navigation] Opening deferred extrusion overlay");
        overlay_panels.extrusion = (lv_obj_t*)lv_xml_create(screen, "extrusion_panel", nullptr);
        if (overlay_panels.extrusion) {
            get_global_extrusion_panel().setup(overlay_panels.extrusion, screen);
            ui_nav_push_overlay(overlay_panels.extrusion);
        }
    }
    if (g_pending_nav.show_fan) {
        spdlog::debug("[Navigation] Opening deferred fan control overlay");
        auto& fan_panel = get_global_fan_panel();
        if (!fan_panel.are_subjects_initialized()) {
            fan_panel.init_subjects();
        }
        lv_obj_t* fan_obj = (lv_obj_t*)lv_xml_create(screen, "fan_panel", nullptr);
        if (fan_obj) {
            fan_panel.setup(fan_obj, screen);
            ui_nav_push_overlay(fan_obj);
        }
    }
    if (g_pending_nav.show_print_status && overlay_panels.print_status) {
        spdlog::debug("[Navigation] Opening deferred print status overlay");
        ui_nav_push_overlay(overlay_panels.print_status);
    }
    if (g_pending_nav.show_bed_mesh) {
        spdlog::debug("[Navigation] Opening deferred bed mesh overlay");
        lv_obj_t* bed_mesh = (lv_obj_t*)lv_xml_create(screen, "bed_mesh_panel", nullptr);
        if (bed_mesh) {
            get_global_bed_mesh_panel().setup(bed_mesh, screen);
            ui_nav_push_overlay(bed_mesh);
            spdlog::debug("[Navigation] Bed mesh overlay pushed to nav stack");
        }
    }
    if (g_pending_nav.show_zoffset) {
        spdlog::debug("[Navigation] Opening deferred Z-offset calibration overlay");
        lv_obj_t* zoffset_panel =
            (lv_obj_t*)lv_xml_create(screen, "calibration_zoffset_panel", nullptr);
        if (zoffset_panel) {
            get_global_zoffset_cal_panel().setup(zoffset_panel, screen, moonraker_client.get());
            ui_nav_push_overlay(zoffset_panel);
        }
    }
    if (g_pending_nav.show_pid) {
        spdlog::debug("[Navigation] Opening deferred PID tuning overlay");
        lv_obj_t* pid_panel = (lv_obj_t*)lv_xml_create(screen, "calibration_pid_panel", nullptr);
        if (pid_panel) {
            get_global_pid_cal_panel().setup(pid_panel, screen, moonraker_client.get());
            ui_nav_push_overlay(pid_panel);
        }
    }
    if (g_pending_nav.show_file_detail) {
        spdlog::debug("[Navigation] File detail requested - navigating to print select panel");
        ui_nav_set_active(UI_PANEL_PRINT_SELECT);
    }

    spdlog::info("[Navigation] Deferred navigation complete");
}

// Initialize all reactive subjects for data binding
static void initialize_subjects() {
    spdlog::debug("Initializing reactive subjects...");
    app_globals_init_subjects();   // Global subjects (notification subject, etc.)
    ui_nav_init();                 // Navigation system (icon colors, active panel)
    ui_status_bar_init_subjects(); // Status bar subjects (printer/network icon states)

    // PrinterState must be initialized BEFORE panels that observe its subjects
    // (e.g., HomePanel observes led_state_, extruder_temp_, connection_state_)
    get_printer_state()
        .init_subjects(); // Printer state subjects (CRITICAL: must be before panel creation)

    // Register print completion notification observer (watches print_state_enum for terminal
    // states)
    print_completion_observer = helix::init_print_completion_observer();

    get_global_home_panel().init_subjects();                  // Home panel data bindings
    get_global_controls_panel().init_subjects();              // Controls panel launcher
    get_global_filament_panel().init_subjects();              // Filament panel
    get_global_settings_panel().init_subjects();              // Settings panel launcher
    init_global_advanced_panel(get_printer_state(), nullptr); // Initialize advanced panel instance
    get_global_advanced_panel().init_subjects();              // Advanced panel capability subjects
    ui_wizard_init_subjects(); // Wizard subjects (for first-run config)
    ui_keypad_init_subjects(); // Keypad display subject (for reactive binding)

    // Panels that need MoonrakerAPI - store pointers for deferred set_api()
    print_select_panel = get_print_select_panel(get_printer_state(), nullptr);
    print_select_panel->init_subjects();

    // Initialize UsbManager with mock backend in test mode
    usb_manager = std::make_unique<UsbManager>(g_runtime_config.should_mock_usb());
    if (usb_manager->start()) {
        spdlog::info("UsbManager started (mock={})", g_runtime_config.should_mock_usb());
        print_select_panel->set_usb_manager(usb_manager.get());
    } else {
        spdlog::warn("Failed to start UsbManager");
    }
    print_status_panel = &get_global_print_status_panel();
    print_status_panel->init_subjects();
    motion_panel = &get_global_motion_panel();
    motion_panel->init_subjects();
    extrusion_panel = &get_global_extrusion_panel();
    extrusion_panel->init_subjects();
    bed_mesh_panel = &get_global_bed_mesh_panel();
    bed_mesh_panel->init_subjects();

    // Initialize TempControlPanel (needs PrinterState ready)
    temp_control_panel = std::make_unique<TempControlPanel>(get_printer_state(), nullptr);
    temp_control_panel->init_subjects();

    // Inject TempControlPanel into ControlsPanel for temperature sub-screens
    get_global_controls_panel().set_temp_control_panel(temp_control_panel.get());

    // Inject TempControlPanel into HomePanel for temperature icon click
    get_global_home_panel().set_temp_control_panel(temp_control_panel.get());

    // Initialize notification system (after subjects are ready)
    ui_notification_init();

    // Initialize E-Stop overlay subjects (must be before XML creation)
    EmergencyStopOverlay::instance().init_subjects();

    // Set up USB drive event notifications (after notification system is ready)
    if (usb_manager) {
        usb_manager->set_drive_callback([](UsbEvent event, const UsbDrive& drive) {
            (void)drive; // Currently not using drive info in messages
            if (event == UsbEvent::DRIVE_INSERTED) {
                ui_notification_success("USB drive connected");

                // Show USB tab in PrintSelectPanel
                if (print_select_panel) {
                    print_select_panel->on_usb_drive_inserted();
                }
            } else if (event == UsbEvent::DRIVE_REMOVED) {
                ui_notification_info("USB drive removed");

                // Hide USB tab and switch to Printer source if viewing USB
                if (print_select_panel) {
                    print_select_panel->on_usb_drive_removed();
                }
            }
        });

        // In test mode, schedule demo drive insertion after UI is fully ready
        // This ensures the toast notification is visible to the user
        if (g_runtime_config.should_mock_usb()) {
            // Use LVGL timer to delay insertion - this runs on the main thread after UI init
            lv_timer_create(
                [](lv_timer_t* timer) {
                    if (auto* mock = dynamic_cast<UsbBackendMock*>(usb_manager->get_backend())) {
                        mock->add_demo_drives();
                        spdlog::debug("Added demo USB drives for test mode (delayed)");
                    }
                    lv_timer_delete(timer);
                },
                3000, // 3 second delay for UI to fully initialize
                nullptr);
        }
    }
}

// Initialize Moonraker client and API instances
static void initialize_moonraker_client(Config* config) {
    spdlog::debug("Initializing Moonraker client...");

    // Create client instance (mock or real based on test mode)
    if (get_runtime_config().should_mock_moonraker()) {
        spdlog::debug("[Test Mode] Creating MOCK Moonraker client (Voron 2.4 profile)");
        moonraker_client =
            std::make_unique<MoonrakerClientMock>(MoonrakerClientMock::PrinterType::VORON_24);
    } else {
        spdlog::debug("Creating REAL Moonraker client");
        moonraker_client = std::make_unique<MoonrakerClient>();
    }

    // Register with app_globals (raw pointer for access, main.cpp owns lifetime)
    set_moonraker_client(moonraker_client.get());

    // Initialize SoundManager with Moonraker client for M300 audio feedback
    SoundManager::instance().set_moonraker_client(moonraker_client.get());

    // Configure timeouts from config file
    uint32_t connection_timeout = static_cast<uint32_t>(
        config->get<int>(config->df() + "moonraker_connection_timeout_ms", 10000));
    uint32_t request_timeout = static_cast<uint32_t>(
        config->get<int>(config->df() + "moonraker_request_timeout_ms", 30000));
    uint32_t keepalive_interval = static_cast<uint32_t>(
        config->get<int>(config->df() + "moonraker_keepalive_interval_ms", 10000));
    uint32_t reconnect_min_delay = static_cast<uint32_t>(
        config->get<int>(config->df() + "moonraker_reconnect_min_delay_ms", 200));
    uint32_t reconnect_max_delay = static_cast<uint32_t>(
        config->get<int>(config->df() + "moonraker_reconnect_max_delay_ms", 2000));

    moonraker_client->configure_timeouts(connection_timeout, request_timeout, keepalive_interval,
                                         reconnect_min_delay, reconnect_max_delay);

    spdlog::debug("Moonraker timeouts configured: connection={}ms, request={}ms, keepalive={}ms",
                  connection_timeout, request_timeout, keepalive_interval);

    // Register event handler to translate transport events to UI notifications
    // This decouples the transport layer (MoonrakerClient) from the UI layer
    moonraker_client->register_event_handler([](const MoonrakerEvent& evt) {
        switch (evt.type) {
        case MoonrakerEventType::CONNECTION_LOST:
            // Warning toast for connection loss
            ui_notification_warning(evt.message.c_str());
            break;

        case MoonrakerEventType::RECONNECTED:
            // Success toast for reconnection
            ui_notification_success(evt.message.c_str());
            break;

        case MoonrakerEventType::KLIPPY_DISCONNECTED:
            // Modal error for Klipper firmware disconnect (serious issue)
            ui_notification_error("Printer Firmware Disconnected", evt.message.c_str(), true);
            break;

        case MoonrakerEventType::KLIPPY_READY:
            // Success toast for Klipper ready
            ui_notification_success(evt.message.c_str());
            break;

        case MoonrakerEventType::CONNECTION_FAILED:
            // Modal error for connection failure
            ui_notification_error("Connection Failed", evt.message.c_str(), true);
            break;

        default:
            // Fallback for other events (RPC_ERROR, DISCOVERY_FAILED, etc.)
            if (evt.is_error) {
                ui_notification_error(nullptr, evt.message.c_str(), false);
            } else {
                ui_notification_warning(evt.message.c_str());
            }
            break;
        }
    });

    // Set up state change callback to queue updates for main thread
    // CRITICAL: This callback runs on the Moonraker event loop thread, NOT the main thread.
    // LVGL is NOT thread-safe, so we must NOT call any LVGL functions here.
    // Instead, queue the state change and process it on the main thread.
    moonraker_client->set_state_change_callback([](ConnectionState old_state,
                                                   ConnectionState new_state) {
        spdlog::debug("[main] State change callback invoked: {} -> {} (queueing for main thread)",
                      static_cast<int>(old_state), static_cast<int>(new_state));

        // Queue state change for main thread processing (same mutex as notifications)
        // Use a special JSON object with "_connection_state" marker
        std::lock_guard<std::mutex> lock(notification_mutex);
        json state_change;
        state_change["_connection_state"] = true;
        state_change["old_state"] = static_cast<int>(old_state);
        state_change["new_state"] = static_cast<int>(new_state);
        notification_queue.push(state_change);
    });

    // Register notification callback to queue updates for main thread
    // CRITICAL: Moonraker callbacks run on background thread, but LVGL is NOT thread-safe
    // Queue notifications here, process on main thread in event loop
    moonraker_client->register_notify_update([](json notification) {
        std::lock_guard<std::mutex> lock(notification_mutex);
        notification_queue.push(notification);
    });

    // Create MoonrakerAPI instance (mock or real based on test mode)
    spdlog::debug("Creating MoonrakerAPI instance...");
    if (get_runtime_config().should_use_test_files()) {
        spdlog::debug("[Test Mode] Creating MOCK MoonrakerAPI (local file transfers)");
        moonraker_api = std::make_unique<MoonrakerAPIMock>(*moonraker_client, get_printer_state());
    } else {
        moonraker_api = std::make_unique<MoonrakerAPI>(*moonraker_client, get_printer_state());
    }

    // Register with app_globals (raw pointer for access, main.cpp owns lifetime)
    set_moonraker_api(moonraker_api.get());

    // Update all panels with API reference (pass raw pointer, main.cpp owns lifetime)
    // Note: These panels are initialized in initialize_subjects() before this function
    get_global_home_panel().set_api(moonraker_api.get());
    if (temp_control_panel) {
        temp_control_panel->set_api(moonraker_api.get());
    }
    if (print_select_panel) {
        print_select_panel->set_api(moonraker_api.get());
    }
    if (print_status_panel) {
        print_status_panel->set_api(moonraker_api.get());
    }
    if (motion_panel) {
        motion_panel->set_api(moonraker_api.get());
    }
    if (extrusion_panel) {
        extrusion_panel->set_api(moonraker_api.get());
    }
    if (bed_mesh_panel) {
        bed_mesh_panel->set_api(moonraker_api.get());
    }

    // Initialize E-Stop overlay with dependencies (creates the floating button)
    EmergencyStopOverlay::instance().init(get_printer_state(), moonraker_api.get());
    EmergencyStopOverlay::instance().create();
    // Apply persisted E-Stop confirmation setting
    EmergencyStopOverlay::instance().set_require_confirmation(
        SettingsManager::instance().get_estop_require_confirmation());
    // Set initial panel for visibility tracking (home_panel is default)
    EmergencyStopOverlay::instance().on_panel_changed("home_panel");

    spdlog::debug("Moonraker client initialized (not connected yet)");
}

// Main application
int main(int argc, char** argv) {
    // Store argv early for restart capability (before any modifications)
    app_store_argv(argc, argv);

    // Ensure we're running from the project root for relative path access
    ensure_project_root_cwd();

    // Parse command-line arguments into structured result
    helix::CliArgs args;
    if (!helix::parse_cli_args(argc, argv, args, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        return 0; // Help shown or parse error
    }

    // Check HELIX_AUTO_QUIT_MS environment variable (only if --timeout not specified)
    if (args.timeout_sec == 0) {
        const char* auto_quit_env = std::getenv("HELIX_AUTO_QUIT_MS");
        if (auto_quit_env != nullptr) {
            char* endptr;
            long val = strtol(auto_quit_env, &endptr, 10);
            if (*endptr == '\0' && val >= 100 && val <= 3600000) {
                // Convert milliseconds to seconds (round up to ensure at least 1 second)
                args.timeout_sec = static_cast<int>((val + 999) / 1000);
            }
        }
    }

    // Check HELIX_AUTO_SCREENSHOT environment variable
    const char* auto_screenshot_env = std::getenv("HELIX_AUTO_SCREENSHOT");
    if (auto_screenshot_env != nullptr && strcmp(auto_screenshot_env, "1") == 0) {
        args.screenshot_enabled = true;
    }

    // Initialize config system early so we can read logging settings
    Config* config = Config::get_instance();
    config->init("helixconfig.json");

    // Initialize logging subsystem
    // Priority: CLI > config > auto-detect
    {
        helix::logging::LogConfig log_config;

        // Set log level from verbosity flags
        switch (args.verbosity) {
        case 0:
            log_config.level = spdlog::level::warn;
            break;
        case 1:
            log_config.level = spdlog::level::info;
            break;
        case 2:
            log_config.level = spdlog::level::debug;
            break;
        default:
            log_config.level = spdlog::level::trace;
            break;
        }

        // Determine log destination: CLI > config > auto
        std::string log_dest_str = g_log_dest_cli;
        if (log_dest_str.empty()) {
            log_dest_str = config->get<std::string>("/log_dest", "auto");
        }
        log_config.target = helix::logging::parse_log_target(log_dest_str);

        // Determine log file path: CLI > config > auto
        log_config.file_path = g_log_file_cli;
        if (log_config.file_path.empty()) {
            log_config.file_path = config->get<std::string>("/log_path", "");
        }

        helix::logging::init(log_config);
    }

    spdlog::info("HelixScreen UI Prototype");
    spdlog::info("========================");
    spdlog::debug("Target: {}x{}", SCREEN_WIDTH, SCREEN_HEIGHT);
    spdlog::debug("DPI: {}{}", (args.dpi > 0 ? args.dpi : LV_DPI_DEF),
                  (args.dpi > 0 ? " (custom)" : " (default)"));
    spdlog::debug("Nav Width: {} pixels", UI_NAV_WIDTH(SCREEN_WIDTH));
    spdlog::debug("Initial Panel: {}", args.initial_panel);

    // Cleanup stale temp files from G-code modifications (older than 1 hour)
    size_t cleaned = gcode::GCodeFileModifier::cleanup_temp_files();
    if (cleaned > 0) {
        spdlog::info("Cleaned up {} stale G-code temp file(s)", cleaned);
    }

    // Determine theme: CLI overrides config, config overrides default (dark)
    bool dark_mode;
    if (args.dark_mode_cli >= 0) {
        // CLI explicitly set --dark or --light (temporary override, not saved)
        dark_mode = (args.dark_mode_cli == 1);
        spdlog::debug("Using CLI theme override: {}", dark_mode ? "dark" : "light");
    } else {
        // Load from config (or default to dark)
        dark_mode = config->get<bool>("/dark_mode", true);
        spdlog::debug("Loaded theme preference from config: {}", dark_mode ? "dark" : "light");
    }

#ifdef HELIX_DISPLAY_SDL
    // Set window position environment variables for LVGL SDL driver (desktop only)
    if (args.display_num >= 0) {
        char display_str[32];
        snprintf(display_str, sizeof(display_str), "%d", args.display_num);
        if (setenv("HELIX_SDL_DISPLAY", display_str, 1) != 0) {
            spdlog::error("Failed to set HELIX_SDL_DISPLAY environment variable");
            return 1;
        }
        spdlog::debug("Window will be centered on display {}", args.display_num);
    }
    if (args.x_pos >= 0 && args.y_pos >= 0) {
        char x_str[32], y_str[32];
        snprintf(x_str, sizeof(x_str), "%d", args.x_pos);
        snprintf(y_str, sizeof(y_str), "%d", args.y_pos);
        if (setenv("HELIX_SDL_XPOS", x_str, 1) != 0 || setenv("HELIX_SDL_YPOS", y_str, 1) != 0) {
            spdlog::error("Failed to set window position environment variables");
            return 1;
        }
        spdlog::debug("Window will be positioned at ({}, {})", args.x_pos, args.y_pos);
    } else if ((args.x_pos >= 0 && args.y_pos < 0) || (args.x_pos < 0 && args.y_pos >= 0)) {
        spdlog::warn("Both -x and -y must be specified for exact positioning. Ignoring.");
    }
#endif

    // Signal external splash process to exit BEFORE creating our display
    // This is critical for DRM - only one process can hold the display at a time
    if (g_runtime_config.splash_pid > 0) {
        spdlog::info("Signaling splash process (PID {}) to exit...", g_runtime_config.splash_pid);
        if (kill(g_runtime_config.splash_pid, SIGUSR1) == 0) {
            // Wait for splash to actually exit and release DRM resources
            // We can't use waitpid() since we're not the parent, so poll with kill(pid, 0)
            int wait_attempts = 50; // 50 * 20ms = 1 second max
            while (wait_attempts-- > 0 && kill(g_runtime_config.splash_pid, 0) == 0) {
                usleep(20000); // 20ms
            }
            if (wait_attempts <= 0) {
                spdlog::warn("Splash process did not exit in time, proceeding anyway");
            } else {
                spdlog::debug("Splash process exited, proceeding with display init");
            }
        } else {
            spdlog::debug("Splash process already exited (PID {})", g_runtime_config.splash_pid);
        }
        // Clear the PID so we don't try to signal it again later
        g_runtime_config.splash_pid = 0;
    }

    // Initialize LVGL with display backend
    helix::LvglContext lvgl_ctx;
    if (!helix::init_lvgl(SCREEN_WIDTH, SCREEN_HEIGHT, lvgl_ctx)) {
        return 1;
    }

    // Apply custom DPI if specified (before theme init)
    if (args.dpi > 0) {
        lv_display_set_dpi(lvgl_ctx.display, args.dpi);
        spdlog::debug("Display DPI set to: {}", args.dpi);
    } else {
        spdlog::debug("Display DPI: {} (from LV_DPI_DEF)", lv_display_get_dpi(lvgl_ctx.display));
    }

    // Create main screen
    lv_obj_t* screen = lv_screen_active();

    // Set window icon (after screen is created)
    ui_set_window_icon(lvgl_ctx.display);

    // Initialize app-level resize handler for responsive layouts
    ui_resize_handler_init(screen);

    // Initialize tips manager (uses standard C++ file I/O, not LVGL's "A:" filesystem)
    TipsManager* tips_mgr = TipsManager::get_instance();
    if (!tips_mgr->init("config/printing_tips.json")) {
        spdlog::warn("Tips manager failed to initialize - tips will not be available");
    } else {
        spdlog::debug("Loaded {} tips", tips_mgr->get_total_tips());
    }

    // Register fonts and images for XML (must be done BEFORE globals.xml for theme init)
    helix::register_fonts_and_images();

    // Register XML components (globals first to make constants available)
    spdlog::debug("Registering XML components...");
    lv_xml_register_component_from_file("A:ui_xml/globals.xml");

    // Initialize LVGL theme from globals.xml constants (after fonts and globals are registered)
    ui_theme_init(lvgl_ctx.display,
                  dark_mode); // dark_mode from command-line args (--dark/--light) or config

    // Theme preference is saved by the settings panel when user toggles dark mode

    // Apply theme background color to screen
    ui_theme_apply_bg_color(screen, "app_bg_color", LV_PART_MAIN);

    // Show splash screen AFTER theme init (skip if requested via --skip-splash or --test)
    // Theme must be initialized first so app_bg_color runtime constant is available
    if (!g_runtime_config.should_skip_splash()) {
        helix::show_splash_screen(SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    // Register custom widgets (must be before XML component registration)
    // Note: Material Design icons are now font-based (mdi_icons_*.c)
    // Icon lookup happens via ui_icon_codepoints.h
    ui_icon_register_widget();
    ui_switch_register();
    ui_card_register();
    ui_severity_card_register();
    ui_dialog_register();
    ui_bed_mesh_register();
    ui_gcode_viewer_register();
    ui_gradient_canvas_register();

    // Initialize component systems (BEFORE XML registration)
    ui_component_header_bar_init();

    // WORKAROUND: Add small delay to stabilize display/LVGL initialization
    // Prevents race condition between display backend and LVGL 9 XML component registration
    helix_delay(100);

    // Register remaining XML components (globals already registered for theme init)
    helix::register_xml_components();

    // Initialize reactive subjects BEFORE creating XML
    initialize_subjects();

    // Register status bar event callbacks BEFORE creating XML (so LVGL can find them)
    ui_status_bar_register_callbacks();

    // Create entire UI from XML (single component contains everything)
    lv_obj_t* app_layout = (lv_obj_t*)lv_xml_create(screen, "app_layout", NULL);

    // Disable scrollbars on screen to prevent overflow issues with overlay panels
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    // Force layout calculation for all LV_SIZE_CONTENT widgets
    lv_obj_update_layout(screen);

    // Register app_layout with navigation system (to prevent hiding it)
    ui_nav_set_app_layout(app_layout);

    // Initialize status bar (must be after XML creation and layout update)
    ui_status_bar_init();

    // Initialize toast notification system (registers close button callback)
    ui_toast_init();

    // Initialize shared overlay backdrop
    ui_nav_init_overlay_backdrop(screen);

    // Find widgets by name (robust to XML structure changes)
    lv_obj_t* navbar = lv_obj_find_by_name(app_layout, "navbar");
    lv_obj_t* content_area = lv_obj_find_by_name(app_layout, "content_area");

    if (!navbar || !content_area) {
        spdlog::error("Failed to find navbar/content_area in app_layout");
        helix::deinit_lvgl(lvgl_ctx);
        return 1;
    }

    // Wire up navigation button click handlers and trigger initial color update
    ui_nav_wire_events(navbar);

    // NOTE: Status icons (printer, network, notification) are now in home_panel.xml
    // They use XML-defined event callbacks and don't need C++ wiring

    // Find panel container by name (robust to layout changes like removing status_bar)
    lv_obj_t* panel_container = lv_obj_find_by_name(content_area, "panel_container");
    if (!panel_container) {
        spdlog::error("Failed to find panel_container in content_area");
        helix::deinit_lvgl(lvgl_ctx);
        return 1;
    }

    // Find all panel widgets by name (robust to child order changes)
    static const char* panel_names[UI_PANEL_COUNT] = {"home_panel",     "print_select_panel",
                                                      "controls_panel", "filament_panel",
                                                      "settings_panel", "advanced_panel"};

    lv_obj_t* panels[UI_PANEL_COUNT];
    for (int i = 0; i < UI_PANEL_COUNT; i++) {
        panels[i] = lv_obj_find_by_name(panel_container, panel_names[i]);
        if (!panels[i]) {
            spdlog::error("Missing panel '{}' in panel_container", panel_names[i]);
            helix::deinit_lvgl(lvgl_ctx);
            return 1;
        }
    }

    // Register panels with navigation system for show/hide management
    ui_nav_set_panels(panels);

    // Setup home panel observers (panels[0] is home panel)
    get_global_home_panel().setup(panels[0], screen);

    // Setup controls panel (wire launcher card click handlers)
    get_global_controls_panel().setup(panels[UI_PANEL_CONTROLS], screen);

    // Setup print select panel (wires up events, creates overlays, NOTE: data populated later)
    get_print_select_panel(get_printer_state(), nullptr)
        ->setup(panels[UI_PANEL_PRINT_SELECT], screen);

    // Setup filament panel (wire preset/action button handlers)
    get_global_filament_panel().setup(panels[UI_PANEL_FILAMENT], screen);

    // Setup settings panel (wire launcher card click handlers)
    get_global_settings_panel().setup(panels[UI_PANEL_SETTINGS], screen);

    // Setup advanced panel (wire action row click handlers)
    get_global_advanced_panel().setup(panels[UI_PANEL_ADVANCED], screen);

    // Initialize numeric keypad modal component (creates reusable keypad widget)
    ui_keypad_init(screen);

    // Create print status panel (overlay for active prints)
    overlay_panels.print_status = (lv_obj_t*)lv_xml_create(screen, "print_status_panel", nullptr);
    if (overlay_panels.print_status) {
        get_global_print_status_panel().setup(overlay_panels.print_status, screen);
        lv_obj_add_flag(overlay_panels.print_status, LV_OBJ_FLAG_HIDDEN); // Hidden by default

        // Wire print status panel to print select (for launching prints)
        get_print_select_panel(get_printer_state(), nullptr)
            ->set_print_status_panel(overlay_panels.print_status);

        spdlog::debug("Print status panel created and wired to print select");
    } else {
        spdlog::error("Failed to create print status panel");
    }

    spdlog::debug("XML UI created successfully with reactive navigation");

    // Test notifications - commented out, uncomment to debug notification history
    // if (get_runtime_config().test_mode) {
    //     NOTIFY_INFO("Info notification test");
    //     NOTIFY_SUCCESS("Success notification test");
    //     NOTIFY_WARNING("Warning notification test");
    //     NOTIFY_ERROR("Error notification test");
    // }

    // Initialize Moonraker client EARLY (before wizard, so it's available for connection test)
    // But don't connect yet - just create the instances
    initialize_moonraker_client(config);

    // Initialize global keyboard BEFORE wizard (required for textarea registration)
    // NOTE: Keyboard is created early but will appear on top due to being moved to top layer below
    ui_keyboard_init(screen);

    // Check if first-run wizard is required (skip for special test panels and explicit panel
    // requests)
    bool wizard_active = false;
    if ((args.force_wizard || config->is_wizard_required()) && !args.overlays.step_test &&
        !args.overlays.test_panel && !args.overlays.keypad && !args.overlays.keyboard &&
        !args.overlays.gcode_test && !args.panel_requested) {
        spdlog::info("Starting first-run configuration wizard");

        // Register wizard event callbacks and responsive constants BEFORE creating
        ui_wizard_register_event_callbacks();
        ui_wizard_container_register_responsive_constants();

        lv_obj_t* wizard = ui_wizard_create(screen);

        if (wizard) {
            spdlog::debug("Wizard created successfully");
            wizard_active = true;

            // Set initial step (screen loader sets appropriate title)
            int initial_step = (args.wizard_step >= 1) ? args.wizard_step : 1;
            ui_wizard_navigate_to_step(initial_step);

            // Move keyboard to top layer so it appears above the full-screen wizard overlay
            lv_obj_t* keyboard = ui_keyboard_get_instance();
            if (keyboard) {
                lv_obj_move_foreground(keyboard);
                spdlog::debug("[Keyboard] Moved to foreground (above wizard overlay)");
            }
        } else {
            spdlog::error("Failed to create wizard");
        }
    }

    // Defer panel/overlay navigation until after Moonraker connection (if connecting).
    // This prevents race conditions where panels are shown before data is available.
    // Overlays that don't require Moonraker data (keypad, keyboard, etc.) open immediately.
    bool needs_moonraker = args.overlays.needs_moonraker() || args.initial_panel >= 0;

    if (!wizard_active && needs_moonraker) {
        // Store pending navigation - will be executed after Moonraker discovery completes
        g_pending_nav.has_pending = true;
        g_pending_nav.initial_panel = args.initial_panel;
        g_pending_nav.show_motion = args.overlays.motion;
        g_pending_nav.show_nozzle_temp = args.overlays.nozzle_temp;
        g_pending_nav.show_bed_temp = args.overlays.bed_temp;
        g_pending_nav.show_extrusion = args.overlays.extrusion;
        g_pending_nav.show_fan = args.overlays.fan;
        g_pending_nav.show_print_status = args.overlays.print_status;
        g_pending_nav.show_bed_mesh = args.overlays.bed_mesh;
        g_pending_nav.show_zoffset = args.overlays.zoffset;
        g_pending_nav.show_pid = args.overlays.pid;
        g_pending_nav.show_file_detail = args.overlays.file_detail;
        spdlog::info("[Navigation] Deferred panel/overlay navigation until Moonraker connects");
    }

    // Show non-Moonraker overlays immediately (they don't need printer data)
    if (!wizard_active) {
        if (args.overlays.keypad) {
            spdlog::debug("Opening keypad modal as requested by command-line flag");
            ui_keypad_config_t keypad_config = {.initial_value = 0.0f,
                                                .min_value = 0.0f,
                                                .max_value = 300.0f,
                                                .title_label = "Test Keypad",
                                                .unit_label = "°C",
                                                .allow_decimal = true,
                                                .allow_negative = false,
                                                .callback = nullptr,
                                                .user_data = nullptr};
            ui_keypad_show(&keypad_config);
        }
        if (args.overlays.keyboard) {
            spdlog::debug("Showing keyboard as requested by command-line flag");
            ui_keyboard_show(nullptr);
        }
        if (args.overlays.step_test) {
            spdlog::debug("Creating step progress test widget as requested by command-line flag");
            lv_obj_t* step_test = (lv_obj_t*)lv_xml_create(screen, "step_progress_test", nullptr);
            if (step_test) {
                get_global_step_test_panel().setup(step_test, screen);
            }
        }
        if (args.overlays.test_panel) {
            spdlog::debug("Opening test panel as requested by command-line flag");
            lv_obj_t* test_panel_obj = (lv_obj_t*)lv_xml_create(screen, "test_panel", nullptr);
            if (test_panel_obj) {
                get_global_test_panel().setup(test_panel_obj, screen);
            }
        }
        if (args.overlays.file_detail) {
            spdlog::debug("File detail view requested - navigating to print select panel first");
            ui_nav_set_active(UI_PANEL_PRINT_SELECT);
        }

        // Handle --select-file flag: auto-select a file in the print select panel
        if (g_runtime_config.select_file != nullptr) {
            spdlog::info("--select-file flag: Will auto-select file '{}'",
                         g_runtime_config.select_file);
            ui_nav_set_active(UI_PANEL_PRINT_SELECT);
            // Set pending selection - will trigger when file list is loaded
            auto* print_panel = get_print_select_panel(get_printer_state(), moonraker_api.get());
            if (print_panel) {
                print_panel->set_pending_file_selection(g_runtime_config.select_file);
            }
        }
    }

    // Create G-code test panel if requested (independent of wizard state)
    if (args.overlays.gcode_test) {
        spdlog::debug("Creating G-code test panel");
        lv_obj_t* gcode_test =
            ui_panel_gcode_test_create(screen); // Uses deprecated wrapper (creates + setups)
        if (gcode_test) {
            spdlog::debug("G-code test panel created successfully");
        } else {
            spdlog::error("Failed to create G-code test panel");
        }
    }

    // Create glyphs panel if requested (independent of wizard state)
    if (args.overlays.glyphs) {
        spdlog::debug("Creating glyphs reference panel");
        lv_obj_t* glyphs_panel = ui_panel_glyphs_create(screen);
        if (glyphs_panel) {
            spdlog::debug("Glyphs panel created successfully");
        } else {
            spdlog::error("Failed to create glyphs panel");
        }
    }

    // Create gradient test panel if requested (independent of wizard state)
    if (args.overlays.gradient_test) {
        spdlog::debug("Creating gradient test panel");
        lv_obj_t* gradient_panel = (lv_obj_t*)lv_xml_create(screen, "gradient_test_panel", nullptr);
        if (gradient_panel) {
            spdlog::debug("Gradient test panel created successfully");
        } else {
            spdlog::error("Failed to create gradient test panel");
        }
    }

    // Connect to Moonraker (only if not in wizard and we have saved config)
    // Wizard will handle its own connection test
    std::string saved_host = config->get<std::string>(config->df() + "moonraker_host", "");
    if (!args.force_wizard && !config->is_wizard_required() && !saved_host.empty()) {
        // Build WebSocket URL from config
        std::string moonraker_url =
            "ws://" + config->get<std::string>(config->df() + "moonraker_host") + ":" +
            std::to_string(config->get<int>(config->df() + "moonraker_port")) + "/websocket";

        // Build HTTP base URL for file transfers (same host:port, http:// scheme)
        std::string http_base_url =
            "http://" + config->get<std::string>(config->df() + "moonraker_host") + ":" +
            std::to_string(config->get<int>(config->df() + "moonraker_port"));
        moonraker_api->set_http_base_url(http_base_url);

        // Register discovery callback (Observer pattern - decouples Moonraker from PrinterState)
        moonraker_client->set_on_discovery_complete([](const PrinterCapabilities& caps) {
            // Update PrinterState with discovered capabilities for reactive UI bindings
            get_printer_state().set_printer_capabilities(caps);

            // Update version info from client (for Settings About section)
            get_printer_state().set_klipper_version(moonraker_client->get_software_version());
            get_printer_state().set_moonraker_version(moonraker_client->get_moonraker_version());
        });

        // Connect to Moonraker
        spdlog::debug("Connecting to Moonraker at {}", moonraker_url);
        int connect_result = moonraker_client->connect(
            moonraker_url.c_str(),
            []() {
                spdlog::info("✓ Connected to Moonraker");
                // State change callback will handle updating PrinterState

                // Start auto-discovery (must be called AFTER connection is established)
                moonraker_client->discover_printer([]() {
                    spdlog::info("✓ Printer auto-discovery complete");
                    // Queue notification to trigger deferred navigation on main thread
                    if (g_pending_nav.has_pending) {
                        std::lock_guard<std::mutex> lock(notification_mutex);
                        notification_queue.push({{"_navigate_pending", true}});
                    }
                });
            },
            []() {
                spdlog::warn("✗ Disconnected from Moonraker");
                // State change callback will handle updating PrinterState
            });

        if (connect_result != 0) {
            spdlog::error("Failed to initiate Moonraker connection (code {})", connect_result);
            // State change callback will handle updating PrinterState
        }
    }

    // Auto-screenshot timer (configurable delay after UI creation)
    uint32_t screenshot_time =
        helix_get_ticks() + (static_cast<uint32_t>(args.screenshot_delay_sec) * 1000U);
    bool screenshot_taken = false;

    // Auto-quit timeout timer (if enabled)
    uint32_t start_time = helix_get_ticks();
    uint32_t timeout_ms = static_cast<uint32_t>(args.timeout_sec) * 1000U;

    // Request timeout check timer (check every 2 seconds)
    uint32_t last_timeout_check = helix_get_ticks();
    uint32_t timeout_check_interval = static_cast<uint32_t>(
        config->get<int>(config->df() + "moonraker_timeout_check_interval_ms", 2000));

    // Main event loop - LVGL handles display events internally via lv_timer_handler()
    // Loop continues while display exists and quit not requested
    while (lv_display_get_next(NULL) && !app_quit_requested()) {
#ifdef HELIX_DISPLAY_SDL
        // Desktop keyboard shortcuts (SDL only)
        // Check for Cmd+Q (macOS) or Win+Q (Windows) to quit
        SDL_Keymod modifiers = SDL_GetModState();
        const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
        if ((modifiers & KMOD_GUI) && keyboard_state[SDL_SCANCODE_Q]) {
            spdlog::info("Cmd+Q/Win+Q pressed - exiting...");
            break;
        }

#endif

        // Auto-screenshot after configured delay (only if enabled)
        if (args.screenshot_enabled && !screenshot_taken && helix_get_ticks() >= screenshot_time) {
            helix::save_screenshot();
            screenshot_taken = true;
        }

        // Auto-quit after timeout (if enabled)
        if (args.timeout_sec > 0 && (helix_get_ticks() - start_time) >= timeout_ms) {
            spdlog::info("Timeout reached ({} seconds) - exiting...", args.timeout_sec);
            break;
        }

        // Check for request timeouts (using configured interval)
        uint32_t current_time = helix_get_ticks();
        if (current_time - last_timeout_check >= timeout_check_interval) {
            moonraker_client->process_timeouts();
            last_timeout_check = current_time;
        }

        // Process queued Moonraker notifications on main thread (LVGL thread-safety)
        {
            std::lock_guard<std::mutex> lock(notification_mutex);
            while (!notification_queue.empty()) {
                json notification = notification_queue.front();
                notification_queue.pop();

                // Check for deferred navigation trigger (queued from discovery callback)
                if (notification.contains("_navigate_pending")) {
                    execute_pending_navigation();
                    continue;
                }

                // Check for connection state change (queued from state_change_callback)
                if (notification.contains("_connection_state")) {
                    int new_state = notification["new_state"].get<int>();
                    static const char* messages[] = {
                        "Disconnected",     // DISCONNECTED
                        "Connecting...",    // CONNECTING
                        "Connected",        // CONNECTED
                        "Reconnecting...",  // RECONNECTING
                        "Connection Failed" // FAILED
                    };
                    spdlog::debug("[main] Processing queued connection state change: {}",
                                  messages[new_state]);
                    get_printer_state().set_printer_connection_state(new_state,
                                                                     messages[new_state]);
                } else {
                    // Regular Moonraker notification
                    get_printer_state().update_from_notification(notification);
                }
            }
        }

        // Check display sleep (inactivity timeout)
        SettingsManager::instance().check_display_sleep();

        // Run LVGL tasks - handles display events and processes input
        lv_timer_handler();
        fflush(stdout);
        helix_delay(5); // Small delay to prevent 100% CPU usage
    }

    // Cleanup
    spdlog::info("Shutting down...");

    // Clear app_globals references before destroying instances
    set_moonraker_api(nullptr);
    set_moonraker_client(nullptr);

    // Reset unique_ptrs explicitly in correct order (API before client)
    moonraker_api.reset();
    moonraker_client.reset();

    // Clean up USB manager explicitly BEFORE spdlog shutdown.
    // UsbBackendMock::stop() logs, and we need spdlog alive for that.
    usb_manager.reset();

    // Clean up wizard WiFi step explicitly BEFORE LVGL deinit and spdlog shutdown.
    // This owns WiFiManager and EthernetManager which have background threads.
    // If destroyed during static destruction, those threads may access destroyed mutexes.
    destroy_wizard_wifi_step();

    helix::deinit_lvgl(lvgl_ctx); // Releases display backend and LVGL

    // Shutdown spdlog BEFORE static destruction begins.
    // Many static unique_ptr<Panel> objects have destructors that may log.
    // If spdlog is destroyed first during static destruction, logging crashes.
    // By calling shutdown() here, we flush and drop all sinks, making any
    // subsequent log calls safe no-ops.
    spdlog::shutdown();

    return 0;
}
