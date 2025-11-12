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

#pragma once

#include "lvgl/lvgl.h"

/**
 * Wizard Moonraker Connection Screen
 *
 * Handles Moonraker WebSocket configuration during first-run wizard:
 * - IP address or hostname entry
 * - Port number configuration (default: 7125)
 * - Connection testing with feedback
 * - Auto-discovery trigger on success
 * - Configuration persistence
 *
 * Initialization Order (CRITICAL):
 *   1. Register XML component (wizard_connection.xml)
 *   2. ui_wizard_connection_init_subjects()
 *   3. ui_wizard_connection_register_callbacks()
 *   4. ui_wizard_connection_create(parent)
 */

/**
 * Initialize connection screen subjects
 *
 * Creates and registers reactive subjects:
 * - connection_ip (string, IP address or hostname)
 * - connection_port (string, port number, default "7125")
 * - connection_status (string, e.g. "Testing...", "✓ Connected", "✗ Failed")
 * - connection_testing (int, 0=idle 1=testing, controls spinner visibility)
 *
 * MUST be called BEFORE creating XML components.
 */
void ui_wizard_connection_init_subjects();

/**
 * Register event callbacks
 *
 * Registers callbacks for:
 * - on_test_connection_clicked (Test Connection button)
 * - on_ip_input_changed (IP address validation)
 * - on_port_input_changed (Port number validation)
 *
 * MUST be called BEFORE creating XML components.
 */
void ui_wizard_connection_register_callbacks();

/**
 * Create Moonraker connection screen
 *
 * Creates the connection UI from wizard_connection.xml.
 * Returns the root connection screen object.
 *
 * Prerequisites:
 * - wizard_connection.xml component registered
 * - ui_wizard_connection_init_subjects() called
 * - ui_wizard_connection_register_callbacks() called
 *
 * @param parent Parent container (typically wizard_content area)
 * @return The connection screen root object, or NULL on failure
 */
lv_obj_t* ui_wizard_connection_create(lv_obj_t* parent);

/**
 * Cleanup connection screen resources
 *
 * Stops any ongoing connection attempts and cleans up resources.
 * Called when leaving the connection setup step.
 */
void ui_wizard_connection_cleanup();

/**
 * Get the configured Moonraker URL
 *
 * Returns the WebSocket URL constructed from the current IP and port values.
 * Format: ws://[ip]:[port]/websocket
 *
 * @param buffer Output buffer for the URL
 * @param size Size of the output buffer
 * @return true if URL was successfully constructed, false if inputs are invalid
 */
bool ui_wizard_connection_get_url(char* buffer, size_t size);

/**
 * Check if connection has been successfully tested
 *
 * Used to determine if the Next button should be enabled.
 *
 * @return true if a successful connection test has been performed
 */
bool ui_wizard_connection_is_validated();