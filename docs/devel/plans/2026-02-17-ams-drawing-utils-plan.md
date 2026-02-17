# AMS Drawing Utils — Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Extract duplicated AMS drawing code into shared utilities and align the mini widget's visuals with the overview cards.

**Architecture:** New `ams_drawing_utils.h/.cpp` in `namespace ams_draw` provides color utils, pulse animation, error badges, slot bar rendering, and presentation helpers. Each consumer file is refactored to call shared functions instead of local duplicates. The mini widget adopts the overview card's visual style (loaded = wider border, severity-aware errors).

**Tech Stack:** C++17, LVGL 9.4, Catch2 (tests), spdlog (logging)

**Design doc:** `docs/devel/plans/2026-02-17-ams-drawing-utils-design.md`

**Key docs to reference:**
- `docs/devel/UI_CONTRIBUTOR_GUIDE.md` — tokens, colors, theme system
- `docs/devel/LVGL9_XML_GUIDE.md` — XML widget patterns
- `docs/devel/FILAMENT_MANAGEMENT.md` — AMS architecture
- `include/ams_types.h` — SlotStatus, SlotError, SlotInfo, AmsUnit, AmsSystemInfo
- `include/theme_manager.h` — `theme_manager_get_color()`, `theme_manager_get_contrast_text()`

**Build/test commands:**
- `make -j` — build binary
- `make test` — build tests
- `make test-run` — build and run tests
- `./build/bin/helix-tests "[ams_draw]"` — run only these tests
- `./build/bin/helix-tests "[ams_slot]"` — run existing slot tests (regression)

**Important:** Makefile auto-discovers sources via wildcard — no Makefile edits needed for new `.cpp` files.

---

## Task 1: Add `SlotInfo::is_present()` to `ams_types.h`

Small, zero-risk change. Gets a canonical "is present" check onto the type so all later tasks can use it.

**Files:**
- Modify: `include/ams_types.h` (near `has_filament_info()` at line ~585)
- Test: `tests/unit/test_ams_drawing_utils.cpp` (new file)

**Step 1: Create test file with `is_present()` tests**

Create `tests/unit/test_ams_drawing_utils.cpp`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ams_types.h"

#include "../catch_amalgamated.hpp"

// ============================================================================
// SlotInfo::is_present()
// ============================================================================

TEST_CASE("SlotInfo::is_present returns false for EMPTY", "[ams_draw][slot_info]") {
    SlotInfo slot;
    slot.status = SlotStatus::EMPTY;
    REQUIRE_FALSE(slot.is_present());
}

TEST_CASE("SlotInfo::is_present returns false for UNKNOWN", "[ams_draw][slot_info]") {
    SlotInfo slot;
    slot.status = SlotStatus::UNKNOWN;
    REQUIRE_FALSE(slot.is_present());
}

TEST_CASE("SlotInfo::is_present returns true for AVAILABLE", "[ams_draw][slot_info]") {
    SlotInfo slot;
    slot.status = SlotStatus::AVAILABLE;
    REQUIRE(slot.is_present());
}

TEST_CASE("SlotInfo::is_present returns true for LOADED", "[ams_draw][slot_info]") {
    SlotInfo slot;
    slot.status = SlotStatus::LOADED;
    REQUIRE(slot.is_present());
}

TEST_CASE("SlotInfo::is_present returns true for FROM_BUFFER", "[ams_draw][slot_info]") {
    SlotInfo slot;
    slot.status = SlotStatus::FROM_BUFFER;
    REQUIRE(slot.is_present());
}

TEST_CASE("SlotInfo::is_present returns true for BLOCKED", "[ams_draw][slot_info]") {
    SlotInfo slot;
    slot.status = SlotStatus::BLOCKED;
    REQUIRE(slot.is_present());
}
```

**Step 2: Run tests — expect compile failure** (`is_present` not defined)

Run: `make test && ./build/bin/helix-tests "[slot_info]" -v`

**Step 3: Add `is_present()` to `SlotInfo`**

In `include/ams_types.h`, after the `is_multi_color()` method (~line 594), add:

```cpp
    /**
     * @brief Check if filament is present in this slot
     * @return true for AVAILABLE, LOADED, FROM_BUFFER, BLOCKED; false for EMPTY, UNKNOWN
     */
    [[nodiscard]] bool is_present() const {
        return status != SlotStatus::EMPTY && status != SlotStatus::UNKNOWN;
    }
```

**Step 4: Run tests — expect pass**

Run: `make test && ./build/bin/helix-tests "[slot_info]" -v`

**Step 5: Commit**

```bash
git add include/ams_types.h tests/unit/test_ams_drawing_utils.cpp
git commit -m "feat(ams): add SlotInfo::is_present() canonical presence check"
```

---

## Task 2: Create `ams_drawing_utils.h/.cpp` — Color Utils

Foundation layer. Pure functions, no LVGL widget dependencies.

**Files:**
- Create: `src/ui/ams_drawing_utils.h`
- Create: `src/ui/ams_drawing_utils.cpp`
- Modify: `tests/unit/test_ams_drawing_utils.cpp` (append tests)

**Step 1: Write color util tests**

Append to `tests/unit/test_ams_drawing_utils.cpp`:

```cpp
#include "ams_drawing_utils.h"

// ============================================================================
// Color Utilities
// ============================================================================

TEST_CASE("ams_draw::lighten_color adds amount clamped to 255", "[ams_draw][color]") {
    lv_color_t c = lv_color_make(100, 200, 250);
    lv_color_t result = ams_draw::lighten_color(c, 50);
    REQUIRE(result.red == 150);
    REQUIRE(result.green == 250);
    REQUIRE(result.blue == 255); // Clamped
}

TEST_CASE("ams_draw::darken_color subtracts amount clamped to 0", "[ams_draw][color]") {
    lv_color_t c = lv_color_make(30, 100, 200);
    lv_color_t result = ams_draw::darken_color(c, 50);
    REQUIRE(result.red == 0); // Clamped
    REQUIRE(result.green == 50);
    REQUIRE(result.blue == 150);
}

TEST_CASE("ams_draw::blend_color interpolates between colors", "[ams_draw][color]") {
    lv_color_t black = lv_color_make(0, 0, 0);
    lv_color_t white = lv_color_make(255, 255, 255);

    // factor=0 → first color
    lv_color_t at_zero = ams_draw::blend_color(black, white, 0.0f);
    REQUIRE(at_zero.red == 0);

    // factor=1 → second color
    lv_color_t at_one = ams_draw::blend_color(black, white, 1.0f);
    REQUIRE(at_one.red == 255);

    // factor=0.5 → midpoint
    lv_color_t mid = ams_draw::blend_color(black, white, 0.5f);
    REQUIRE(mid.red >= 126);
    REQUIRE(mid.red <= 128);
}

