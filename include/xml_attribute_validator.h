// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace xml_validator {

/**
 * @brief Information extracted from an XML component definition.
 */
struct ComponentInfo {
    std::string extends; ///< Base widget this component extends (e.g., "lv_label")
    std::unordered_set<std::string> props; ///< Props declared in the <api> section
};

/**
 * @brief Database of widget attributes and inheritance relationships.
 */
struct WidgetDatabase {
    /// widget_name -> set of directly declared attributes (not inherited)
    std::unordered_map<std::string, std::unordered_set<std::string>> widget_attrs;

    /// widget_name -> parent widget name (for inheritance)
    std::unordered_map<std::string, std::string> inheritance;
};

/**
 * @brief Extract valid attribute names from an LVGL XML parser C source file.
 *
 * Parses the parser source code to find:
 * - Direct string comparisons: lv_streq("attr_name", name)
 * - SET_STYLE_IF macro usages: SET_STYLE_IF(prop, ...) -> style_prop
 *
 * @param file_content The content of the parser C file
 * @param widget_name The name of the widget (used for logging/debugging)
 * @return Set of valid attribute names for this widget
 */
std::unordered_set<std::string> extract_attributes_from_parser(const std::string& file_content,
                                                               const std::string& widget_name);

/**
 * @brief Extract widget registration information from LVGL source.
 *
 * Finds all lv_xml_register_widget() calls and extracts:
 * - Widget name (e.g., "lv_label", "ui_button")
 * - Apply function name (used to locate the parser)
 *
 * @param file_content Content of the registration source file (e.g., lv_xml.c)
 * @return Vector of (widget_name, apply_function_name) pairs
 */
std::vector<std::pair<std::string, std::string>>
extract_widget_registration(const std::string& file_content);

/**
 * @brief Extract component properties from an XML component file.
 *
 * Parses the XML to find:
 * - <view extends="..."> for inheritance
 * - <prop name="..."> declarations in the <api> section
 *
 * @param xml_content Content of the component XML file
 * @return ComponentInfo with extends and props
 */
ComponentInfo extract_component_props(const std::string& xml_content);

/**
 * @brief Build complete attribute sets including inherited attributes.
 *
 * For each widget in the database, computes the full set of valid attributes
 * by walking up the inheritance chain and merging attribute sets.
 *
 * @param db Widget database with direct attributes and inheritance info
 * @return Map of widget_name -> complete set of valid attributes
 */
std::unordered_map<std::string, std::unordered_set<std::string>>
build_inheritance_tree(const WidgetDatabase& db);

} // namespace xml_validator
