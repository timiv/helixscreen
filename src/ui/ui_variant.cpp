// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_variant.h"

#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <cstring>

namespace helix::ui {

Variant parse_variant(const char* str) {
    if (!str || str[0] == '\0')
        return Variant::NONE;

    // Ordered by expected frequency
    if (strcmp(str, "success") == 0)
        return Variant::SUCCESS;
    if (strcmp(str, "muted") == 0)
        return Variant::MUTED;
    if (strcmp(str, "danger") == 0)
        return Variant::DANGER;
    if (strcmp(str, "warning") == 0)
        return Variant::WARNING;
    if (strcmp(str, "info") == 0)
        return Variant::INFO;
    if (strcmp(str, "primary") == 0)
        return Variant::PRIMARY;
    if (strcmp(str, "secondary") == 0)
        return Variant::SECONDARY;
    if (strcmp(str, "tertiary") == 0)
        return Variant::TERTIARY;
    if (strcmp(str, "disabled") == 0)
        return Variant::DISABLED;
    if (strcmp(str, "text") == 0)
        return Variant::TEXT;
    if (strcmp(str, "none") == 0)
        return Variant::NONE;

    spdlog::warn("[Variant] Unknown variant '{}', using NONE", str);
    return Variant::NONE;
}

lv_color_t variant_color(Variant v) {
    auto& p = ThemeManager::instance().current_palette();
    switch (v) {
    case Variant::SUCCESS:
        return p.success;
    case Variant::WARNING:
        return p.warning;
    case Variant::DANGER:
        return p.danger;
    case Variant::INFO:
        return p.info;
    case Variant::PRIMARY:
        return p.primary;
    case Variant::SECONDARY:
        return p.secondary;
    case Variant::TERTIARY:
        return p.tertiary;
    case Variant::MUTED:
        return p.text_muted;
    case Variant::DISABLED:
        return p.text;
    case Variant::TEXT:
    case Variant::NONE:
    default:
        return p.text;
    }
}

lv_opa_t variant_opa(Variant v) {
    return (v == Variant::DISABLED) ? LV_OPA_50 : LV_OPA_COVER;
}

void remove_variant_styles(lv_obj_t* obj) {
    auto& tm = ThemeManager::instance();
    lv_style_t* styles[] = {
        tm.get_style(StyleRole::IconText),     tm.get_style(StyleRole::TextMuted),
        tm.get_style(StyleRole::IconPrimary),  tm.get_style(StyleRole::IconSecondary),
        tm.get_style(StyleRole::IconTertiary), tm.get_style(StyleRole::IconSuccess),
        tm.get_style(StyleRole::IconWarning),  tm.get_style(StyleRole::IconDanger),
        tm.get_style(StyleRole::IconInfo),
    };
    for (auto* s : styles) {
        if (s)
            lv_obj_remove_style(obj, s, LV_PART_MAIN);
    }
}

void apply_variant_text_style(lv_obj_t* obj, Variant v) {
    remove_variant_styles(obj);

    lv_color_t color = variant_color(v);
    lv_opa_t opa = variant_opa(v);

    // Use local style (lv_obj_set_style_*) instead of adding style to ensure
    // variant has highest priority, even when lv_xml_obj_apply() or other
    // functions set local style properties (like text_font).
    lv_obj_set_style_text_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_text_opa(obj, opa, LV_PART_MAIN);
}

} // namespace helix::ui