TEST_CASE("ams_draw::blend_color clamps factor to [0,1]", "[ams_draw][color]") {
    lv_color_t a = lv_color_make(100, 100, 100);
    lv_color_t b = lv_color_make(200, 200, 200);

    lv_color_t below = ams_draw::blend_color(a, b, -1.0f);
    REQUIRE(below.red == 100);

    lv_color_t above = ams_draw::blend_color(a, b, 2.0f);
    REQUIRE(above.red == 200);
}
```

**Step 2: Run tests — expect compile failure**

Run: `make test`

**Step 3: Create header and implementation**

Create `src/ui/ams_drawing_utils.h`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ams_types.h"

#include "lvgl/lvgl.h"

#include <string>

/**
 * @brief Shared AMS drawing utilities
 *
 * Consolidates duplicated drawing code used by ui_ams_mini_status,
 * ui_panel_ams_overview, ui_ams_slot, and ui_spool_canvas.
 *
 * Layout ownership stays with each widget. These are the shared
 * rendering primitives and presentation helpers.
 */
namespace ams_draw {

// ============================================================================
// Color Utilities
// ============================================================================

/** Lighten a color by adding amount to each channel (clamped to 255) */
lv_color_t lighten_color(lv_color_t c, uint8_t amount);

/** Darken a color by subtracting amount from each channel (clamped to 0) */
lv_color_t darken_color(lv_color_t c, uint8_t amount);

/** Blend two colors: factor=0 → c1, factor=1 → c2 (clamped to [0,1]) */
lv_color_t blend_color(lv_color_t c1, lv_color_t c2, float factor);

} // namespace ams_draw
```

Create `src/ui/ams_drawing_utils.cpp`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ams_drawing_utils.h"

#include <algorithm>

namespace ams_draw {

// ============================================================================
// Color Utilities
// ============================================================================

lv_color_t lighten_color(lv_color_t c, uint8_t amount) {
    return lv_color_make(std::min(255, c.red + amount),
                         std::min(255, c.green + amount),
                         std::min(255, c.blue + amount));
}

lv_color_t darken_color(lv_color_t c, uint8_t amount) {
    return lv_color_make(c.red > amount ? c.red - amount : 0,
                         c.green > amount ? c.green - amount : 0,
                         c.blue > amount ? c.blue - amount : 0);
}

lv_color_t blend_color(lv_color_t c1, lv_color_t c2, float factor) {
    factor = std::clamp(factor, 0.0f, 1.0f);
    return lv_color_make(
        static_cast<uint8_t>(c1.red + (c2.red - c1.red) * factor),
        static_cast<uint8_t>(c1.green + (c2.green - c1.green) * factor),
        static_cast<uint8_t>(c1.blue + (c2.blue - c1.blue) * factor));
}

} // namespace ams_draw
```

**Step 4: Run tests — expect pass**

Run: `make test && ./build/bin/helix-tests "[ams_draw]" -v`

**Step 5: Commit**

```bash
git add src/ui/ams_drawing_utils.h src/ui/ams_drawing_utils.cpp tests/unit/test_ams_drawing_utils.cpp
git commit -m "feat(ams): add ams_drawing_utils with color utilities"
```

---

## Task 3: Add Severity Color, Worst Unit Severity, Fill Percent, Bar Width, Display Name

Pure functions that don't need LVGL widgets for testing (severity_color needs theme init, so use LVGLTestFixture for those).

**Files:**
- Modify: `src/ui/ams_drawing_utils.h` (add declarations)
- Modify: `src/ui/ams_drawing_utils.cpp` (add implementations)
- Modify: `tests/unit/test_ams_drawing_utils.cpp` (append tests)

**Step 1: Write tests**

Append to `tests/unit/test_ams_drawing_utils.cpp`:

```cpp
#include "../lvgl_test_fixture.h"
#include "theme_manager.h"

// ============================================================================
// severity_color (needs theme init for theme_manager_get_color)
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::severity_color maps ERROR to danger",
                 "[ams_draw][severity]") {
    lv_color_t result = ams_draw::severity_color(SlotError::ERROR);
    lv_color_t expected = theme_manager_get_color("danger");
    REQUIRE(result.red == expected.red);
    REQUIRE(result.green == expected.green);
    REQUIRE(result.blue == expected.blue);
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::severity_color maps WARNING to warning",
                 "[ams_draw][severity]") {
    lv_color_t result = ams_draw::severity_color(SlotError::WARNING);
    lv_color_t expected = theme_manager_get_color("warning");
    REQUIRE(result.red == expected.red);
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::severity_color maps INFO to text_muted",
                 "[ams_draw][severity]") {
    lv_color_t result = ams_draw::severity_color(SlotError::INFO);
    lv_color_t expected = theme_manager_get_color("text_muted");
    REQUIRE(result.red == expected.red);
}

// ============================================================================
// worst_unit_severity
// ============================================================================

TEST_CASE("ams_draw::worst_unit_severity returns INFO for no errors", "[ams_draw][severity]") {
    AmsUnit unit;
    unit.slots.resize(4); // 4 slots, no errors
    REQUIRE(ams_draw::worst_unit_severity(unit) == SlotError::INFO);
}

TEST_CASE("ams_draw::worst_unit_severity finds ERROR among warnings", "[ams_draw][severity]") {
    AmsUnit unit;
    unit.slots.resize(4);
    unit.slots[1].error = SlotError{SlotError::WARNING, "warn"};
    unit.slots[3].error = SlotError{SlotError::ERROR, "err"};
    REQUIRE(ams_draw::worst_unit_severity(unit) == SlotError::ERROR);
}

// ============================================================================
// fill_percent_from_slot
// ============================================================================

TEST_CASE("ams_draw::fill_percent_from_slot with known weight", "[ams_draw][fill]") {
    SlotInfo slot;
    slot.remaining_weight_g = 500.0f;
    slot.total_weight_g = 1000.0f;
    REQUIRE(ams_draw::fill_percent_from_slot(slot) == 50);
}

TEST_CASE("ams_draw::fill_percent_from_slot clamps to min_pct", "[ams_draw][fill]") {
    SlotInfo slot;
    slot.remaining_weight_g = 1.0f;
    slot.total_weight_g = 1000.0f;
    // 0.1% → clamped to min_pct (default 5)
    REQUIRE(ams_draw::fill_percent_from_slot(slot) == 5);
}

TEST_CASE("ams_draw::fill_percent_from_slot returns 100 for unknown weight", "[ams_draw][fill]") {
    SlotInfo slot;
    slot.remaining_weight_g = -1.0f; // Unknown
    slot.total_weight_g = 0.0f;
    REQUIRE(ams_draw::fill_percent_from_slot(slot) == 100);
}

TEST_CASE("ams_draw::fill_percent_from_slot custom min_pct", "[ams_draw][fill]") {
    SlotInfo slot;
    slot.remaining_weight_g = 0.0f;
    slot.total_weight_g = 1000.0f;
    REQUIRE(ams_draw::fill_percent_from_slot(slot, 0) == 0);
    REQUIRE(ams_draw::fill_percent_from_slot(slot, 10) == 10);
}

// ============================================================================
// calc_bar_width
// ============================================================================

