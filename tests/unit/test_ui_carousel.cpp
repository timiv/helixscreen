// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_ui_carousel.cpp
 * @brief Unit tests for ui_carousel XML widget
 *
 * Tests carousel creation, state management, and tile management.
 */

#include "ui_carousel.h"

#include "../lvgl_test_fixture.h"

#include "../catch_amalgamated.hpp"

// ============================================================================
// Task 2: Basic state and creation tests
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ui_carousel_get_state returns nullptr for nullptr",
                 "[carousel]") {
    REQUIRE(ui_carousel_get_state(nullptr) == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "ui_carousel_get_state returns nullptr for non-carousel object",
                 "[carousel]") {
    lv_obj_t* plain = lv_obj_create(test_screen());
    REQUIRE(plain != nullptr);
    REQUIRE(ui_carousel_get_state(plain) == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture,
                 "ui_carousel_get_state returns nullptr for object with wrong magic",
                 "[carousel]") {
    // Object with arbitrary user data should not be treated as carousel
    lv_obj_t* obj = lv_obj_create(test_screen());
    int dummy = 42;
    lv_obj_set_user_data(obj, &dummy);
    REQUIRE(ui_carousel_get_state(obj) == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "Carousel creation produces valid state with correct defaults",
                 "[carousel]") {
    // Manually create a carousel by allocating state (simulating what create callback does)
    lv_obj_t* container = lv_obj_create(test_screen());
    CarouselState* state = new CarouselState();
    state->scroll_container = lv_obj_create(container);
    state->indicator_row = lv_obj_create(container);
    lv_obj_set_user_data(container, state);

    CarouselState* retrieved = ui_carousel_get_state(container);
    REQUIRE(retrieved != nullptr);
    REQUIRE(retrieved->magic == CarouselState::MAGIC);
    REQUIRE(retrieved->scroll_container != nullptr);
    REQUIRE(retrieved->indicator_row != nullptr);
    REQUIRE(retrieved->real_tiles.empty());
    REQUIRE(retrieved->current_page == 0);
    REQUIRE(retrieved->wrap == true);
    REQUIRE(retrieved->show_indicators == true);
    REQUIRE(retrieved->auto_scroll_ms == 0);
    REQUIRE(retrieved->auto_timer == nullptr);
    REQUIRE(retrieved->page_subject == nullptr);

    // Clean up manually since we didn't use the XML create path
    delete state;
    lv_obj_set_user_data(container, nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "Empty carousel has page_count=0 and current_page=0",
                 "[carousel]") {
    lv_obj_t* container = lv_obj_create(test_screen());
    CarouselState* state = new CarouselState();
    state->scroll_container = lv_obj_create(container);
    lv_obj_set_user_data(container, state);

    REQUIRE(ui_carousel_get_page_count(container) == 0);
    REQUIRE(ui_carousel_get_current_page(container) == 0);

    delete state;
    lv_obj_set_user_data(container, nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "get_page_count and get_current_page return 0 for non-carousel",
                 "[carousel]") {
    lv_obj_t* plain = lv_obj_create(test_screen());
    REQUIRE(ui_carousel_get_page_count(plain) == 0);
    REQUIRE(ui_carousel_get_current_page(plain) == 0);
}

// ============================================================================
// Task 3: Tile management tests
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ui_carousel_add_item creates tiles and increments page count",
                 "[carousel]") {
    lv_obj_t* container = lv_obj_create(test_screen());
    CarouselState* state = new CarouselState();
    state->scroll_container = lv_obj_create(container);
    lv_obj_set_user_data(container, state);

    // Add first item
    lv_obj_t* item1 = lv_obj_create(test_screen());
    ui_carousel_add_item(container, item1);
    REQUIRE(ui_carousel_get_page_count(container) == 1);
    REQUIRE(state->real_tiles.size() == 1);

    // Verify item was reparented into the tile
    lv_obj_t* tile1 = state->real_tiles[0];
    REQUIRE(tile1 != nullptr);
    REQUIRE(lv_obj_get_parent(item1) == tile1);
    REQUIRE(lv_obj_get_parent(tile1) == state->scroll_container);

    delete state;
    lv_obj_set_user_data(container, nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "Adding multiple items tracks correct count", "[carousel]") {
    lv_obj_t* container = lv_obj_create(test_screen());
    CarouselState* state = new CarouselState();
    state->scroll_container = lv_obj_create(container);
    lv_obj_set_user_data(container, state);

    // Add three items
    lv_obj_t* items[3];
    for (int i = 0; i < 3; i++) {
        items[i] = lv_obj_create(test_screen());
        ui_carousel_add_item(container, items[i]);
    }

    REQUIRE(ui_carousel_get_page_count(container) == 3);
    REQUIRE(state->real_tiles.size() == 3);

    // Verify each item is in its own tile, and all tiles are in the scroll container
    for (int i = 0; i < 3; i++) {
        lv_obj_t* tile = state->real_tiles[i];
        REQUIRE(tile != nullptr);
        REQUIRE(lv_obj_get_parent(items[i]) == tile);
        REQUIRE(lv_obj_get_parent(tile) == state->scroll_container);
    }

    // Each tile should be distinct
    REQUIRE(state->real_tiles[0] != state->real_tiles[1]);
    REQUIRE(state->real_tiles[1] != state->real_tiles[2]);

    delete state;
    lv_obj_set_user_data(container, nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "ui_carousel_add_item ignores null carousel or item",
                 "[carousel]") {
    // Should not crash with nullptr carousel
    lv_obj_t* item = lv_obj_create(test_screen());
    ui_carousel_add_item(nullptr, item);

    // Should not crash with nullptr item
    lv_obj_t* container = lv_obj_create(test_screen());
    CarouselState* state = new CarouselState();
    state->scroll_container = lv_obj_create(container);
    lv_obj_set_user_data(container, state);

    ui_carousel_add_item(container, nullptr);
    REQUIRE(ui_carousel_get_page_count(container) == 0);

    delete state;
    lv_obj_set_user_data(container, nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "ui_carousel_add_item ignores non-carousel object",
                 "[carousel]") {
    lv_obj_t* plain = lv_obj_create(test_screen());
    lv_obj_t* item = lv_obj_create(test_screen());

    // Should not crash when called on a non-carousel object
    ui_carousel_add_item(plain, item);
    REQUIRE(ui_carousel_get_page_count(plain) == 0);
}
