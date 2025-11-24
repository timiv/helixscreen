// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file moonraker_inspector_interactive.cpp
 * @brief Interactive TUI mode for moonraker inspector with collapsible tree
 *
 * Features:
 * - Arrow keys to navigate sections
 * - Enter/Space to expand/collapse sections
 * - Color-coded status indicators
 * - Real-time data display
 *
 * Built with tuibox - single-header C library using pure ANSI escape sequences
 */

#include "moonraker_client.h"
#include "ansi_colors.h"
#include "terminal_raw.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

using json = nlohmann::json;

// Get terminal size
struct TermSize {
    int rows;
    int cols;
};

TermSize get_terminal_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return {w.ws_row, w.ws_col};
}

// Tree node for hierarchical data display
struct TreeNode {
    std::string key;
    std::string value;
    bool expanded;
    bool is_section;  // Section headers vs data items
    int indent_level;
    std::string object_name;  // Moonraker object name for querying
    json object_data;  // Detailed data from Moonraker
    bool data_fetched;  // Have we fetched detailed data?
    std::vector<TreeNode> children;

    TreeNode(const std::string& k, const std::string& v = "", bool section = false, int indent = 0, const std::string& obj_name = "")
        : key(k), value(v), expanded(section), is_section(section), indent_level(indent),
          object_name(obj_name), data_fetched(false) {}
};

// Global state for interactive mode
struct InteractiveState {
    std::vector<TreeNode> tree;
    int selected_index;
    int scroll_offset;
    json server_info;
    json printer_info;
    json objects_list;
    bool data_ready;
    MoonrakerClient* client;  // For querying object details
    TreeNode* selected_node;  // Track actual selected node (not just index)
    bool need_redraw;  // Flag to trigger redraw from async callbacks

    InteractiveState() : selected_index(0), scroll_offset(0), data_ready(false), client(nullptr), selected_node(nullptr), need_redraw(false) {}
};

static InteractiveState* g_state = nullptr;

// Format a value from Moonraker JSON response
std::string format_value(const json& val) {
    if (val.is_number()) {
        if (val.is_number_float()) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f", val.get<double>());
            return buf;
        }
        return std::to_string(val.get<int>());
    } else if (val.is_boolean()) {
        return val.get<bool>() ? "true" : "false";
    } else if (val.is_string()) {
        return val.get<std::string>();
    } else if (val.is_array()) {
        return "[array]";
    } else if (val.is_object()) {
        return "[object]";
    }
    return "?";
}

// Query Moonraker for detailed object data
void query_object_data(TreeNode* node, MoonrakerClient* client) {
    if (!node || node->object_name.empty() || node->data_fetched || !client) {
        return;
    }

    // Add loading indicator
    node->children.clear();
    node->children.push_back(TreeNode("⏳ Loading data...", "", false, 3));
    if (g_state) {
        g_state->need_redraw = true;
    }

    // Query this specific object
    json params = json::object();
    params["objects"] = json::object();
    params["objects"][node->object_name] = json::value_t::null;  // Query all fields

    client->send_jsonrpc("printer.objects.query", params,
        [node](json response) {
            if (response.contains("result") && response["result"].contains("status")) {
                node->object_data = response["result"]["status"];
                node->data_fetched = true;

                // Clear loading indicator and populate with detailed data
                node->children.clear();

                if (node->object_data.contains(node->object_name)) {
                    const auto& obj_data = node->object_data[node->object_name];

                    // Add each field as a child
                    for (auto it = obj_data.begin(); it != obj_data.end(); ++it) {
                        std::string key = it.key();
                        std::string value = format_value(it.value());

                        // Add descriptive labels for common fields
                        if (key == "temperature") {
                            key = "🌡️  Current Temp";
                            value += "°C";
                        } else if (key == "target") {
                            key = "🎯 Target Temp";
                            value += "°C";
                        } else if (key == "power") {
                            key = "⚡ Heater Power";
                            double pct = it.value().get<double>() * 100.0;
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%.1f%%", pct);
                            value = buf;
                        } else if (key == "speed") {
                            key = "💨 Fan Speed";
                            double pct = it.value().get<double>() * 100.0;
                            char buf[32];
                            snprintf(buf, sizeof(buf), "%.0f%%", pct);
                            value = buf;
                        } else if (key == "rpm") {
                            key = "🔄 RPM";
                        } else if (key == "run_current") {
                            key = "⚡ Run Current";
                            value += "A";
                        } else if (key == "hold_current") {
                            key = "⏸️  Hold Current";
                            value += "A";
                        } else if (key == "microsteps") {
                            key = "📐 Microsteps";
                        }

                        node->children.push_back(TreeNode(key, value, false, 3));
                    }
                }

                // Trigger redraw to show new data
                if (g_state) {
                    g_state->need_redraw = true;
                }
            }
        },
        [node](const MoonrakerError&) {
            // Query failed - show error
            node->children.clear();
            node->children.push_back(TreeNode("❌ Failed to fetch data", "", false, 3));
            if (g_state) {
                g_state->need_redraw = true;
            }
        });
}