TEST_CASE("ams_draw::calc_bar_width distributes evenly", "[ams_draw][bar_width]") {
    // 100px container, 4 slots, 2px gap => 3 gaps = 6px => 94px / 4 = 23px
    // Clamped to max=14
    int32_t w = ams_draw::calc_bar_width(100, 4, 2, 6, 14);
    REQUIRE(w == 14);
}

TEST_CASE("ams_draw::calc_bar_width respects min", "[ams_draw][bar_width]") {
    // Tiny container, many slots
    int32_t w = ams_draw::calc_bar_width(20, 16, 2, 6, 14);
    REQUIRE(w == 6);
}

TEST_CASE("ams_draw::calc_bar_width with container_pct", "[ams_draw][bar_width]") {
    // 100px container at 90% => 90px usable, 1 slot, no gaps => 90px, clamped to max=14
    int32_t w = ams_draw::calc_bar_width(100, 1, 2, 6, 14, 90);
    REQUIRE(w == 14);
}

TEST_CASE("ams_draw::calc_bar_width handles zero slots", "[ams_draw][bar_width]") {
    int32_t w = ams_draw::calc_bar_width(100, 0, 2, 6, 14);
    REQUIRE(w == 14); // Falls through to max (100/1 clamped)
}

// ============================================================================
// get_unit_display_name
// ============================================================================

TEST_CASE("ams_draw::get_unit_display_name uses name when set", "[ams_draw][display_name]") {
    AmsUnit unit;
    unit.name = "Box Turtle 1";
    REQUIRE(ams_draw::get_unit_display_name(unit, 0) == "Box Turtle 1");
}

TEST_CASE("ams_draw::get_unit_display_name falls back to Unit N", "[ams_draw][display_name]") {
    AmsUnit unit; // name is empty
    REQUIRE(ams_draw::get_unit_display_name(unit, 0) == "Unit 1");
    REQUIRE(ams_draw::get_unit_display_name(unit, 2) == "Unit 3");
}
```

**Step 2: Run tests — expect compile failure**

Run: `make test`

**Step 3: Add declarations to header**

Append to `ams_drawing_utils.h` inside the namespace, after color utils:

```cpp
// ============================================================================
// Severity & Error Helpers
// ============================================================================

/** Map error severity to theme color (danger/warning/text_muted) */
lv_color_t severity_color(SlotError::Severity severity);

/** Get worst error severity across all slots in a unit */
SlotError::Severity worst_unit_severity(const AmsUnit& unit);

// ============================================================================
// Data Helpers
// ============================================================================

/** Calculate fill percentage from SlotInfo weight data (returns min_pct..100, or 100 if unknown) */
int fill_percent_from_slot(const SlotInfo& slot, int min_pct = 5);

/**
 * Calculate bar width to fit slot_count bars in container_width.
 * @param container_pct Percentage of container_width to use (default 100)
 */
int32_t calc_bar_width(int32_t container_width, int slot_count,
                       int32_t gap, int32_t min_width, int32_t max_width,
                       int container_pct = 100);

// ============================================================================
// Presentation Helpers
// ============================================================================

/** Get display name for a unit (uses unit.name, falls back to "Unit N") */
std::string get_unit_display_name(const AmsUnit& unit, int unit_index);
```

**Step 4: Add implementations**

Append to `ams_drawing_utils.cpp`:

```cpp
#include "theme_manager.h"

#include <spdlog/spdlog.h>

// ============================================================================
// Severity & Error Helpers
// ============================================================================

lv_color_t severity_color(SlotError::Severity severity) {
    switch (severity) {
    case SlotError::ERROR:
        return theme_manager_get_color("danger");
    case SlotError::WARNING:
        return theme_manager_get_color("warning");
    default:
        return theme_manager_get_color("text_muted");
    }
}

SlotError::Severity worst_unit_severity(const AmsUnit& unit) {
    SlotError::Severity worst = SlotError::INFO;
    for (const auto& slot : unit.slots) {
        if (slot.error.has_value() && slot.error->severity > worst) {
            worst = slot.error->severity;
        }
    }
    return worst;
}

// ============================================================================
// Data Helpers
// ============================================================================

int fill_percent_from_slot(const SlotInfo& slot, int min_pct) {
    float pct = slot.get_remaining_percent();
    if (pct < 0) {
        return 100; // Unknown weight → show full
    }
    return std::clamp(static_cast<int>(pct), min_pct, 100);
}

int32_t calc_bar_width(int32_t container_width, int slot_count,
                       int32_t gap, int32_t min_width, int32_t max_width,
                       int container_pct) {
    int32_t usable = (container_width * container_pct) / 100;
    int count = std::max(1, slot_count);
    int32_t total_gaps = (count > 1) ? (count - 1) * gap : 0;
    int32_t width = (usable - total_gaps) / count;
    return std::clamp(width, min_width, max_width);
}

// ============================================================================
// Presentation Helpers
// ============================================================================

std::string get_unit_display_name(const AmsUnit& unit, int unit_index) {
    if (!unit.name.empty()) {
        return unit.name;
    }
    return "Unit " + std::to_string(unit_index + 1);
}
```

**Step 5: Run tests — expect pass**

Run: `make test && ./build/bin/helix-tests "[ams_draw]" -v`

**Step 6: Commit**

```bash
git add src/ui/ams_drawing_utils.h src/ui/ams_drawing_utils.cpp tests/unit/test_ams_drawing_utils.cpp
git commit -m "feat(ams): add severity, fill, bar-width, display-name utils"
```

---

## Task 4: Add Pulse Animation, Error Badge, Transparent Container, Slot Bar Column

These need LVGL objects so tests use `LVGLTestFixture`. The pulse animation and error badge are closely coupled (badge uses pulse), so implement together.

**Files:**
- Modify: `src/ui/ams_drawing_utils.h`
- Modify: `src/ui/ams_drawing_utils.cpp`
- Modify: `tests/unit/test_ams_drawing_utils.cpp`

**Step 1: Write tests**

Append to `tests/unit/test_ams_drawing_utils.cpp`:

```cpp
// ============================================================================
// Transparent Container
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::create_transparent_container basic properties",
                 "[ams_draw][container]") {
    lv_obj_t* c = ams_draw::create_transparent_container(test_screen());
    REQUIRE(c != nullptr);
    REQUIRE(lv_obj_get_style_bg_opa(c, LV_PART_MAIN) == LV_OPA_TRANSP);
    REQUIRE(lv_obj_get_style_border_width(c, LV_PART_MAIN) == 0);
    REQUIRE(lv_obj_get_style_pad_top(c, LV_PART_MAIN) == 0);
    REQUIRE(lv_obj_has_flag(c, LV_OBJ_FLAG_EVENT_BUBBLE));
    REQUIRE_FALSE(lv_obj_has_flag(c, LV_OBJ_FLAG_SCROLLABLE));
}

