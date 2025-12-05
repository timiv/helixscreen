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
 * Semantic text widgets for theme-aware typography
 *
 * These widgets create labels with automatic font and color styling based on
 * semantic meaning, reading from globals.xml theme constants.
 *
 * Available widgets:
 * - <text_heading>: Section headings, large text (20/26/28 + header_text)
 * - <text_body>:    Standard body text (14/18/20 + text_primary)
 * - <text_small>:   Small/helper text (12/16/18 + text_secondary)
 * - <text_xs>:      Extra-small text (10/12/14 + text_secondary) for compact metadata
 *
 * All widgets inherit standard lv_label attributes (text, width, align, etc.)
 */

/**
 * Initialize semantic text widgets for XML usage
 * Registers custom widgets: text_heading, text_body, text_small, text_xs
 *
 * Call this after globals.xml is registered but before creating UI.
 */
void ui_text_init();
