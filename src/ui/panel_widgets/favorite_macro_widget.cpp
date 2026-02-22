// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "favorite_macro_widget.h"

#include "ui_event_safety.h"
#include "ui_icon.h"
#include "ui_update_queue.h"

#include "app_globals.h"
#include "config.h"
#include "macro_param_modal.h"
#include "moonraker_api.h"
#include "panel_widget_config.h"
#include "panel_widget_manager.h"
#include "panel_widget_registry.h"
#include "printer_state.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <regex>

namespace {

// Self-register both widget factories
const bool s_registered_1 = [] {
    helix::register_widget_factory("favorite_macro_1", []() {
        return std::make_unique<helix::FavoriteMacroWidget>("favorite_macro_1");
    });
    return true;
}();

const bool s_registered_2 = [] {
    helix::register_widget_factory("favorite_macro_2", []() {
        return std::make_unique<helix::FavoriteMacroWidget>("favorite_macro_2");
    });
    return true;
}();

} // namespace

using namespace helix;

// File-local helper: get the shared PanelWidgetConfig instance
static helix::PanelWidgetConfig& get_widget_config_ref() {
    static helix::PanelWidgetConfig config("home", *Config::get_instance());
    config.load();
    return config;
}

// ============================================================================
// parse_macro_params — pure function
// ============================================================================

std::vector<MacroParam> helix::parse_macro_params(const std::string& gcode_template) {
    std::vector<MacroParam> result;
    std::unordered_set<std::string> seen;

    // Match params.NAME or params['NAME'] or params["NAME"]
    // Optionally followed by |default(VALUE)
    // Pattern 1: params.NAME (dot access)
    std::regex dot_re(R"(params\.([A-Z_][A-Z_0-9]*))", std::regex::icase);
    // Pattern 2: params['NAME'] or params["NAME"] (bracket access)
    std::regex bracket_re(R"(params\[['"]([A-Za-z_][A-Za-z_0-9]*)['"]\])", std::regex::icase);
    // Default value extraction: |default(VALUE) or | default(VALUE)
    // Applied contextually after finding the param reference
    std::regex default_re(R"(\|\s*default\(([^)]*)\))");

    auto process_matches = [&](const std::regex& re) {
        auto begin = std::sregex_iterator(gcode_template.begin(), gcode_template.end(), re);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            std::string param_name = (*it)[1].str();

            // Normalize to uppercase
            std::transform(param_name.begin(), param_name.end(), param_name.begin(), ::toupper);

            // Skip duplicates
            if (seen.count(param_name)) {
                continue;
            }
            seen.insert(param_name);

            // Try to extract default value from surrounding context
            std::string default_value;
            auto match_pos = (*it).position();
            auto match_len = (*it).length();

            // Look at the rest of the line/expression after the param reference
            // for |default(VALUE) pattern
            size_t context_start = match_pos + match_len;
            size_t context_end = std::min(context_start + 100, gcode_template.size());
            std::string context = gcode_template.substr(context_start, context_end - context_start);

            std::smatch default_match;
            if (std::regex_search(context, default_match, default_re)) {
                default_value = default_match[1].str();
                // Strip surrounding quotes from default value if present
                if (default_value.size() >= 2 &&
                    ((default_value.front() == '\'' && default_value.back() == '\'') ||
                     (default_value.front() == '"' && default_value.back() == '"'))) {
                    default_value = default_value.substr(1, default_value.size() - 2);
                }
            }

            result.push_back({param_name, default_value});
        }
    };

    process_matches(dot_re);
    process_matches(bracket_re);

    return result;
}

// ============================================================================
// FavoriteMacroWidget
// ============================================================================

FavoriteMacroWidget* FavoriteMacroWidget::s_active_picker_ = nullptr;

FavoriteMacroWidget::FavoriteMacroWidget(const std::string& widget_id) : widget_id_(widget_id) {}

FavoriteMacroWidget::~FavoriteMacroWidget() {
    detach();
}

void FavoriteMacroWidget::attach(lv_obj_t* widget_obj, lv_obj_t* parent_screen) {
    widget_obj_ = widget_obj;
    parent_screen_ = parent_screen;
    *alive_ = true;

    if (widget_obj_) {
        lv_obj_set_user_data(widget_obj_, this);
    }

    // Cache label pointers
    icon_label_ = lv_obj_find_by_name(widget_obj_, "fav_macro_icon");
    name_label_ = lv_obj_find_by_name(widget_obj_, "fav_macro_name");

    // Load saved macro selection from config
    load_config();
    update_display();

    spdlog::debug("[FavoriteMacroWidget:{}] Attached (macro: {})", widget_id_,
                  macro_name_.empty() ? "none" : macro_name_);
}