// Get human-readable description for a Moonraker component
std::string get_component_description(const std::string& component) {
    if (component == "file_manager") {
        return "Manages G-code files and print job queue";
    } else if (component == "update_manager") {
        return "Handles software updates for Moonraker/Klipper/system";
    } else if (component == "machine") {
        return "System info, power control, and service management";
    } else if (component == "webcam") {
        return "Manages webcam streams for print monitoring";
    } else if (component == "history") {
        return "Tracks print history and statistics";
    } else if (component == "authorization") {
        return "Handles API authentication and user permissions";
    } else if (component == "data_store") {
        return "Persistent storage for UI settings and preferences";
    } else if (component == "announcements") {
        return "News and important updates from Moonraker project";
    } else if (component == "octoprint_compat") {
        return "Compatibility layer for OctoPrint plugins/slicers";
    } else if (component == "job_queue") {
        return "Sequential print job queue management";
    } else if (component == "job_state") {
        return "Tracks current print job state and progress";
    } else if (component == "proc_stats") {
        return "System resource monitoring (CPU/memory/disk)";
    } else if (component == "klippy_apis") {
        return "API endpoints for Klipper communication";
    } else if (component == "database") {
        return "Internal database for configuration storage";
    } else if (component == "http_client") {
        return "HTTP client for external requests (updates/notifications)";
    } else if (component == "secrets") {
        return "Secure storage for API keys and credentials";
    } else if (component == "template") {
        return "Jinja2 template processing for dynamic configs";
    } else if (component == "klippy_connection") {
        return "WebSocket connection manager to Klipper";
    } else if (component == "jsonrpc") {
        return "JSON-RPC protocol handler for API requests";
    } else if (component == "internal_transport") {
        return "Internal IPC between Moonraker components";
    } else if (component == "application") {
        return "Core application framework and lifecycle";
    } else if (component == "websockets") {
        return "WebSocket server for realtime client connections";
    } else if (component == "dbus_manager") {
        return "DBus integration for system service control";
    } else if (component == "shell_command") {
        return "Execute shell commands from G-code macros";
    } else if (component == "extensions") {
        return "Third-party plugin extension system";
    }
    return "";
}

