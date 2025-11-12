// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * LVGL 8 to LVGL 9 compatibility layer for Material Design icons
 * These icons were generated for LVGL 8, but we're using LVGL 9
 */

#pragma once

#include "lvgl/lvgl.h"

// LVGL 8 used these constants, LVGL 9 changed them
#ifndef LV_IMG_CF_TRUE_COLOR_ALPHA
#define LV_IMG_CF_TRUE_COLOR_ALPHA LV_COLOR_FORMAT_ARGB8888
#endif

#ifndef LV_IMG_PX_SIZE_ALPHA_BYTE
#define LV_IMG_PX_SIZE_ALPHA_BYTE 1
#endif