// ============================================================================
// Error Badge
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::create_error_badge creates circle",
                 "[ams_draw][badge]") {
    lv_obj_t* badge = ams_draw::create_error_badge(test_screen(), 12);
    REQUIRE(badge != nullptr);
    REQUIRE(lv_obj_get_width(badge) == 12);
    REQUIRE(lv_obj_get_height(badge) == 12);
    REQUIRE(lv_obj_get_style_radius(badge, LV_PART_MAIN) == LV_RADIUS_CIRCLE);
    REQUIRE(lv_obj_has_flag(badge, LV_OBJ_FLAG_HIDDEN)); // Hidden by default
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::update_error_badge shows on error",
                 "[ams_draw][badge]") {
    lv_obj_t* badge = ams_draw::create_error_badge(test_screen(), 12);

    ams_draw::update_error_badge(badge, true, SlotError::ERROR, false);
    REQUIRE_FALSE(lv_obj_has_flag(badge, LV_OBJ_FLAG_HIDDEN));

    ams_draw::update_error_badge(badge, false, SlotError::INFO, false);
    REQUIRE(lv_obj_has_flag(badge, LV_OBJ_FLAG_HIDDEN));
}

// ============================================================================
// Pulse Animation
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::start_pulse and stop_pulse don't crash",
                 "[ams_draw][pulse]") {
    lv_obj_t* dot = ams_draw::create_error_badge(test_screen(), 14);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_HIDDEN);
    process_lvgl(10);

    // Start pulse — should set border_color for anim callback reference
    ams_draw::start_pulse(dot, lv_color_hex(0xFF0000));
    process_lvgl(50);

    // Verify scale transform was applied (animation started)
    // Just ensure no crash; exact values depend on anim timing
    REQUIRE(lv_obj_get_style_transform_scale(dot, LV_PART_MAIN) > 0);

    // Stop pulse — should restore defaults
    ams_draw::stop_pulse(dot);
    process_lvgl(10);
    REQUIRE(lv_obj_get_style_transform_scale(dot, LV_PART_MAIN) == 256);
    REQUIRE(lv_obj_get_style_shadow_width(dot, LV_PART_MAIN) == 0);
}

// ============================================================================
// Slot Bar Column
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::create_slot_column creates all parts",
                 "[ams_draw][slot_bar]") {
    auto col = ams_draw::create_slot_column(test_screen(), 10, 40, 4);
    REQUIRE(col.container != nullptr);
    REQUIRE(col.bar_bg != nullptr);
    REQUIRE(col.bar_fill != nullptr);
    REQUIRE(col.status_line != nullptr);

    // bar_fill is child of bar_bg
    REQUIRE(lv_obj_get_parent(col.bar_fill) == col.bar_bg);
    // bar_bg and status_line are children of container
    REQUIRE(lv_obj_get_parent(col.bar_bg) == col.container);
    REQUIRE(lv_obj_get_parent(col.status_line) == col.container);
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::style_slot_bar loaded state",
                 "[ams_draw][slot_bar]") {
    auto col = ams_draw::create_slot_column(test_screen(), 10, 40, 4);

    ams_draw::BarStyleParams params;
    params.color_rgb = 0xFF0000;
    params.fill_pct = 75;
    params.is_present = true;
    params.is_loaded = true;
    params.has_error = false;
    ams_draw::style_slot_bar(col, params, 4);

    // Loaded: 2px border, text color, 80% opacity
    REQUIRE(lv_obj_get_style_border_width(col.bar_bg, LV_PART_MAIN) == 2);
    REQUIRE(lv_obj_get_style_border_opa(col.bar_bg, LV_PART_MAIN) == LV_OPA_80);

    // Fill visible
    REQUIRE_FALSE(lv_obj_has_flag(col.bar_fill, LV_OBJ_FLAG_HIDDEN));

    // Status line hidden (loaded shown via border, not status line)
    REQUIRE(lv_obj_has_flag(col.status_line, LV_OBJ_FLAG_HIDDEN));
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::style_slot_bar error state shows status line",
                 "[ams_draw][slot_bar]") {
    auto col = ams_draw::create_slot_column(test_screen(), 10, 40, 4);

    ams_draw::BarStyleParams params;
    params.color_rgb = 0x00FF00;
    params.fill_pct = 50;
    params.is_present = true;
    params.is_loaded = false;
    params.has_error = true;
    params.severity = SlotError::ERROR;
    ams_draw::style_slot_bar(col, params, 4);

    // Error: status line visible
    REQUIRE_FALSE(lv_obj_has_flag(col.status_line, LV_OBJ_FLAG_HIDDEN));
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::style_slot_bar empty state ghosted",
                 "[ams_draw][slot_bar]") {
    auto col = ams_draw::create_slot_column(test_screen(), 10, 40, 4);

    ams_draw::BarStyleParams params;
    params.is_present = false;
    ams_draw::style_slot_bar(col, params, 4);

    // Empty: 20% border opacity, fill hidden, status line hidden
    REQUIRE(lv_obj_get_style_border_opa(col.bar_bg, LV_PART_MAIN) == LV_OPA_20);
    REQUIRE(lv_obj_has_flag(col.bar_fill, LV_OBJ_FLAG_HIDDEN));
    REQUIRE(lv_obj_has_flag(col.status_line, LV_OBJ_FLAG_HIDDEN));
}
```

**Step 2: Run tests — expect compile failure**

Run: `make test`

**Step 3: Add declarations to header**

Append to `ams_drawing_utils.h` inside the namespace:

```cpp
// ============================================================================
// LVGL Widget Factories
// ============================================================================

/** Create a transparent container (no bg, no border, no padding, no scroll, event bubble) */
lv_obj_t* create_transparent_container(lv_obj_t* parent);

// ============================================================================
// Pulse Animation
// ============================================================================

/// Pulse animation constants (shared by error badges and error dots)
constexpr int32_t PULSE_SCALE_MIN = 180;  ///< ~70% scale
constexpr int32_t PULSE_SCALE_MAX = 256;  ///< 100% scale
constexpr int32_t PULSE_SAT_MIN = 80;     ///< Washed out
constexpr int32_t PULSE_SAT_MAX = 255;    ///< Full vivid
constexpr uint32_t PULSE_DURATION_MS = 800;

/** Start scale+saturation pulse animation on an object. Stores base_color in border_color. */
void start_pulse(lv_obj_t* dot, lv_color_t base_color);

/** Stop pulse animation and restore defaults (scale=256, no shadow) */
void stop_pulse(lv_obj_t* dot);

// ============================================================================
// Error Badge
// ============================================================================

/** Create a circular error badge (hidden by default, caller positions it) */
lv_obj_t* create_error_badge(lv_obj_t* parent, int32_t size);

/** Update badge visibility, color, and pulse based on error state */
void update_error_badge(lv_obj_t* badge, bool has_error,
                        SlotError::Severity severity, bool animate);

// ============================================================================
// Slot Bar Column (mini bar with fill + status line)
// ============================================================================

/** Return type for create_slot_column */
struct SlotColumn {
    lv_obj_t* container = nullptr;    ///< Column flex wrapper (bar + status line)
    lv_obj_t* bar_bg = nullptr;       ///< Background/outline container
    lv_obj_t* bar_fill = nullptr;     ///< Colored fill (child of bar_bg)
    lv_obj_t* status_line = nullptr;  ///< Bottom indicator line
};