// Get human-readable description for a Klipper object
std::string get_object_description(const std::string& obj_name) {
    if (obj_name.find("extruder") != std::string::npos) {
        return "Hotend extruder - heats plastic and pushes filament";
    } else if (obj_name.find("heater_bed") != std::string::npos) {
        return "Heated print bed - keeps prints from warping";
    } else if (obj_name.find("heater_generic") != std::string::npos) {
        return "Generic heater - chamber/other heating element";
    } else if (obj_name.find("temperature_sensor") != std::string::npos) {
        return "Temperature sensor - monitors ambient/component temps";
    } else if (obj_name.find("fan") != std::string::npos) {
        if (obj_name.find("heater_fan") != std::string::npos) {
            return "Heater fan - cools hotend/heatbreak";
        } else if (obj_name.find("controller_fan") != std::string::npos) {
            return "Controller fan - cools MCU/stepper drivers";
        } else if (obj_name.find("fan_generic") != std::string::npos) {
            return "Generic fan - chamber/auxiliary cooling";
        } else {
            return "Part cooling fan - cools printed plastic";
        }
    } else if (obj_name.find("led") != std::string::npos || obj_name.find("neopixel") != std::string::npos) {
        return "LED strip - lighting/status indication";
    } else if (obj_name.find("tmc") != std::string::npos) {
        return "TMC stepper driver - silent motor control with stallguard";
    } else if (obj_name.find("stepper_") != std::string::npos) {
        return "Stepper motor - controls axis movement";
    } else if (obj_name.find("probe") != std::string::npos) {
        return "Z-probe - measures bed height for leveling";
    } else if (obj_name.find("bltouch") != std::string::npos) {
        return "BLTouch probe - servo-based bed leveling sensor";
    } else if (obj_name.find("bed_mesh") != std::string::npos) {
        return "Bed mesh - compensates for uneven bed surface";
    } else if (obj_name.find("filament_switch_sensor") != std::string::npos) {
        return "Filament sensor - detects filament runout";
    } else if (obj_name.find("filament_motion_sensor") != std::string::npos) {
        return "Filament motion sensor - detects jams/clogs";
    } else if (obj_name.find("servo") != std::string::npos) {
        return "Servo motor - precise angular positioning";
    } else if (obj_name.find("gcode_macro") != std::string::npos) {
        return "G-code macro - custom print command";
    } else if (obj_name.find("gcode_button") != std::string::npos) {
        return "Physical button - triggers G-code commands";
    } else if (obj_name.find("firmware_retraction") != std::string::npos) {
        return "Firmware retraction - fast filament retract/prime";
    }
    return "";  // No description
}

