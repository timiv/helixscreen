// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file validate_xml_constants.cpp
 * @brief Validates XML constant sets are complete (responsive px, theme colors)
 *
 * Pre-commit validation tool that checks for incomplete constant sets:
 * - Responsive px: If ANY of foo_small, foo_medium, foo_large exist but NOT ALL -> warn
 * - Themed colors: If ONLY bar_light OR ONLY bar_dark exists -> warn
 *
 * Usage: validate-xml-constants [directory]
 *   directory - Path to XML directory (default: ui_xml)
 *
 * Exit codes:
 *   0 - All constant sets complete
 *   1 - Found incomplete constant sets
 */

#include "theme_manager.h"

#include <iostream>

int main(int argc, char* argv[]) {
    const char* directory = (argc > 1) ? argv[1] : "ui_xml";
    auto warnings = theme_manager_validate_constant_sets(directory);

    if (warnings.empty()) {
        std::cout << "All XML constant sets are complete" << std::endl;
        return 0;
    }

    std::cerr << "Found " << warnings.size() << " incomplete constant set(s):" << std::endl;
    for (const auto& warning : warnings) {
        std::cerr << "   " << warning << std::endl;
    }
    return 1;
}
