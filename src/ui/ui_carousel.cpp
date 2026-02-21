// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_carousel.h"

#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"
#include "lvgl/src/xml/lv_xml_parser.h"
#include "lvgl/src/xml/lv_xml_utils.h"
#include "lvgl/src/xml/lv_xml_widget.h"
#include "lvgl/src/xml/parsers/lv_xml_obj_parser.h"

#include <spdlog/spdlog.h>

#include <cstring>

namespace {

/**
 * @brief DELETE event handler — cleans up CarouselState and auto-scroll timer
 */
void carousel_delete_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    CarouselState* state = static_cast<CarouselState*>(lv_obj_get_user_data(obj));
    if (!state || state->magic != CarouselState::MAGIC) {
        return;
    }

    if (state->auto_timer) {
        lv_timer_delete(state->auto_timer);
        state->auto_timer = nullptr;
    }

    spdlog::trace("[ui_carousel] Deleting carousel state ({} tiles)", state->real_tiles.size());
    delete state;
    lv_obj_set_user_data(obj, nullptr);
}

/**
 * @brief XML create callback for <ui_carousel> widget
 *
 * Creates a vertical container with:
 * - Horizontal scroll container with snap-to-start behavior (for pages)
 * - Indicator row at the bottom (for page dots)
 *
 * @param state XML parser state
 * @param attrs XML attributes
 * @return Created carousel container object
 */