// Build tree from collected data (all sections collapsed by default)
void build_tree(InteractiveState* state) {
    state->tree.clear();

    // Server Information section (collapsed by default)
    TreeNode server_section("📡 Server Information", "", true, 0);
    server_section.expanded = false;  // Collapsed by default

    if (state->server_info.contains("klippy_connected")) {
        bool connected = state->server_info["klippy_connected"].get<bool>();
        std::string status = connected ? "Connected ✓" : "Disconnected ✗";
        server_section.children.push_back(TreeNode("Klippy Status", status, false, 1));
    }

    if (state->server_info.contains("klippy_state")) {
        server_section.children.push_back(TreeNode("Klippy State",
            state->server_info["klippy_state"].get<std::string>(), false, 1));
    }

    // Moonraker version field is actually "version" not "moonraker_version"
    if (state->server_info.contains("version")) {
        server_section.children.push_back(TreeNode("Moonraker Version",
            state->server_info["version"].get<std::string>(), false, 1));
    } else if (state->server_info.contains("moonraker_version")) {
        server_section.children.push_back(TreeNode("Moonraker Version",
            state->server_info["moonraker_version"].get<std::string>(), false, 1));
    }

    if (state->server_info.contains("klippy_version")) {
        server_section.children.push_back(TreeNode("Klippy Version",
            state->server_info["klippy_version"].get<std::string>(), false, 1));
    }

    if (state->server_info.contains("components")) {
        TreeNode comp_node("🧩 Components (Moonraker Modules)", "", true, 1);
        comp_node.expanded = false;  // Collapsible subsection
        for (const auto& comp : state->server_info["components"]) {
            std::string comp_name = comp.get<std::string>();
            std::string desc = get_component_description(comp_name);
            comp_node.children.push_back(TreeNode(comp_name, desc, false, 2));
        }
        server_section.children.push_back(comp_node);
    }

    state->tree.push_back(server_section);

    // Printer Information section (collapsed by default)
    TreeNode printer_section("🖨️  Printer Information", "", true, 0);
    printer_section.expanded = false;

    if (state->printer_info.contains("state")) {
        std::string state_str = state->printer_info["state"].get<std::string>();
        printer_section.children.push_back(TreeNode("State", state_str, false, 1));
    }

    if (state->printer_info.contains("hostname")) {
        printer_section.children.push_back(TreeNode("Hostname",
            state->printer_info["hostname"].get<std::string>(), false, 1));
    }

    // Check multiple possible field names for Klipper version
    if (state->printer_info.contains("software_version")) {
        printer_section.children.push_back(TreeNode("Klipper Version",
            state->printer_info["software_version"].get<std::string>(), false, 1));
    } else if (state->printer_info.contains("klipper_version")) {
        printer_section.children.push_back(TreeNode("Klipper Version",
            state->printer_info["klipper_version"].get<std::string>(), false, 1));
    }

    state->tree.push_back(printer_section);

    // Hardware Objects section
    if (state->objects_list.contains("objects")) {
        TreeNode hw_section("🔧 Hardware Objects", "", true, 0);
        hw_section.expanded = false;  // Collapsed by default

        const auto& obj_array = state->objects_list["objects"];

        // Categorize objects (more detailed categorization)
        std::vector<std::string> heaters, sensors, fans, leds, macros, steppers, probes, other;

        for (const auto& obj : obj_array) {
            std::string name = obj.get<std::string>();

            if (name.find("gcode_macro") != std::string::npos) {
                macros.push_back(name);
            } else if (name.find("extruder") != std::string::npos ||
                name.find("heater_bed") != std::string::npos ||
                name.find("heater_generic") != std::string::npos) {
                heaters.push_back(name);
            } else if (name.find("temperature_sensor") != std::string::npos ||
                       name.find("temperature_") != std::string::npos) {
                sensors.push_back(name);
            } else if (name.find("fan") != std::string::npos) {
                fans.push_back(name);
            } else if (name.find("led") != std::string::npos ||
                       name.find("neopixel") != std::string::npos ||
                       name.find("dotstar") != std::string::npos) {
                leds.push_back(name);
            } else if (name.find("stepper") != std::string::npos ||
                       name.find("tmc") != std::string::npos) {
                steppers.push_back(name);
            } else if (name.find("probe") != std::string::npos ||
                       name.find("bltouch") != std::string::npos ||
                       name.find("bed_mesh") != std::string::npos) {
                probes.push_back(name);
            } else if (name == "gcode" || name == "webhooks" || name == "configfile" ||
                       name == "mcu" || name.find("mcu ") == 0 || name == "heaters" ||
                       name == "gcode_move" || name == "print_stats" || name == "virtual_sdcard" ||
                       name == "display_status" || name == "exclude_object" ||
                       name == "idle_timeout" || name == "pause_resume") {
                // Core Klipper objects - not interesting to expand
                continue;
            } else {
                other.push_back(name);
            }
        }

        // Add categorized subsections (all collapsed by default)
        if (!heaters.empty()) {
            TreeNode heater_node("🔥 Heaters (" + std::to_string(heaters.size()) + ")", "", true, 1);
            heater_node.expanded = false;
            for (const auto& h : heaters) {
                std::string desc = get_object_description(h);
                heater_node.children.push_back(TreeNode(h, desc, true, 2, h));  // Expandable, store object name
            }
            hw_section.children.push_back(heater_node);
        }

        if (!sensors.empty()) {
            TreeNode sensor_node("🌡️  Sensors (" + std::to_string(sensors.size()) + ")", "", true, 1);
            sensor_node.expanded = false;
            for (const auto& s : sensors) {
                std::string desc = get_object_description(s);
                sensor_node.children.push_back(TreeNode(s, desc, true, 2, s));
            }
            hw_section.children.push_back(sensor_node);
        }

        if (!fans.empty()) {
            TreeNode fan_node("💨 Fans (" + std::to_string(fans.size()) + ")", "", true, 1);
            fan_node.expanded = false;
            for (const auto& f : fans) {
                std::string desc = get_object_description(f);
                fan_node.children.push_back(TreeNode(f, desc, true, 2, f));
            }
            hw_section.children.push_back(fan_node);
        }

        if (!leds.empty()) {
            TreeNode led_node("💡 LEDs (" + std::to_string(leds.size()) + ")", "", true, 1);
            led_node.expanded = false;
            for (const auto& l : leds) {
                std::string desc = get_object_description(l);
                led_node.children.push_back(TreeNode(l, desc, true, 2, l));
            }
            hw_section.children.push_back(led_node);
        }

        if (!steppers.empty()) {
            TreeNode stepper_node("🔩 Steppers/Drivers (" + std::to_string(steppers.size()) + ")", "", true, 1);
            stepper_node.expanded = false;
            for (const auto& s : steppers) {
                std::string desc = get_object_description(s);
                stepper_node.children.push_back(TreeNode(s, desc, true, 2, s));
            }
            hw_section.children.push_back(stepper_node);
        }

        if (!probes.empty()) {
            TreeNode probe_node("📍 Probes/Leveling (" + std::to_string(probes.size()) + ")", "", true, 1);
            probe_node.expanded = false;
            for (const auto& p : probes) {
                std::string desc = get_object_description(p);
                probe_node.children.push_back(TreeNode(p, desc, true, 2, p));
            }
            hw_section.children.push_back(probe_node);
        }

        if (!macros.empty()) {
            TreeNode macro_node("⚙️  G-code Macros (" + std::to_string(macros.size()) + ")", "", true, 1);
            macro_node.expanded = false;  // ESPECIALLY collapsed by default
            for (const auto& m : macros) {
                std::string desc = get_object_description(m);
                macro_node.children.push_back(TreeNode(m, desc, true, 2, m));
            }
            hw_section.children.push_back(macro_node);
        }

        if (!other.empty()) {
            TreeNode other_node("🔌 Accessories (" + std::to_string(other.size()) + ")", "", true, 1);
            other_node.expanded = false;
            for (const auto& o : other) {
                std::string desc = get_object_description(o);
                other_node.children.push_back(TreeNode(o, desc, true, 2, o));
            }
            hw_section.children.push_back(other_node);
        }

        state->tree.push_back(hw_section);
    }
}

