// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors
/**
 * @file lvgl_assert_handler.h
 * @brief Custom LVGL assert handler for debugging
 *
 * This handler logs assertion failures with file/line/function info
 * instead of the default while(1) infinite loop, allowing better
 * diagnosis of issues like the lv_inv_area rendering assertion.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Custom LVGL assert handler that logs and optionally continues
 *
 * Called when LV_ASSERT fails. Logs detailed information about the
 * assertion location to help diagnose issues.
 *
 * @param file Source file where assertion failed
 * @param line Line number where assertion failed
 * @param func Function name where assertion failed
 */
static inline void helix_lvgl_assert_handler(const char* file, int line, const char* func) {
    // Get timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    // Log to stderr (captured by syslog on embedded)
    fprintf(stderr,
            "\n"
            "╔══════════════════════════════════════════════════════════════╗\n"
            "║                    LVGL ASSERTION FAILED                     ║\n"
            "╠══════════════════════════════════════════════════════════════╣\n"
            "║ Time: %-54s ║\n"
            "║ File: %-54s ║\n"
            "║ Line: %-54d ║\n"
            "║ Func: %-54s ║\n"
            "╠══════════════════════════════════════════════════════════════╣\n"
            "║ This is likely caused by lv_obj_invalidate() being called    ║\n"
            "║ during the LVGL render phase. Check for:                     ║\n"
            "║   1. Subject observers that call invalidate                  ║\n"
            "║   2. Timer callbacks during render                           ║\n"
            "║   3. Race conditions in async callbacks                      ║\n"
            "╚══════════════════════════════════════════════════════════════╝\n",
            time_buf, file, line, func);
    fflush(stderr);

    // Also try to write to log file for embedded systems
    FILE* log = fopen("/tmp/helix_assert.log", "a");
    if (log) {
        fprintf(log, "[%s] LVGL ASSERT at %s:%d in %s()\n", time_buf, file, line, func);
        fflush(log);
        fclose(log);
    }

    // IMPORTANT: We continue instead of halting to allow the app to keep running
    // and potentially log more context. However, LVGL state may be corrupted.
    // In production, you might want to trigger a graceful restart here.

    // Uncomment to halt (original behavior):
    // while(1);

    // Uncomment to abort (generates core dump for debugging):
    // abort();
}

#ifdef __cplusplus
}
#endif