/** Parameters for styling a slot bar */
struct BarStyleParams {
    uint32_t color_rgb = 0x808080;
    int fill_pct = 100;
    bool is_present = false;
    bool is_loaded = false;
    bool has_error = false;
    SlotError::Severity severity = SlotError::INFO;
};

/// Status line dimensions
constexpr int32_t STATUS_LINE_HEIGHT_PX = 3;
constexpr int32_t STATUS_LINE_GAP_PX = 2;

/** Create slot column: bar_bg (with bar_fill child) + status_line in a column flex container */
SlotColumn create_slot_column(lv_obj_t* parent, int32_t bar_width,
                              int32_t bar_height, int32_t bar_radius);

/**
 * Style an existing slot bar (update colors, borders, fill, status line).
 * Visual style matches the overview cards:
 * - Loaded: 2px border, text color, 80% opa
 * - Present: 1px border, text_muted, 50% opa
 * - Empty: 1px border, text_muted, 20% opa (ghosted)
 * - Error: status line with severity color
 * - Non-error: status line hidden
 */
void style_slot_bar(const SlotColumn& col, const BarStyleParams& params,
                    int32_t bar_radius);
```

**Step 4: Add implementations**

Append to `ams_drawing_utils.cpp`:

```cpp
#include "settings_manager.h"

// ============================================================================
// LVGL Widget Factories
// ============================================================================