// Flatten tree for rendering (only visible nodes)
std::vector<TreeNode*> flatten_tree(std::vector<TreeNode>& tree) {
    std::vector<TreeNode*> flat;

    for (auto& node : tree) {
        flat.push_back(&node);

        if (node.expanded && !node.children.empty()) {
            for (auto& child : node.children) {
                flat.push_back(&child);

                if (child.expanded && !child.children.empty()) {
                    for (auto& grandchild : child.children) {
                        flat.push_back(&grandchild);
                    }
                }
            }
        }
    }

    return flat;
}

// Find node in tree by flattened index
TreeNode* find_node_by_index(std::vector<TreeNode>& tree, int index) {
    auto flat = flatten_tree(tree);
    if (index >= 0 && index < static_cast<int>(flat.size())) {
        return flat[index];
    }
    return nullptr;
}

// Re-sync selected_index to match selected_node after tree structure changes
void resync_selected_index(InteractiveState* state) {
    if (!state->selected_node) {
        // No node selected, default to first item
        state->selected_index = 0;
        auto flat = flatten_tree(state->tree);
        if (!flat.empty()) {
            state->selected_node = flat[0];
        }
        return;
    }

    // Find where selected_node is in the current flattened tree
    auto flat = flatten_tree(state->tree);
    for (int i = 0; i < static_cast<int>(flat.size()); i++) {
        if (flat[i] == state->selected_node) {
            state->selected_index = i;
            return;
        }
    }

    // Node not found (shouldn't happen), reset to first item
    state->selected_index = 0;
    if (!flat.empty()) {
        state->selected_node = flat[0];
    }
}

// Truncate string to fit within max_len, adding "..." if truncated
std::string truncate_line(const std::string& text, int max_len) {
    if (max_len < 3) return "";
    if (static_cast<int>(text.length()) <= max_len) {
        return text;
    }
    return text.substr(0, max_len - 3) + "...";
}