void* ui_carousel_create(lv_xml_parser_state_t* state, const char** /*attrs*/) {
    lv_obj_t* parent = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));

    // Outer container: column layout holding scroll area + indicators
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(container, 4, LV_PART_MAIN);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);

    // Scroll container: horizontal, full width, snaps to pages
    lv_obj_t* scroll = lv_obj_create(container);
    lv_obj_set_size(scroll, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(scroll, 0, LV_PART_MAIN);
    lv_obj_set_scroll_snap_x(scroll, LV_SCROLL_SNAP_START);
    lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scroll_dir(scroll, LV_DIR_HOR);
    lv_obj_set_style_border_width(scroll, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, LV_PART_MAIN);

    // Indicator row: centered dots at bottom
    lv_obj_t* indicator_row = lv_obj_create(container);
    lv_obj_set_size(indicator_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(indicator_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indicator_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(indicator_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(indicator_row, 6, LV_PART_MAIN);
    lv_obj_remove_flag(indicator_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(indicator_row, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(indicator_row, LV_OPA_TRANSP, LV_PART_MAIN);

    // Allocate and store carousel state
    CarouselState* cstate = new CarouselState();
    cstate->scroll_container = scroll;
    cstate->indicator_row = indicator_row;
    lv_obj_set_user_data(container, cstate);

    // Register delete handler for cleanup
    lv_obj_add_event_cb(container, carousel_delete_cb, LV_EVENT_DELETE, nullptr);

    spdlog::trace("[ui_carousel] Created carousel widget");
    return container;
}

/**
 * @brief XML apply callback for <ui_carousel> widget
 *
 * Parses custom attributes: wrap, auto_scroll_ms, show_indicators, current_page_subject
 * Delegates standard attributes to lv_xml_obj_apply.
 *
 * @param state XML parser state
 * @param attrs XML attributes
 */
void ui_carousel_apply(lv_xml_parser_state_t* state, const char** attrs) {
    // Apply standard object properties first (size, position, style, etc.)
    lv_xml_obj_apply(state, attrs);

    void* item = lv_xml_state_get_item(state);
    lv_obj_t* container = static_cast<lv_obj_t*>(item);
    CarouselState* cstate = ui_carousel_get_state(container);
    if (!cstate) {
        return;
    }

    // Parse wrap attribute (default: true)
    const char* wrap_str = lv_xml_get_value_of(attrs, "wrap");
    if (wrap_str) {
        cstate->wrap = (strcmp(wrap_str, "true") == 0 || strcmp(wrap_str, "1") == 0);
    }

    // Parse auto_scroll_ms attribute (default: 0 = disabled)
    const char* auto_str = lv_xml_get_value_of(attrs, "auto_scroll_ms");
    if (auto_str) {
        cstate->auto_scroll_ms = atoi(auto_str);
    }

    // Parse show_indicators attribute (default: true)
    const char* ind_str = lv_xml_get_value_of(attrs, "show_indicators");
    if (ind_str) {
        cstate->show_indicators = (strcmp(ind_str, "true") == 0 || strcmp(ind_str, "1") == 0);
    }

    // Hide indicator row if indicators are disabled
    if (!cstate->show_indicators && cstate->indicator_row) {
        lv_obj_add_flag(cstate->indicator_row, LV_OBJ_FLAG_HIDDEN);
    }

    // Parse current_page_subject for subject binding
    const char* subject_name = lv_xml_get_value_of(attrs, "current_page_subject");
    if (subject_name && subject_name[0] != '\0') {
        lv_subject_t* subject = lv_xml_get_subject(&state->scope, subject_name);
        if (subject) {
            cstate->page_subject = subject;
            spdlog::trace("[ui_carousel] Bound to page subject '{}'", subject_name);
        } else {
            spdlog::warn("[ui_carousel] Subject '{}' not found", subject_name);
        }
    }

    spdlog::trace("[ui_carousel] Applied: wrap={} auto_scroll={}ms indicators={}", cstate->wrap,
                  cstate->auto_scroll_ms, cstate->show_indicators);
}

} // namespace

void ui_carousel_init() {
    lv_xml_register_widget("ui_carousel", ui_carousel_create, ui_carousel_apply);
    spdlog::trace("[ui_carousel] Registered carousel widget");
}

CarouselState* ui_carousel_get_state(lv_obj_t* obj) {
    if (!obj) {
        return nullptr;
    }
    CarouselState* state = static_cast<CarouselState*>(lv_obj_get_user_data(obj));
    if (!state || state->magic != CarouselState::MAGIC) {
        return nullptr;
    }
    return state;
}

void ui_carousel_goto_page(lv_obj_t* carousel, int page, bool animate) {
    CarouselState* state = ui_carousel_get_state(carousel);
    if (!state || !state->scroll_container) {
        return;
    }

    int count = static_cast<int>(state->real_tiles.size());
    if (count == 0 || page < 0 || page >= count) {
        return;
    }

    // Calculate scroll position based on page width
    int32_t container_w = lv_obj_get_content_width(state->scroll_container);
    int32_t scroll_x = page * container_w;

    lv_obj_scroll_to_x(state->scroll_container, scroll_x, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    state->current_page = page;

    // Update page subject if bound
    if (state->page_subject) {
        lv_subject_set_int(state->page_subject, page);
    }

    spdlog::trace("[ui_carousel] Navigated to page {}/{}", page, count);
}

int ui_carousel_get_current_page(lv_obj_t* carousel) {
    CarouselState* state = ui_carousel_get_state(carousel);
    if (!state) {
        return 0;
    }
    return state->current_page;
}

int ui_carousel_get_page_count(lv_obj_t* carousel) {
    CarouselState* state = ui_carousel_get_state(carousel);
    if (!state) {
        return 0;
    }
    return static_cast<int>(state->real_tiles.size());
}

void ui_carousel_add_item(lv_obj_t* carousel, lv_obj_t* item) {
    CarouselState* state = ui_carousel_get_state(carousel);
    if (!state || !state->scroll_container || !item) {
        return;
    }

    // Create a tile container inside the scroll area
    lv_obj_t* tile = lv_obj_create(state->scroll_container);
    lv_obj_set_size(tile, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(tile, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(tile, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tile, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tile, LV_OBJ_FLAG_SNAPPABLE);

    // Reparent the item into the tile
    lv_obj_set_parent(item, tile);

    // Track the tile
    state->real_tiles.push_back(tile);

    spdlog::trace("[ui_carousel] Added item, page count now {}", state->real_tiles.size());
}

void ui_carousel_rebuild_indicators(lv_obj_t* carousel) {
    CarouselState* state = ui_carousel_get_state(carousel);
    if (!state || !state->indicator_row) {
        return;
    }

    // Stub: will be implemented in Task 4-5 with actual indicator dots
    spdlog::trace("[ui_carousel] Rebuild indicators stub ({} pages)", state->real_tiles.size());
}