void FavoriteMacroWidget::detach() {
    *alive_ = false;
    dismiss_macro_picker();

    if (widget_obj_) {
        lv_obj_set_user_data(widget_obj_, nullptr);
        widget_obj_ = nullptr;
    }
    parent_screen_ = nullptr;
    icon_label_ = nullptr;
    name_label_ = nullptr;

    spdlog::debug("[FavoriteMacroWidget:{}] Detached", widget_id_);
}

MoonrakerAPI* FavoriteMacroWidget::get_api() const {
    return PanelWidgetManager::instance().shared_resource<MoonrakerAPI>();
}

void FavoriteMacroWidget::handle_clicked() {
    if (macro_name_.empty()) {
        spdlog::info("[FavoriteMacroWidget:{}] No macro configured, showing picker", widget_id_);
        show_macro_picker();
    } else {
        spdlog::info("[FavoriteMacroWidget:{}] Executing macro: {}", widget_id_, macro_name_);
        fetch_and_execute();
    }
}

void FavoriteMacroWidget::handle_long_press() {
    spdlog::info("[FavoriteMacroWidget:{}] Long press, showing picker", widget_id_);
    show_macro_picker();
}

void FavoriteMacroWidget::update_display() {
    if (name_label_) {
        if (macro_name_.empty()) {
            lv_label_set_text(name_label_, "Configure");
        } else {
            // Show prettified macro name
            std::string display = macro_name_;
            // Replace underscores with spaces for readability
            std::replace(display.begin(), display.end(), '_', ' ');
            lv_label_set_text(name_label_, display.c_str());
        }
    }
}

void FavoriteMacroWidget::load_config() {
    auto& wc = get_widget_config_ref();
    auto config = wc.get_widget_config(widget_id_);
    if (config.contains("macro") && config["macro"].is_string()) {
        macro_name_ = config["macro"].get<std::string>();
        spdlog::debug("[FavoriteMacroWidget:{}] Loaded config: macro={}", widget_id_, macro_name_);
    }
}

void FavoriteMacroWidget::save_config() {
    nlohmann::json config;
    if (!macro_name_.empty()) {
        config["macro"] = macro_name_;
    }

    auto& wc = get_widget_config_ref();
    wc.set_widget_config(widget_id_, config);

    spdlog::debug("[FavoriteMacroWidget:{}] Saved config: macro={}", widget_id_, macro_name_);
}

void FavoriteMacroWidget::select_macro(const std::string& name) {
    macro_name_ = name;
    params_cached_ = false; // Reset cache for new macro
    cached_params_.clear();
    update_display();
    save_config();

    spdlog::info("[FavoriteMacroWidget:{}] Selected macro: {}", widget_id_, name);
}

void FavoriteMacroWidget::fetch_and_execute() {
    MoonrakerAPI* api = get_api();
    if (!api) {
        spdlog::warn("[FavoriteMacroWidget:{}] No API available", widget_id_);
        return;
    }

    // If params already cached: execute directly or show modal
    if (params_cached_) {
        if (cached_params_.empty()) {
            // No params needed, execute directly
            execute_with_params({});
        } else {
            // Show param modal
            static MacroParamModal modal;
            std::weak_ptr<bool> weak_alive = alive_;
            FavoriteMacroWidget* self = this;
            modal.show_for_macro(
                lv_screen_active(), macro_name_, cached_params_,
                [weak_alive, self](const std::map<std::string, std::string>& params) {
                    auto alive = weak_alive.lock();
                    if (!alive || !*alive)
                        return;
                    self->execute_with_params(params);
                });
        }
        return;
    }

    // Fetch configfile to discover params
    std::weak_ptr<bool> weak_alive = alive_;
    FavoriteMacroWidget* self = this;
    std::string macro_name_copy = macro_name_;

    api->query_configfile(
        [weak_alive, self, macro_name_copy](const nlohmann::json& config) {
            // Background thread — queue UI update
            helix::ui::queue_update([weak_alive, self, macro_name_copy, config]() {
                auto alive = weak_alive.lock();
                if (!alive || !*alive)
                    return;

                // Look for gcode_macro section
                std::string section_key = "gcode_macro " + macro_name_copy;
                // Try lowercase version too (Klipper normalizes to lowercase)
                std::string section_key_lower = section_key;
                std::transform(section_key_lower.begin(), section_key_lower.end(),
                               section_key_lower.begin(), ::tolower);

                std::string gcode_template;
                if (config.contains(section_key_lower) &&
                    config[section_key_lower].contains("gcode")) {
                    gcode_template = config[section_key_lower]["gcode"].get<std::string>();
                } else if (config.contains(section_key) && config[section_key].contains("gcode")) {
                    gcode_template = config[section_key]["gcode"].get<std::string>();
                }

                self->cached_params_ = helix::parse_macro_params(gcode_template);
                self->params_cached_ = true;

                spdlog::debug("[FavoriteMacroWidget:{}] Parsed {} params for {}", self->widget_id_,
                              self->cached_params_.size(), macro_name_copy);

                // Now retry execution with cached params
                self->fetch_and_execute();
            });
        },
        [weak_alive, self, macro_name_copy](const MoonrakerError& err) {
            helix::ui::queue_update([weak_alive, self, macro_name_copy, err]() {
                auto alive = weak_alive.lock();
                if (!alive || !*alive)
                    return;

                spdlog::warn("[FavoriteMacroWidget:{}] Failed to query configfile: {}",
                             self->widget_id_, err.message);

                // Execute without params as fallback
                self->cached_params_.clear();
                self->params_cached_ = true;
                self->execute_with_params({});
            });
        });
}