// Render the tree with scrolling viewport
void render_tree(InteractiveState* state) {
    // Get terminal size
    TermSize term = get_terminal_size();

    // Reserve space for header (4 lines) and footer (3 lines)
    int header_lines = 4;
    int footer_lines = 3;
    int available_lines = term.rows - header_lines - footer_lines;
    if (available_lines < 5) available_lines = 5;  // Minimum viewport

    // Clear screen
    printf("\033[2J\033[H");

    // Header (64 chars wide for proper alignment)
    printf("%s%s╔══════════════════════════════════════════════════════════════╗%s\n",
           ansi::BOLD, ansi::BRIGHT_CYAN, ansi::RESET);
    printf("%s%s║ Moonraker Inspector - Interactive Mode                       ║%s\n",
           ansi::BOLD, ansi::BRIGHT_CYAN, ansi::RESET);
    printf("%s%s╚══════════════════════════════════════════════════════════════╝%s\n",
           ansi::BOLD, ansi::BRIGHT_CYAN, ansi::RESET);
    printf("\n");

    if (!state->data_ready) {
        printf("%sLoading data...%s\n", ansi::YELLOW, ansi::RESET);
        return;
    }

    auto flat_tree = flatten_tree(state->tree);
    int total_items = flat_tree.size();

    // Adjust scroll offset to keep selected item visible
    if (state->selected_index < state->scroll_offset) {
        state->scroll_offset = state->selected_index;
    } else if (state->selected_index >= state->scroll_offset + available_lines) {
        state->scroll_offset = state->selected_index - available_lines + 1;
    }

    // Clamp scroll offset
    int max_scroll = std::max(0, total_items - available_lines);
    if (state->scroll_offset > max_scroll) {
        state->scroll_offset = max_scroll;
    }
    if (state->scroll_offset < 0) {
        state->scroll_offset = 0;
    }

    // Render visible window of nodes
    int end_index = std::min(state->scroll_offset + available_lines, total_items);
    for (int i = state->scroll_offset; i < end_index; i++) {
        TreeNode* node = flat_tree[i];

        // Highlight selected row
        bool selected = (i == state->selected_index);
        if (selected) {
            printf("%s", ansi::BRIGHT_WHITE);
        }

        // Build the line text (without ANSI codes) to calculate length
        std::string indent_str(node->indent_level * 2, ' ');
        std::string line_text;

        if (node->is_section) {
            line_text = indent_str + (node->expanded ? "▼ " : "▶ ") + node->key;
            if (!node->value.empty()) {
                line_text += " - " + node->value;
            }
        } else {
            line_text = indent_str + "  " + node->key;
            if (!node->value.empty()) {
                line_text += ": " + node->value;
            }
        }

        // Add cursor indicator length if selected
        int cursor_len = selected ? 3 : 0;  // " ◀"

        // Truncate if needed (leave room for cursor)
        int max_text_len = term.cols - cursor_len - 1;  // -1 for safety
        std::string display_text = truncate_line(line_text, max_text_len);

        // Now render with proper formatting
        // Print indent
        for (int j = 0; j < node->indent_level; j++) {
            printf("  ");
        }

        // Print icon and content with colors
        if (node->is_section) {
            printf("%s ", node->expanded ? "▼" : "▶");

            // Extract key and value parts for coloring
            std::string key_part = node->key;
            std::string value_part = node->value;

            // Truncate if combined is too long
            int used = indent_str.length() + 2;  // icon + space
            int remaining = max_text_len - used;

            if (!value_part.empty()) {
                int separator_len = 3;  // " - "
                int key_len = key_part.length();
                int val_len = value_part.length();

                if (key_len + separator_len + val_len > remaining) {
                    // Need to truncate
                    if (key_len + separator_len + 3 <= remaining) {
                        // Truncate value
                        value_part = truncate_line(value_part, remaining - key_len - separator_len);
                    } else {
                        // Truncate key, omit value
                        key_part = truncate_line(key_part, remaining);
                        value_part = "";
                    }
                }
            } else {
                key_part = truncate_line(key_part, remaining);
            }

            printf("%s%s%s%s", ansi::BOLD, ansi::CYAN, key_part.c_str(), ansi::RESET);
            if (!value_part.empty()) {
                printf(" %s- %s%s", ansi::DIM, value_part.c_str(), ansi::RESET);
            }
        } else {
            printf("  ");

            // Extract key and value
            std::string key_part = node->key;
            std::string value_part = node->value;

            int used = indent_str.length() + 2;  // "  "
            int remaining = max_text_len - used;

            if (!value_part.empty()) {
                int separator_len = 2;  // ": "
                int key_len = key_part.length();
                int val_len = value_part.length();

                if (key_len + separator_len + val_len > remaining) {
                    if (key_len + separator_len + 3 <= remaining) {
                        value_part = truncate_line(value_part, remaining - key_len - separator_len);
                    } else {
                        key_part = truncate_line(key_part, remaining);
                        value_part = "";
                    }
                }
            } else {
                key_part = truncate_line(key_part, remaining);
            }

            printf("%s%s%s", ansi::BRIGHT_BLUE, key_part.c_str(), ansi::RESET);
            if (!value_part.empty()) {
                printf(": %s%s%s", ansi::WHITE, value_part.c_str(), ansi::RESET);
            }
        }

        if (selected) {
            printf(" ◀");
        }

        printf("%s\n", ansi::RESET);
    }

    // Scroll indicator
    if (total_items > available_lines) {
        int visible_start = state->scroll_offset + 1;
        int visible_end = end_index;
        printf("\n%s[%d-%d of %d items]%s",
               ansi::DIM, visible_start, visible_end, total_items, ansi::RESET);
    }

    // Controls footer
    printf("\n%s────────────────────────────────────────────────────────────────%s\n",
           ansi::DIM, ansi::RESET);
    printf("%s↑/↓%s Navigate  %sEnter/Space%s Expand/Collapse  %sq%s Quit\n",
           ansi::BRIGHT_CYAN, ansi::RESET,
           ansi::BRIGHT_CYAN, ansi::RESET,
           ansi::BRIGHT_CYAN, ansi::RESET);

    fflush(stdout);
}