lv_obj_t* create_transparent_container(lv_obj_t* parent) {
    lv_obj_t* c = lv_obj_create(parent);
    lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(c, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(c, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN);
    return c;
}

// ============================================================================
// Pulse Animation
// ============================================================================

static void pulse_scale_anim_cb(void* var, int32_t value) {
    auto* obj = static_cast<lv_obj_t*>(var);
    lv_obj_set_style_transform_scale(obj, value, LV_PART_MAIN);
    int32_t range = PULSE_SCALE_MAX - PULSE_SCALE_MIN;
    int32_t progress = value - PULSE_SCALE_MIN;
    int32_t shadow = progress * 8 / range;
    lv_obj_set_style_shadow_width(obj, shadow, LV_PART_MAIN);
    lv_opa_t shadow_opa = static_cast<lv_opa_t>(progress * 180 / range);
    lv_obj_set_style_shadow_opa(obj, shadow_opa, LV_PART_MAIN);
}

static void pulse_color_anim_cb(void* var, int32_t value) {
    auto* obj = static_cast<lv_obj_t*>(var);
    lv_color_t base = lv_obj_get_style_border_color(obj, LV_PART_MAIN);
    uint8_t gray = static_cast<uint8_t>((base.red * 77 + base.green * 150 + base.blue * 29) >> 8);
    lv_color_t gray_color = lv_color_make(gray, gray, gray);
    lv_color_t result = lv_color_mix(base, gray_color, static_cast<lv_opa_t>(value));
    lv_obj_set_style_bg_color(obj, result, LV_PART_MAIN);
}

void start_pulse(lv_obj_t* dot, lv_color_t base_color) {
    lv_obj_set_style_border_color(dot, base_color, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(dot, base_color, LV_PART_MAIN);

    int32_t w = lv_obj_get_width(dot);
    int32_t h = lv_obj_get_height(dot);
    lv_obj_set_style_transform_pivot_x(dot, w / 2, LV_PART_MAIN);
    lv_obj_set_style_transform_pivot_y(dot, h / 2, LV_PART_MAIN);

    lv_anim_t sa;
    lv_anim_init(&sa);
    lv_anim_set_var(&sa, dot);
    lv_anim_set_values(&sa, PULSE_SCALE_MAX, PULSE_SCALE_MIN);
    lv_anim_set_time(&sa, PULSE_DURATION_MS);
    lv_anim_set_playback_time(&sa, PULSE_DURATION_MS);
    lv_anim_set_repeat_count(&sa, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&sa, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&sa, pulse_scale_anim_cb);
    lv_anim_start(&sa);

    lv_anim_t ca;
    lv_anim_init(&ca);
    lv_anim_set_var(&ca, dot);
    lv_anim_set_values(&ca, PULSE_SAT_MAX, PULSE_SAT_MIN);
    lv_anim_set_time(&ca, PULSE_DURATION_MS);
    lv_anim_set_playback_time(&ca, PULSE_DURATION_MS);
    lv_anim_set_repeat_count(&ca, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&ca, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&ca, pulse_color_anim_cb);
    lv_anim_start(&ca);
}

void stop_pulse(lv_obj_t* dot) {
    lv_anim_delete(dot, pulse_scale_anim_cb);
    lv_anim_delete(dot, pulse_color_anim_cb);
    lv_obj_set_style_transform_scale(dot, 256, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(dot, LV_OPA_TRANSP, LV_PART_MAIN);
}

// ============================================================================
// Error Badge
// ============================================================================

lv_obj_t* create_error_badge(lv_obj_t* parent, int32_t size) {
    lv_obj_t* badge = lv_obj_create(parent);
    lv_obj_set_size(badge, size, size);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(badge, 0, LV_PART_MAIN);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(badge, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(badge, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
    return badge;
}

void update_error_badge(lv_obj_t* badge, bool has_error,
                        SlotError::Severity severity, bool animate) {
    if (!badge) {
        return;
    }

    if (has_error) {
        lv_color_t color = severity_color(severity);
        lv_obj_set_style_bg_color(badge, color, LV_PART_MAIN);
        lv_obj_remove_flag(badge, LV_OBJ_FLAG_HIDDEN);
        if (animate) {
            start_pulse(badge, color);
        } else {
            stop_pulse(badge);
        }
    } else {
        stop_pulse(badge);
        lv_obj_add_flag(badge, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============================================================================
// Slot Bar Column
// ============================================================================

SlotColumn create_slot_column(lv_obj_t* parent, int32_t bar_width,
                              int32_t bar_height, int32_t bar_radius) {
    SlotColumn col;

    // Column container (bar + status line)
    col.container = create_transparent_container(parent);
    lv_obj_set_size(col.container, bar_width,
                    bar_height + STATUS_LINE_HEIGHT_PX + STATUS_LINE_GAP_PX);
    lv_obj_set_flex_flow(col.container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col.container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col.container, STATUS_LINE_GAP_PX, LV_PART_MAIN);

    // Bar background (outline container)
    col.bar_bg = create_transparent_container(col.container);
    lv_obj_set_size(col.bar_bg, bar_width, bar_height);
    lv_obj_set_style_radius(col.bar_bg, bar_radius, LV_PART_MAIN);

    // Fill inside bar_bg (anchored to bottom, grows upward)
    col.bar_fill = create_transparent_container(col.bar_bg);
    lv_obj_set_width(col.bar_fill, LV_PCT(100));
    lv_obj_set_style_radius(col.bar_fill, bar_radius, LV_PART_MAIN);

    // Status line below bar
    col.status_line = create_transparent_container(col.container);
    lv_obj_set_size(col.status_line, bar_width, STATUS_LINE_HEIGHT_PX);
    lv_obj_set_style_radius(col.status_line, bar_radius / 2, LV_PART_MAIN);

    return col;
}

void style_slot_bar(const SlotColumn& col, const BarStyleParams& params,
                    int32_t bar_radius) {
    if (!col.bar_bg || !col.bar_fill) {
        return;
    }

    // --- Bar background border ---
    if (params.is_loaded && !params.has_error) {
        // Loaded: wider, brighter border
        lv_obj_set_style_border_width(col.bar_bg, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(col.bar_bg, theme_manager_get_color("text"), LV_PART_MAIN);
        lv_obj_set_style_border_opa(col.bar_bg, LV_OPA_80, LV_PART_MAIN);
    } else {
        lv_obj_set_style_border_width(col.bar_bg, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(col.bar_bg, theme_manager_get_color("text_muted"),
                                      LV_PART_MAIN);
        lv_obj_set_style_border_opa(col.bar_bg, params.is_present ? LV_OPA_50 : LV_OPA_20,
                                    LV_PART_MAIN);
    }

    // --- Fill gradient ---
    if (params.is_present && params.fill_pct > 0) {
        lv_color_t base_color = lv_color_hex(params.color_rgb);
        lv_color_t light_color = lighten_color(base_color, 50);

        lv_obj_set_style_bg_color(col.bar_fill, light_color, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(col.bar_fill, base_color, LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(col.bar_fill, LV_GRAD_DIR_VER, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(col.bar_fill, LV_OPA_COVER, LV_PART_MAIN);

        lv_obj_set_height(col.bar_fill, LV_PCT(params.fill_pct));
        lv_obj_align(col.bar_fill, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_remove_flag(col.bar_fill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(col.bar_fill, LV_OBJ_FLAG_HIDDEN);
    }

    // --- Status line ---
    if (col.status_line) {
        if (params.has_error) {
            lv_color_t error_color = severity_color(params.severity);
            lv_obj_set_style_bg_color(col.status_line, error_color, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(col.status_line, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_remove_flag(col.status_line, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(col.status_line, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
```

**Step 5: Run tests — expect pass**

Run: `make test && ./build/bin/helix-tests "[ams_draw]" -v`

**Step 6: Commit**

```bash
git add src/ui/ams_drawing_utils.h src/ui/ams_drawing_utils.cpp tests/unit/test_ams_drawing_utils.cpp
git commit -m "feat(ams): add pulse, error badge, slot bar, container utils"
```

---

## Task 5: Add Logo Helper

Needs `AmsState::get_logo_path()` which is a static method — no backend needed for testing.

**Files:**
- Modify: `src/ui/ams_drawing_utils.h`
- Modify: `src/ui/ams_drawing_utils.cpp`
- Modify: `tests/unit/test_ams_drawing_utils.cpp`

**Step 1: Write tests**

Append to `tests/unit/test_ams_drawing_utils.cpp`:

```cpp
#include "ams_state.h"

// ============================================================================
// Logo Helper
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::apply_logo hides image when no logo found",
                 "[ams_draw][logo]") {
    lv_obj_t* img = lv_image_create(test_screen());
    lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);

    // Use a type name that won't match any logo
    ams_draw::apply_logo(img, "NonExistentType12345");
    REQUIRE(lv_obj_has_flag(img, LV_OBJ_FLAG_HIDDEN));
}

TEST_CASE_METHOD(LVGLTestFixture, "ams_draw::apply_logo with unit fallback",
                 "[ams_draw][logo]") {
    lv_obj_t* img = lv_image_create(test_screen());

    AmsUnit unit;
    unit.name = "NonExistent";
    AmsSystemInfo info;
    info.type_name = "AlsoNonExistent";

    ams_draw::apply_logo(img, unit, info);
    // Both names unknown → hidden
    REQUIRE(lv_obj_has_flag(img, LV_OBJ_FLAG_HIDDEN));
}
```

**Step 2: Run — expect compile failure**

**Step 3: Add declarations and implementation**

In header:
```cpp
// ============================================================================
// Logo Helpers
// ============================================================================

/** Apply logo to image widget: try unit name → type name → hide */
void apply_logo(lv_obj_t* image, const AmsUnit& unit, const AmsSystemInfo& info);

/** Apply logo to image widget: try type name → hide */
void apply_logo(lv_obj_t* image, const std::string& type_name);
```

In implementation:
```cpp
#include "ams_state.h"

// ============================================================================
// Logo Helpers
// ============================================================================

void apply_logo(lv_obj_t* image, const AmsUnit& unit, const AmsSystemInfo& info) {
    if (!image) {
        return;
    }

    const char* path = AmsState::get_logo_path(unit.name);
    if (!path || !path[0]) {
        path = AmsState::get_logo_path(info.type_name);
    }

    if (path && path[0]) {
        lv_image_set_src(image, path);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
    }
}

void apply_logo(lv_obj_t* image, const std::string& type_name) {
    if (!image) {
        return;
    }

    const char* path = AmsState::get_logo_path(type_name);
    if (path && path[0]) {
        lv_image_set_src(image, path);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
    }
}
```

**Step 4: Run tests — expect pass**

Run: `make test && ./build/bin/helix-tests "[ams_draw]" -v`

**Step 5: Commit**

```bash
git add src/ui/ams_drawing_utils.h src/ui/ams_drawing_utils.cpp tests/unit/test_ams_drawing_utils.cpp
git commit -m "feat(ams): add logo fallback helper to drawing utils"
```

---

## Task 6: Refactor `ui_spool_canvas.cpp` — Use Shared Color Utils

Smallest consumer refactor. Replace local `lighten_color`, `darken_color`, `blend_color` with `ams_draw::` versions.

**Files:**
- Modify: `src/ui/ui_spool_canvas.cpp`

**Step 1: Build + run existing tests to establish baseline**

Run: `make test-run`

All tests should pass. Note the count.

**Step 2: Replace color functions**

In `ui_spool_canvas.cpp`:
1. Add `#include "ams_drawing_utils.h"` near the top includes
2. Delete the local `darken_color()` (lines 53-56), `lighten_color()` (lines 58-62), and `blend_color()` (lines 65-70) functions
3. Replace all call sites with `ams_draw::` prefix:
   - `darken_color(` → `ams_draw::darken_color(`
   - `lighten_color(` → `ams_draw::lighten_color(`
   - `blend_color(` → `ams_draw::blend_color(`

**Step 3: Build and run all tests**

Run: `make test-run`

All tests should still pass with same count.

**Step 4: Commit**

```bash
git add src/ui/ui_spool_canvas.cpp
git commit -m "refactor(ams): use shared color utils in spool canvas"
```

---

## Task 7: Refactor `ui_ams_slot.cpp` — Use Shared Pulse, Color, Contrast

Replace local pulse animation, `darken_color`, and hand-rolled contrast text.

**Files:**
- Modify: `src/ui/ui_ams_slot.cpp`

**Step 1: Build + run existing ams_slot tests**

Run: `make test && ./build/bin/helix-tests "[ams_slot]" -v`

Note passing count.

**Step 2: Replace includes and remove local functions**

1. Add `#include "ams_drawing_utils.h"` near the top
2. Delete local `darken_color()` (lines 162-167)
3. Delete local pulse constants: `ERROR_DOT_SCALE_MIN/MAX`, `ERROR_DOT_SAT_MIN/MAX`, `ERROR_DOT_PULSE_MS` (lines 417-422)
4. Delete local `error_dot_scale_anim_cb` (lines 425-435)
5. Delete local `error_dot_color_anim_cb` (lines 439-450)
6. Delete local `start_error_dot_pulse` (lines 452-484)
7. Delete local `stop_error_dot_pulse` (lines 486-494)

**Step 3: Update call sites**

- `darken_color(` → `ams_draw::darken_color(`
- `start_error_dot_pulse(` → `ams_draw::start_pulse(`
- `stop_error_dot_pulse(` → `ams_draw::stop_pulse(`

**Step 4: Fix contrast text** (lines 274-275 and 403-404)

Replace:
```cpp
int brightness = theme_compute_brightness(badge_bg);
lv_color_t text_color = (brightness > 140) ? lv_color_black() : lv_color_white();
```

With:
```cpp
lv_color_t text_color = theme_manager_get_contrast_text(badge_bg);
```

And same at lines 403-404:
```cpp
lv_color_t bg = lv_obj_get_style_bg_color(data->tool_badge_bg, LV_PART_MAIN);
lv_color_t text_color = theme_manager_get_contrast_text(bg);
```

**Step 5: Use `SlotInfo::is_present()` in `apply_slot_status`**

In `apply_slot_status()`, look for the status-based spool visibility logic and use `slot_info.is_present()` where appropriate for clarity (the existing switch statement may stay as-is since it handles each status individually — just use `is_present()` where the binary present/not-present check appears).

**Step 6: Build and run tests**

Run: `make test && ./build/bin/helix-tests "[ams_slot]" -v`

Same count as before.

**Step 7: Commit**

```bash
git add src/ui/ui_ams_slot.cpp
git commit -m "refactor(ams): use shared pulse, color, and contrast in ams_slot"
```

---

## Task 8: Refactor `ui_panel_ams_overview.cpp` — Use All Shared Utils

The biggest refactor. Remove all local helpers replaced by shared functions.

**Files:**
- Modify: `src/ui/ui_panel_ams_overview.cpp`

**Step 1: Build baseline**

Run: `make -j`

**Step 2: Replace includes**

Add `#include "ams_drawing_utils.h"` near the top.

**Step 3: Delete local duplicates**

Remove these from `ui_panel_ams_overview.cpp`:
1. Badge pulse constants (lines 72-76): `BADGE_SCALE_MIN/MAX`, `BADGE_SAT_MIN/MAX`, `BADGE_PULSE_MS`
2. `badge_scale_anim_cb` (lines 78-87)
3. `badge_color_anim_cb` (lines 89-96)
4. `start_badge_pulse` (lines 98-127)
5. `stop_badge_pulse` (lines 129-135)
6. `get_unit_display_name` (lines 141-146) — now in shared utils
7. `get_worst_unit_severity` (lines 159-167) — now in shared utils
8. Status line constants `STATUS_LINE_HEIGHT_PX` and `STATUS_LINE_GAP_PX` (lines 66-69) — use `ams_draw::` versions

**Step 4: Update `create_unit_cards()` — error badge**

Replace the inline badge creation block (lines 441-469) with:
```cpp
uc.error_badge = ams_draw::create_error_badge(uc.card, 12);
lv_obj_set_align(uc.error_badge, LV_ALIGN_TOP_RIGHT);
lv_obj_set_style_translate_x(uc.error_badge, -4, LV_PART_MAIN);
lv_obj_set_style_translate_y(uc.error_badge, 4, LV_PART_MAIN);

bool animate = SettingsManager::instance().get_animations_enabled();
auto worst = ams_draw::worst_unit_severity(unit);
ams_draw::update_error_badge(uc.error_badge, unit.has_any_error(), worst, animate);
```

**Step 5: Update `update_unit_card()` — error badge**

Replace lines 498-514 with:
```cpp
if (card.error_badge) {
    bool animate = SettingsManager::instance().get_animations_enabled();
    auto worst = ams_draw::worst_unit_severity(unit);
    ams_draw::update_error_badge(card.error_badge, unit.has_any_error(), worst, animate);
}
```

**Step 6: Update `create_mini_bars()` — use shared slot column**

Replace the per-slot loop body (lines 540-632) with calls to `ams_draw::create_slot_column()` + `ams_draw::style_slot_bar()`:

```cpp
for (int s = 0; s < slot_count; ++s) {
    const SlotInfo& slot = unit.slots[s];
    int global_idx = unit.first_slot_global_index + s;
    bool is_loaded = (global_idx == current_slot);

    auto col = ams_draw::create_slot_column(card.bars_container, bar_width,
                                            MINI_BAR_HEIGHT_PX, MINI_BAR_RADIUS_PX);

    ams_draw::BarStyleParams params;
    params.color_rgb = slot.color_rgb;
    params.fill_pct = ams_draw::fill_percent_from_slot(slot);
    params.is_present = slot.is_present();
    params.is_loaded = is_loaded;
    params.has_error = (slot.status == SlotStatus::BLOCKED || slot.error.has_value());
    params.severity = slot.error.has_value() ? slot.error->severity : SlotError::INFO;

    ams_draw::style_slot_bar(col, params, MINI_BAR_RADIUS_PX);
}
```

**Step 7: Update bar width calculation**

Replace the manual width calc (lines 535-538) with:
```cpp
int32_t bar_width = ams_draw::calc_bar_width(container_width, slot_count, gap,
                                              MINI_BAR_MIN_WIDTH_PX, MINI_BAR_MAX_WIDTH_PX);
```

**Step 8: Update name labels**

Replace `get_unit_display_name(` with `ams_draw::get_unit_display_name(` at all call sites.

**Step 9: Update logo**

Replace the logo fallback blocks (lines 416-427, 927-938) with:
```cpp
if (uc.logo_image) {
    ams_draw::apply_logo(uc.logo_image, unit, info);
}
```

**Step 10: Build**

Run: `make -j`

Fix any compile errors.

**Step 11: Commit**

```bash
git add src/ui/ui_panel_ams_overview.cpp
git commit -m "refactor(ams): use shared drawing utils in overview panel"
```

---

## Task 9: Refactor `ui_ams_mini_status.cpp` — Use Shared Utils + Adopt Overview Style

The core goal: mini widget uses shared slot bar rendering and adopts the overview visual style.

**Files:**
- Modify: `src/ui/ui_ams_mini_status.cpp`

**Step 1: Build baseline**

Run: `make -j`

**Step 2: Replace includes and remove local functions**

1. Add `#include "ams_drawing_utils.h"`
2. Delete local `lighten_color()` (lines 61-65)
3. Delete local `update_slot_bar()` (lines 126-184)
4. Delete local `create_slot_container()` (lines 192-228)
5. Delete local `STATUS_LINE_HEIGHT_PX` and `STATUS_LINE_GAP_PX` constants (lines 120-123)

**Step 3: Update `SlotBarData` struct**

Replace the separate pointers with `ams_draw::SlotColumn`:
```cpp
struct SlotBarData {
    ams_draw::SlotColumn col;          // Shared slot column (container, bar_bg, bar_fill, status_line)
    uint32_t color_rgb = 0x808080;
    int fill_pct = 100;
    bool present = false;
    bool loaded = false;
    bool has_error = false;
    SlotError::Severity severity = SlotError::INFO;
};
```

**Step 4: Update all references**

Throughout the file, replace:
- `slot->slot_container` → `slot->col.container`
- `slot->bar_bg` → `slot->col.bar_bg`
- `slot->bar_fill` → `slot->col.bar_fill`
- `slot->status_line` → `slot->col.status_line`

**Step 5: Update bar creation in `rebuild_bars()`**

Replace `create_slot_container(slot, parent)` calls with:
```cpp
slot->col = ams_draw::create_slot_column(parent, bar_width, bar_height, BAR_BORDER_RADIUS_PX);
```

Replace `update_slot_bar(slot)` calls with:
```cpp
ams_draw::BarStyleParams params;
params.color_rgb = slot->color_rgb;
params.fill_pct = slot->fill_pct;
params.is_present = slot->present;
params.is_loaded = slot->loaded;
params.is_loaded = slot->loaded;
params.has_error = slot->has_error;
params.severity = slot->severity;
ams_draw::style_slot_bar(slot->col, params, BAR_BORDER_RADIUS_PX);
```

**Step 6: Update bar width calculations**

Replace manual width math (2 locations) with:
```cpp
int32_t bar_width = ams_draw::calc_bar_width(container_width, slot_count, gap,
                                              MIN_BAR_WIDTH_PX, MAX_BAR_WIDTH_PX, 90);
```

**Step 7: Update `sync_from_ams_state()`**

Use `slot.is_present()` and `ams_draw::fill_percent_from_slot()`:
```cpp
bool present = slot.is_present();
bool loaded = (slot.status == SlotStatus::LOADED);
bool has_error = (slot.status == SlotStatus::BLOCKED || slot.error.has_value());

slot_bar->color_rgb = slot.color_rgb;
slot_bar->fill_pct = ams_draw::fill_percent_from_slot(slot, 0);
slot_bar->present = present;
slot_bar->loaded = loaded;
slot_bar->has_error = has_error;
slot_bar->severity = slot.error.has_value() ? slot.error->severity : SlotError::INFO;
```

**Step 8: Update transparent container creation**

Replace the ~7-line boilerplate for `bars_container`, `container`, and row containers with `ams_draw::create_transparent_container()` + just the layout-specific settings (flex flow, sizing, etc.).

For example, `ensure_unit_row()`:
```cpp
row->row_container = ams_draw::create_transparent_container(data->bars_container);
lv_obj_set_flex_flow(row->row_container, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(row->row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END,
                      LV_FLEX_ALIGN_CENTER);
lv_obj_set_style_pad_column(row->row_container, theme_manager_get_spacing("space_xxs"),
                            LV_PART_MAIN);
lv_obj_set_size(row->row_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
```

**Step 9: Update bar height calculation to account for new style**

The old mini status used `STATUS_LINE_HEIGHT_PX + STATUS_LINE_GAP_PX` in its height calc. Since the shared `create_slot_column` handles this internally, adjust the total slot height:
```cpp
int32_t bar_height = total_slot_height - ams_draw::STATUS_LINE_HEIGHT_PX - ams_draw::STATUS_LINE_GAP_PX;
```

**Step 10: Build and test**

Run: `make -j && ./build/bin/helix-screen --test -vv`

Visually verify the mini widget on the home panel now shows:
- Loaded slot: wider border (no green status line)
- Error slot: red/amber status line (severity-aware)
- Empty slot: ghosted (20% opacity border)

**Step 11: Commit**

```bash
git add src/ui/ui_ams_mini_status.cpp
git commit -m "refactor(ams): use shared drawing utils in mini status, adopt overview style"
```

---

## Task 10: Refactor `ui_panel_ams.cpp` — Logo + Display Name

Small cleanup — just the logo fallback and display name.

**Files:**
- Modify: `src/ui/ui_panel_ams.cpp`

**Step 1: Add include**

Add `#include "ams_drawing_utils.h"`

**Step 2: Replace logo fallback** (around lines 528-562)

Replace the two logo fallback blocks with:
```cpp
// Scoped to unit:
if (uc.logo_image) {
    ams_draw::apply_logo(system_logo, unit, info);
}
// System level:
ams_draw::apply_logo(system_logo, info.type_name);
```

**Step 3: Replace display name** (line 545)

Replace:
```cpp
std::string display_name =
    unit.name.empty() ? ("Unit " + std::to_string(scoped_unit_index_ + 1)) : unit.name;
```
With:
```cpp
std::string display_name = ams_draw::get_unit_display_name(unit, scoped_unit_index_);
```

**Step 4: Build**

Run: `make -j`

**Step 5: Commit**

```bash
git add src/ui/ui_panel_ams.cpp
git commit -m "refactor(ams): use shared logo and display name in AMS panel"
```

---

## Task 11: Final Validation + Regression

**Step 1: Full test suite**

Run: `make test-run`

All tests must pass. Compare count with Task 6 baseline — should be baseline + new `[ams_draw]` tests.

**Step 2: Run the app and visually verify**

Run: `./build/bin/helix-screen --test -vv`

Check:
- Home panel: mini AMS widget shows bars with correct loaded/error/empty styling
- AMS overview: unit cards look identical to before
- AMS detail: error dots pulse, loaded highlight works
- Multi-unit: mini widget stacked rows render correctly

**Step 3: Build for other targets** (if available)

Run: `make -j` (desktop build already done)

**Step 4: Final commit if any fixups needed**

```bash
git add -p  # Stage only the fixups
git commit -m "fix(ams): address drawing utils integration fixups"
```

---

## Summary

| Task | Description | New Tests | Files Modified |
|------|-------------|-----------|----------------|
| 1 | `SlotInfo::is_present()` | 6 | ams_types.h |
| 2 | Color utils | 6 | NEW: ams_drawing_utils.h/.cpp |
| 3 | Severity, fill, bar-width, display-name | 12 | ams_drawing_utils.h/.cpp |
| 4 | Pulse, badge, container, slot bar | 10 | ams_drawing_utils.h/.cpp |
| 5 | Logo helper | 2 | ams_drawing_utils.h/.cpp |
| 6 | Refactor spool_canvas | 0 (regression) | ui_spool_canvas.cpp |
| 7 | Refactor ams_slot | 0 (regression) | ui_ams_slot.cpp |
| 8 | Refactor overview | 0 (regression) | ui_panel_ams_overview.cpp |
| 9 | Refactor mini status | 0 (regression) | ui_ams_mini_status.cpp |
| 10 | Refactor AMS panel | 0 (regression) | ui_panel_ams.cpp |
| 11 | Final validation | 0 | — |

**Total new tests:** ~36
**Total commits:** ~11
**Estimated net code reduction:** ~200 lines
