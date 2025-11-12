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

#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

// Forward declarations
class MoonrakerClient;
class MoonrakerAPI;
class PrinterState;

/**
 * @brief Get global MoonrakerClient instance
 * @return Pointer to global MoonrakerClient (may be nullptr if not initialized)
 */
MoonrakerClient* get_moonraker_client();

/**
 * @brief Get global MoonrakerAPI instance
 * @return Pointer to global MoonrakerAPI (may be nullptr if not initialized)
 */
MoonrakerAPI* get_moonraker_api();

/**
 * @brief Get global PrinterState instance
 * @return Reference to global PrinterState (always valid)
 */
PrinterState& get_printer_state();

#endif // APP_GLOBALS_H