// Handle keyboard input
void handle_input(InteractiveState* state, char key) {
    auto flat_tree = flatten_tree(state->tree);
    int max_index = flat_tree.size() - 1;

    switch (key) {
        case 'A':  // Up arrow
        case 'k':
            if (state->selected_index > 0) {
                state->selected_index--;
                // Skip over non-expandable items (data items)
                while (state->selected_index > 0) {
                    TreeNode* node = find_node_by_index(state->tree, state->selected_index);
                    if (node && node->is_section) {
                        state->selected_node = node;  // Update selected node
                        break;  // Found a section (expandable), stop here
                    }
                    state->selected_index--;
                }
            }
            break;

        case 'B':  // Down arrow
        case 'j':
            if (state->selected_index < max_index) {
                state->selected_index++;
                // Skip over non-expandable items (data items)
                while (state->selected_index < max_index) {
                    TreeNode* node = find_node_by_index(state->tree, state->selected_index);
                    if (node && node->is_section) {
                        state->selected_node = node;  // Update selected node
                        break;  // Found a section (expandable), stop here
                    }
                    state->selected_index++;
                }
            }
            break;

        case '\n':  // Enter
        case '\r':
        case ' ':   // Space
            {
                TreeNode* node = find_node_by_index(state->tree, state->selected_index);
                if (node && node->is_section) {
                    bool was_expanded = node->expanded;
                    node->expanded = !node->expanded;

                    // If expanding and has object name, query Moonraker for details
                    if (!was_expanded && !node->object_name.empty() && !node->data_fetched && state->client) {
                        query_object_data(node, state->client);
                    }

                    // After expand/collapse, resync selected_index to match selected_node
                    state->selected_node = node;
                    resync_selected_index(state);
                }
            }
            break;
    }
}