void FavoriteMacroWidget::execute_with_params(const std::map<std::string, std::string>& params) {
    MoonrakerAPI* api = get_api();
    if (!api) {
        spdlog::warn("[FavoriteMacroWidget:{}] No API available", widget_id_);
        return;
    }

    std::weak_ptr<bool> weak_alive = alive_;
    std::string widget_id = widget_id_;

    api->advanced().execute_macro(
        macro_name_, params,
        [weak_alive, widget_id]() {
            helix::ui::queue_update([weak_alive, widget_id]() {
                auto alive = weak_alive.lock();
                if (!alive || !*alive)
                    return;
                spdlog::info("[FavoriteMacroWidget:{}] Macro executed successfully", widget_id);
            });
        },
        [weak_alive, widget_id](const MoonrakerError& err) {
            helix::ui::queue_update([weak_alive, widget_id, err]() {
                auto alive = weak_alive.lock();
                if (!alive || !*alive)
                    return;
                spdlog::error("[FavoriteMacroWidget:{}] Macro execution failed: {}", widget_id,
                              err.message);
                helix::ui::modal_show_alert("Macro Failed", err.message.c_str(),
                                            ModalSeverity::Error);
            });
        });
}

// ============================================================================
// Macro Picker
// ============================================================================

void FavoriteMacroWidget::show_macro_picker() {
    if (picker_backdrop_ || !parent_screen_) {
        return;
    }

    MoonrakerAPI* api = get_api();
    if (!api) {
        spdlog::warn("[FavoriteMacroWidget:{}] No API available for picker", widget_id_);
        return;
    }

    const auto& macros = api->hardware().macros();
    if (macros.empty()) {
        spdlog::warn("[FavoriteMacroWidget:{}] No macros available", widget_id_);
        return;
    }

    // Sort macros alphabetically, filter out system macros
    std::vector<std::string> sorted_macros;
    for (const auto& m : macros) {
        if (!m.empty() && m[0] != '_') {
            sorted_macros.push_back(m);
        }
    }
    std::sort(sorted_macros.begin(), sorted_macros.end());

    if (sorted_macros.empty()) {
        spdlog::warn("[FavoriteMacroWidget:{}] No user macros available", widget_id_);
        return;
    }

    // Create picker from XML
    picker_backdrop_ =
        static_cast<lv_obj_t*>(lv_xml_create(parent_screen_, "favorite_macro_picker", nullptr));
    if (!picker_backdrop_) {
        spdlog::error("[FavoriteMacroWidget:{}] Failed to create picker from XML", widget_id_);
        return;
    }

    // Find the macro_list container
    lv_obj_t* macro_list = lv_obj_find_by_name(picker_backdrop_, "macro_list");
    if (!macro_list) {
        spdlog::error("[FavoriteMacroWidget:{}] macro_list not found in picker XML", widget_id_);
        lv_obj_delete(picker_backdrop_);
        picker_backdrop_ = nullptr;
        return;
    }

    // Populate macro rows
    for (const auto& macro : sorted_macros) {
        bool is_selected = (macro == macro_name_);

        lv_obj_t* row = lv_obj_create(macro_list);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(row, 6, 0);
        lv_obj_set_style_pad_gap(row, 4, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

        // Highlight selected row
        lv_obj_set_style_bg_opa(row, is_selected ? 30 : 0, 0);

        // Macro display name (prettified)
        std::string display = macro;
        std::replace(display.begin(), display.end(), '_', ' ');

        lv_obj_t* name = lv_label_create(row);
        lv_label_set_text(name, display.c_str());
        lv_obj_set_flex_grow(name, 1);
        lv_label_set_long_mode(name, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_font(name, lv_font_get_default(), 0);

        // Store macro name as user_data for click handler
        auto* macro_name_copy = new std::string(macro);
        lv_obj_set_user_data(row, macro_name_copy);

        lv_obj_add_event_cb(
            row,
            [](lv_event_t* e) {
                LVGL_SAFE_EVENT_CB_BEGIN("[FavoriteMacroWidget] macro_row_cb");
                auto* target = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
                auto* name_ptr = static_cast<std::string*>(lv_obj_get_user_data(target));
                if (!name_ptr)
                    return;

                if (FavoriteMacroWidget::s_active_picker_) {
                    std::string selected = *name_ptr;
                    FavoriteMacroWidget::s_active_picker_->select_macro(selected);
                    FavoriteMacroWidget::s_active_picker_->dismiss_macro_picker();
                }
                LVGL_SAFE_EVENT_CB_END();
            },
            LV_EVENT_CLICKED, nullptr);
    }

    s_active_picker_ = this;

    // Position the context menu card near the widget
    lv_obj_t* card = lv_obj_find_by_name(picker_backdrop_, "context_menu");
    if (card && widget_obj_) {
        int screen_w = lv_obj_get_width(parent_screen_);
        int screen_h = lv_obj_get_height(parent_screen_);

        lv_area_t widget_area;
        lv_obj_get_coords(widget_obj_, &widget_area);

        int card_w = 220;
        int card_x = (widget_area.x1 + widget_area.x2) / 2 - card_w / 2;
        int card_y = widget_area.y2 + 4;

        // Clamp to screen bounds
        if (card_x < 4)
            card_x = 4;
        if (card_x + card_w > screen_w - 4)
            card_x = screen_w - card_w - 4;
        if (card_y + 250 > screen_h) {
            card_y = widget_area.y1 - 250 - 4;
            if (card_y < 4)
                card_y = 4;
        }

        lv_obj_set_pos(card, card_x, card_y);
    }

    spdlog::debug("[FavoriteMacroWidget:{}] Picker shown with {} macros", widget_id_,
                  sorted_macros.size());
}

void FavoriteMacroWidget::dismiss_macro_picker() {
    if (!picker_backdrop_) {
        return;
    }

    // Clean up heap-allocated macro name strings
    lv_obj_t* macro_list = lv_obj_find_by_name(picker_backdrop_, "macro_list");
    if (macro_list) {
        uint32_t count = lv_obj_get_child_count(macro_list);
        for (uint32_t i = 0; i < count; ++i) {
            lv_obj_t* row = lv_obj_get_child(macro_list, i);
            auto* name_ptr = static_cast<std::string*>(lv_obj_get_user_data(row));
            delete name_ptr;
            lv_obj_set_user_data(row, nullptr);
        }
    }

    lv_obj_delete(picker_backdrop_);
    picker_backdrop_ = nullptr;
    s_active_picker_ = nullptr;

    spdlog::debug("[FavoriteMacroWidget:{}] Picker dismissed", widget_id_);
}

// ============================================================================
// Static Callbacks
// ============================================================================

void FavoriteMacroWidget::clicked_1_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[FavoriteMacroWidget] clicked_1_cb");
    auto* widget = panel_widget_from_event<FavoriteMacroWidget>(e);
    if (widget) {
        widget->handle_clicked();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void FavoriteMacroWidget::long_press_1_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[FavoriteMacroWidget] long_press_1_cb");
    auto* widget = panel_widget_from_event<FavoriteMacroWidget>(e);
    if (widget) {
        widget->handle_long_press();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void FavoriteMacroWidget::clicked_2_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[FavoriteMacroWidget] clicked_2_cb");
    auto* widget = panel_widget_from_event<FavoriteMacroWidget>(e);
    if (widget) {
        widget->handle_clicked();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void FavoriteMacroWidget::long_press_2_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[FavoriteMacroWidget] long_press_2_cb");
    auto* widget = panel_widget_from_event<FavoriteMacroWidget>(e);
    if (widget) {
        widget->handle_long_press();
    }
    LVGL_SAFE_EVENT_CB_END();
}

void FavoriteMacroWidget::picker_backdrop_cb(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[FavoriteMacroWidget] picker_backdrop_cb");
    (void)e;
    if (s_active_picker_) {
        s_active_picker_->dismiss_macro_picker();
    }
    LVGL_SAFE_EVENT_CB_END();
}
