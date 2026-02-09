// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_markdown.h"

#include "lv_markdown.h"
#include "lv_markdown_style.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"
#include "lvgl/src/xml/lv_xml_parser.h"
#include "lvgl/src/xml/lv_xml_utils.h"
#include "lvgl/src/xml/lv_xml_widget.h"
#include "lvgl/src/xml/parsers/lv_xml_obj_parser.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <cstring>

namespace {

/// User data attached to each ui_markdown widget for RAII cleanup
struct UiMarkdownData {
    static constexpr uint32_t MAGIC = 0x4D444F57; // "MDOW"
    uint32_t magic{MAGIC};
    lv_markdown_style_t style{};
};

/**
 * @brief Build a theme-aware markdown style from current design tokens
 */
void build_theme_style(lv_markdown_style_t& style) {
    lv_markdown_style_init(&style);

    // Body text — use default font, theme text color
    style.body_font = LV_FONT_DEFAULT;
    style.body_color = theme_manager_get_color("text");

    // Headings — all use text color (no separate heading color token)
    for (int i = 0; i < 6; i++) {
        style.heading_font[i] = nullptr; // fallback to body_font
        style.heading_color[i] = theme_manager_get_color("text");
    }

    // Emphasis — NULL triggers faux bold (letter spacing) and underline fallbacks
    style.bold_font = nullptr;
    style.italic_font = nullptr;
    style.bold_italic_font = nullptr;

    // Code styling
    style.code_font = nullptr; // fallback to body_font
    style.code_color = theme_manager_get_color("text");
    style.code_bg_color = theme_manager_get_color("card_bg");
    style.code_block_bg_color = theme_manager_get_color("card_bg");

    // Blockquote and horizontal rule
    style.blockquote_border_color = theme_manager_get_color("text_muted");
    style.hr_color = theme_manager_get_color("text_muted");

    // Spacing
    style.paragraph_spacing = 8;
    style.line_spacing = 4;
}

/**
 * @brief Observer callback for bind_text subject changes
 *
 * Updates the markdown content when the bound string subject changes.
 */
void markdown_text_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    lv_obj_t* md_widget = lv_observer_get_target_obj(observer);
    if (!md_widget || !lv_obj_is_valid(md_widget)) {
        return;
    }

    const char* text = lv_subject_get_string(subject);
    lv_markdown_set_text(md_widget, text);
    spdlog::trace("[ui_markdown] Observer updated text ({} bytes)", text ? strlen(text) : 0);
}

/**
 * @brief Delete callback — frees UiMarkdownData
 */
void markdown_delete_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    auto* data = static_cast<UiMarkdownData*>(lv_obj_get_user_data(obj));
    if (data && data->magic == UiMarkdownData::MAGIC) {
        delete data;
        lv_obj_set_user_data(obj, nullptr);
    }
}

/**
 * @brief XML create callback for <ui_markdown> widget
 *
 * Creates a markdown viewer with theme-aware styling and RAII cleanup.
 */
void* ui_markdown_create(lv_xml_parser_state_t* state, const char** /*attrs*/) {
    lv_obj_t* parent = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));

    // Create the underlying markdown widget
    lv_obj_t* obj = lv_markdown_create(parent);

    // Build theme-aware style and apply it
    auto* data = new UiMarkdownData{};
    build_theme_style(data->style);
    lv_markdown_set_style(obj, &data->style);

    // Store user data for cleanup
    lv_obj_set_user_data(obj, data);

    // Register delete callback for RAII cleanup
    lv_obj_add_event_cb(obj, markdown_delete_cb, LV_EVENT_DELETE, nullptr);

    spdlog::trace("[ui_markdown] Created markdown widget");
    return obj;
}

/**
 * @brief XML apply callback for <ui_markdown> widget
 *
 * Handles standard object properties plus:
 * - bind_text: binds to a string subject for dynamic markdown content
 * - text: sets static markdown content
 */
void ui_markdown_apply(lv_xml_parser_state_t* state, const char** attrs) {
    // Apply base object attributes (width, height, align, hidden, etc.)
    lv_xml_obj_apply(state, attrs);

    void* item = lv_xml_state_get_item(state);
    lv_obj_t* obj = static_cast<lv_obj_t*>(item);

    // Handle bind_text — bind to a string subject
    const char* bind_text = lv_xml_get_value_of(attrs, "bind_text");
    if (bind_text) {
        lv_subject_t* subject = lv_xml_get_subject(&state->scope, bind_text);
        if (subject) {
            lv_subject_add_observer_obj(subject, markdown_text_observer_cb, obj, nullptr);
            spdlog::trace("[ui_markdown] Bound to subject '{}'", bind_text);
        } else {
            spdlog::warn("[ui_markdown] Subject '{}' not found for bind_text", bind_text);
        }
    }

    // Handle static text attribute
    const char* text = lv_xml_get_value_of(attrs, "text");
    if (text) {
        lv_markdown_set_text(obj, text);
        spdlog::trace("[ui_markdown] Set static text ({} bytes)", strlen(text));
    }
}

} // namespace

void ui_markdown_init() {
    lv_xml_register_widget("ui_markdown", ui_markdown_create, ui_markdown_apply);
    spdlog::trace("[ui_markdown] Registered markdown widget");
}