// Debug: dump tree structure to verify it's built correctly
void dump_tree_debug(const std::vector<TreeNode>& tree, int indent = 0) {
    for (const auto& node : tree) {
        for (int i = 0; i < indent; i++) printf("  ");
        printf("%s %s", node.is_section ? "[SECTION]" : "[DATA]", node.key.c_str());
        if (!node.value.empty()) {
            printf(" = \"%s\"", node.value.c_str());
        }
        if (!node.object_name.empty()) {
            printf(" (object: %s)", node.object_name.c_str());
        }
        printf("\n");

        if (!node.children.empty()) {
            dump_tree_debug(node.children, indent + 1);
        }
    }
}

// Interactive main loop
int run_interactive(const std::string& ip, int port) {
    InteractiveState state;
    g_state = &state;

    spdlog::set_level(spdlog::level::off);  // Silence logs in interactive mode

    // Initial render
    render_tree(&state);

    // Connect to Moonraker
    std::string url = "ws://" + ip + ":" + std::to_string(port) + "/websocket";
    MoonrakerClient client;
    client.configure_timeouts(5000, 10000, 10000, 200, 2000);
    state.client = &client;  // Store client pointer for querying

    bool connected = false;

    auto on_connect = [&]() {
        connected = true;

        // Query all data
        client.send_jsonrpc("server.info", json::object(),
            [&](json response) {
                if (response.contains("result")) {
                    state.server_info = response["result"];
                }
            },
            [](const MoonrakerError&) {});

        client.send_jsonrpc("printer.info", json::object(),
            [&](json response) {
                if (response.contains("result")) {
                    state.printer_info = response["result"];
                }
            },
            [](const MoonrakerError&) {});

        client.send_jsonrpc("printer.objects.list", json::object(),
            [&](json response) {
                if (response.contains("result")) {
                    state.objects_list = response["result"];
                    state.data_ready = true;
                    build_tree(&state);
                }
            },
            [](const MoonrakerError&) {});
    };

    auto on_disconnect = []() {};

    int result = client.connect(url.c_str(), on_connect, on_disconnect);
    if (result != 0) {
        printf("%sFailed to connect to %s%s\n", ansi::RED, url.c_str(), ansi::RESET);
        return 1;
    }

    // Wait for data to load (for debug mode)
    const char* debug_env = getenv("MOONRAKER_DEBUG_TREE");
    if (debug_env && strcmp(debug_env, "1") == 0) {
        printf("Debug mode: waiting for data...\n");
        int wait_count = 0;
        while (!state.data_ready && wait_count < 50) {  // Wait up to 5 seconds
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            wait_count++;
        }
        if (state.data_ready) {
            printf("\n=== DEBUG: Tree Structure ===\n");
            dump_tree_debug(state.tree);
            printf("=== END DEBUG ===\n\n");
            return 0;  // Exit after dumping
        } else {
            printf("Timed out waiting for data\n");
            return 1;
        }
    }

    // Enable raw terminal mode
    terminal::RawMode raw_mode;
    if (!raw_mode.enable()) {
        printf("%sFailed to enable raw terminal mode%s\n", ansi::RED, ansi::RESET);
        return 1;
    }

    terminal::ansi::hide_cursor();

    // Main event loop
    bool running = true;
    bool need_redraw = true;  // Initial draw needed
    bool last_data_ready = false;

    while (running) {
        // Check if data became ready (trigger initial render)
        if (state.data_ready && !last_data_ready) {
            last_data_ready = true;
            need_redraw = true;
        }

        // Check if async callbacks triggered a redraw
        if (state.need_redraw) {
            need_redraw = true;
            state.need_redraw = false;
        }

        // Only redraw if needed
        if (need_redraw) {
            render_tree(&state);
            need_redraw = false;
        }

        // Check for keyboard input
        char key = raw_mode.read_key();
        if (key != 0) {
            if (key == 'q' || key == 'Q' || key == '\033') {
                running = false;
            } else {
                handle_input(&state, key);
                need_redraw = true;  // User input requires redraw
            }
        }

        // Small delay to prevent CPU spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    terminal::ansi::show_cursor();
    raw_mode.disable();

    printf("\n%sExited interactive mode.%s\n", ansi::GREEN, ansi::RESET);

    return 0;
}
