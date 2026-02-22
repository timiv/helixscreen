# Z-Offset Extraction Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Extract duplicated Z-offset save/apply/format logic into shared `ZOffsetUtils`, fix tune overlay save bug, slim controls panel.

**Architecture:** Create `helix::zoffset` namespace with formatting and save utilities. Three consumers (controls panel, tune overlay, calibration overlay) delegate to shared functions instead of duplicating strategy-aware command dispatch.

**Tech Stack:** C++17, LVGL subjects, MoonrakerAPI (async gcode execution), Catch2 tests.

**Design doc:** `docs/devel/plans/2026-02-21-zoffset-extraction-design.md`

---

### Task 1: Create `z_offset_utils.h` with format functions + tests

**Files:**
- Create: `include/z_offset_utils.h`
- Create: `src/ui/z_offset_utils.cpp`
- Create: `tests/unit/test_z_offset_utils.cpp`

**Step 1: Write failing tests for format functions**

Create `tests/unit/test_z_offset_utils.cpp`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "z_offset_utils.h"
#include "../catch_amalgamated.hpp"

#include <cstring>

using namespace helix::zoffset;

// ============================================================================
// format_delta tests
// ============================================================================

TEST_CASE("format_delta: zero microns produces empty string", "[zoffset][format]") {
    char buf[32] = "garbage";
    format_delta(0, buf, sizeof(buf));
    REQUIRE(buf[0] == '\0');
}

TEST_CASE("format_delta: positive microns formats with plus sign", "[zoffset][format]") {
    char buf[32] = {};
    format_delta(50, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "+0.050mm");
}

TEST_CASE("format_delta: negative microns formats with minus sign", "[zoffset][format]") {
    char buf[32] = {};
    format_delta(-25, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "-0.025mm");
}

TEST_CASE("format_delta: large positive value", "[zoffset][format]") {
    char buf[32] = {};
    format_delta(1500, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "+1.500mm");
}

// ============================================================================
// format_offset tests
// ============================================================================

TEST_CASE("format_offset: zero microns shows +0.000mm", "[zoffset][format]") {
    char buf[32] = {};
    format_offset(0, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "+0.000mm");
}

TEST_CASE("format_offset: positive microns", "[zoffset][format]") {
    char buf[32] = {};
    format_offset(100, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "+0.100mm");
}

TEST_CASE("format_offset: negative microns", "[zoffset][format]") {
    char buf[32] = {};
    format_offset(-250, buf, sizeof(buf));
    REQUIRE(std::string(buf) == "-0.250mm");
}

// ============================================================================
// is_auto_saved tests
// ============================================================================

TEST_CASE("is_auto_saved: GCODE_OFFSET returns true", "[zoffset][strategy]") {
    // Note: is_auto_saved also shows a toast, but we can't verify that in unit tests.
    // The important contract is the return value.
    REQUIRE(is_auto_saved(ZOffsetCalibrationStrategy::GCODE_OFFSET) == true);
}

TEST_CASE("is_auto_saved: PROBE_CALIBRATE returns false", "[zoffset][strategy]") {
    REQUIRE(is_auto_saved(ZOffsetCalibrationStrategy::PROBE_CALIBRATE) == false);
}

TEST_CASE("is_auto_saved: ENDSTOP returns false", "[zoffset][strategy]") {
    REQUIRE(is_auto_saved(ZOffsetCalibrationStrategy::ENDSTOP) == false);
}
```

**Step 2: Run tests to verify they fail**

Run: `make test && ./build/bin/helix-tests "[zoffset]" -v`
Expected: Compilation error — `z_offset_utils.h` doesn't exist yet.

**Step 3: Write minimal header and implementation**

Create `include/z_offset_utils.h`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "printer_state.h"  // ZOffsetCalibrationStrategy

#include <cstddef>
#include <functional>
#include <string>

class MoonrakerAPI;

namespace helix::zoffset {

/// Returns true and shows toast if strategy auto-persists (GCODE_OFFSET).
/// Callers should return early when this returns true.
bool is_auto_saved(ZOffsetCalibrationStrategy strategy);

/// Format microns as "+0.050mm" or "-0.025mm". Empty string if microns == 0.
void format_delta(int microns, char* buf, size_t buf_size);

/// Format microns as "+0.050mm" (always shows value, even for 0).
void format_offset(int microns, char* buf, size_t buf_size);

/// Execute strategy-aware save sequence:
///   PROBE_CALIBRATE → Z_OFFSET_APPLY_PROBE → SAVE_CONFIG
///   ENDSTOP → Z_OFFSET_APPLY_ENDSTOP → SAVE_CONFIG
///   GCODE_OFFSET → warns and returns (should not be called)
///
/// @param api           Moonraker API for gcode execution (must not be null)
/// @param strategy      Calibration strategy determining command sequence
/// @param on_success    Called after SAVE_CONFIG succeeds (Klipper will restart)
/// @param on_error      Called with user-facing message on any failure
void apply_and_save(MoonrakerAPI* api, ZOffsetCalibrationStrategy strategy,
                    std::function<void()> on_success,
                    std::function<void(const std::string& error)> on_error);

} // namespace helix::zoffset
```

Create `src/ui/z_offset_utils.cpp`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "z_offset_utils.h"

#include "moonraker_api.h"
#include "ui_toast_manager.h"

#include <spdlog/spdlog.h>
#include <lvgl.h>

#include <cstdio>

namespace helix::zoffset {

bool is_auto_saved(ZOffsetCalibrationStrategy strategy) {
    if (strategy == ZOffsetCalibrationStrategy::GCODE_OFFSET) {
        spdlog::debug("[ZOffsetUtils] Z-offset auto-saved by firmware (gcode_offset strategy)");
        ToastManager::instance().show(ToastSeverity::INFO,
                                      lv_tr("Z-offset is auto-saved by firmware"), 3000);
        return true;
    }
    return false;
}

void format_delta(int microns, char* buf, size_t buf_size) {
    if (microns == 0) {
        buf[0] = '\0';
        return;
    }
    double mm = static_cast<double>(microns) / 1000.0;
    std::snprintf(buf, buf_size, "%+.3fmm", mm);
}

void format_offset(int microns, char* buf, size_t buf_size) {
    double mm = static_cast<double>(microns) / 1000.0;
    std::snprintf(buf, buf_size, "%+.3fmm", mm);
}

void apply_and_save(MoonrakerAPI* api, ZOffsetCalibrationStrategy strategy,
                    std::function<void()> on_success,
                    std::function<void(const std::string& error)> on_error) {
    if (!api) {
        spdlog::error("[ZOffsetUtils] apply_and_save called with null API");
        if (on_error) on_error("No printer connection");
        return;
    }

    if (strategy == ZOffsetCalibrationStrategy::GCODE_OFFSET) {
        spdlog::warn("[ZOffsetUtils] apply_and_save called with gcode_offset strategy — ignoring");
        if (on_success) on_success();
        return;
    }

    const char* apply_cmd = (strategy == ZOffsetCalibrationStrategy::PROBE_CALIBRATE)
                                ? "Z_OFFSET_APPLY_PROBE"
                                : "Z_OFFSET_APPLY_ENDSTOP";

    const char* strategy_name = (strategy == ZOffsetCalibrationStrategy::PROBE_CALIBRATE)
                                    ? "probe_calibrate"
                                    : "endstop";

    spdlog::info("[ZOffsetUtils] Applying Z-offset with {} strategy (cmd: {})",
                 strategy_name, apply_cmd);

    api->execute_gcode(
        apply_cmd,
        [api, apply_cmd, on_success, on_error]() {
            spdlog::info("[ZOffsetUtils] {} success, executing SAVE_CONFIG", apply_cmd);

            api->execute_gcode(
                "SAVE_CONFIG",
                [on_success]() {
                    spdlog::info("[ZOffsetUtils] SAVE_CONFIG success — Klipper restarting");
                    if (on_success) on_success();
                },
                [on_error](const MoonrakerError& err) {
                    std::string msg = fmt::format(
                        "SAVE_CONFIG failed: {}. Z-offset was applied but not saved. "
                        "Run SAVE_CONFIG manually or the offset will be lost on restart.",
                        err.user_message());
                    spdlog::error("[ZOffsetUtils] {}", msg);
                    if (on_error) on_error(msg);
                });
        },
        [apply_cmd, on_error](const MoonrakerError& err) {
            std::string msg = fmt::format("{} failed: {}", apply_cmd, err.user_message());
            spdlog::error("[ZOffsetUtils] {}", msg);
            if (on_error) on_error(msg);
        });
}

} // namespace helix::zoffset
```

**Step 4: Run tests to verify they pass**

Run: `make test && ./build/bin/helix-tests "[zoffset]" -v`
Expected: All 9 tests PASS.

**Step 5: Commit**

```bash
git add include/z_offset_utils.h src/ui/z_offset_utils.cpp tests/unit/test_z_offset_utils.cpp
git commit -m "refactor(zoffset): add shared ZOffsetUtils with format and save helpers"
```

---

### Task 2: Migrate controls panel to use ZOffsetUtils

**Files:**
- Modify: `src/ui/ui_panel_controls.cpp` (lines 752-897)
- Modify: `include/ui_panel_controls.h` (remove private helpers that move to utils)

**Step 1: Replace format functions in controls panel**

In `ui_panel_controls.cpp`, replace `update_z_offset_delta_display`:

```cpp
// Before (lines 752-766): inline formatting
// After:
void ControlsPanel::update_z_offset_delta_display(int delta_microns) {
    helix::zoffset::format_delta(delta_microns, z_offset_delta_display_buf_,
                                  sizeof(z_offset_delta_display_buf_));
    lv_subject_copy_string(&z_offset_delta_display_subject_, z_offset_delta_display_buf_);
    spdlog::trace("[{}] Z-offset delta display updated: '{}'", get_name(),
                  z_offset_delta_display_buf_);
}
```

Replace `update_controls_z_offset_display`:

```cpp
// Before (lines 768-772): inline formatting
// After:
void ControlsPanel::update_controls_z_offset_display(int offset_microns) {
    helix::zoffset::format_offset(offset_microns, controls_z_offset_buf_,
                                   sizeof(controls_z_offset_buf_));
    lv_subject_copy_string(&controls_z_offset_subject_, controls_z_offset_buf_);
}
```

**Step 2: Replace save logic with `is_auto_saved` + `apply_and_save`**

Replace `handle_save_z_offset` (lines 781-824):

```cpp
void ControlsPanel::handle_save_z_offset() {
    auto strategy = printer_state_.get_z_offset_calibration_strategy();
    if (helix::zoffset::is_auto_saved(strategy)) return;

    int offset_microns = 0;
    if (auto* subj = printer_state_.get_gcode_z_offset_subject()) {
        offset_microns = lv_subject_get_int(subj);
    }

    if (offset_microns == 0) {
        spdlog::debug("[{}] No Z-offset adjustment to save", get_name());
        return;
    }

    spdlog::info("[{}] Save Z-offset clicked: {:+.3f}mm", get_name(),
                 static_cast<double>(offset_microns) / 1000.0);

    const char* confirm_msg =
        (strategy == ZOffsetCalibrationStrategy::PROBE_CALIBRATE)
            ? lv_tr("This will apply the Z-offset to your probe and restart Klipper to save the "
                    "configuration. The printer will briefly disconnect.")
            : lv_tr("This will apply the Z-offset to your endstop and restart Klipper to save the "
                    "configuration. The printer will briefly disconnect.");

    save_z_offset_confirmation_dialog_ = helix::ui::modal_show_confirmation(
        lv_tr("Save Z-Offset?"), confirm_msg, ModalSeverity::Warning, lv_tr("Save"),
        on_save_z_offset_confirm, on_save_z_offset_cancel, this);

    if (!save_z_offset_confirmation_dialog_) {
        LOG_ERROR_INTERNAL("Failed to create save Z-offset confirmation dialog");
        NOTIFY_ERROR("Failed to show confirmation dialog");
        return;
    }
}
```

Replace `handle_save_z_offset_confirm` (lines 826-897):

```cpp
void ControlsPanel::handle_save_z_offset_confirm() {
    spdlog::debug("[{}] Save Z-offset confirmed", get_name());

    if (save_z_offset_in_progress_) {
        spdlog::warn("[{}] Save Z-offset already in progress, ignoring", get_name());
        return;
    }
    save_z_offset_in_progress_ = true;
    save_z_offset_confirmation_dialog_.hide();

    int offset_microns = 0;
    if (auto* subj = printer_state_.get_gcode_z_offset_subject()) {
        offset_microns = lv_subject_get_int(subj);
    }
    double offset_mm = static_cast<double>(offset_microns) / 1000.0;

    auto strategy = printer_state_.get_z_offset_calibration_strategy();
    NOTIFY_INFO("Saving Z-offset...");

    helix::zoffset::apply_and_save(
        api_, strategy,
        [this, offset_mm]() {
            NOTIFY_SUCCESS("Z-offset saved ({:+.3f}mm). Klipper restarting...", offset_mm);
            save_z_offset_in_progress_ = false;
        },
        [this](const std::string& error) {
            NOTIFY_ERROR("{}", error);
            save_z_offset_in_progress_ = false;
        });
}
```

Add `#include "z_offset_utils.h"` to the includes.

**Step 3: Build and run existing tests**

Run: `make -j`
Expected: Clean build, no errors.

Run: `make test && ./build/bin/helix-tests "[zoffset]" -v`
Expected: All tests pass.

**Step 4: Commit**

```bash
git add src/ui/ui_panel_controls.cpp include/ui_panel_controls.h
git commit -m "refactor(controls): use ZOffsetUtils for Z-offset format and save"
```

---

### Task 3: Fix tune overlay save bug + migrate to ZOffsetUtils

**Files:**
- Modify: `src/ui/ui_print_tune_overlay.cpp` (lines 510-541)

**Step 1: Replace `handle_save_z_offset` in tune overlay**

The current code does a bare `SAVE_CONFIG` without `Z_OFFSET_APPLY_PROBE`/`Z_OFFSET_APPLY_ENDSTOP` first. This means for probe/endstop strategies, the offset is not applied to the config before saving — the save is incomplete.

Replace `handle_save_z_offset` (lines 510-541):

```cpp
void PrintTuneOverlay::handle_save_z_offset() {
    if (printer_state_) {
        auto strategy = printer_state_->get_z_offset_calibration_strategy();
        if (helix::zoffset::is_auto_saved(strategy)) return;
    }

    save_z_offset_modal_.set_on_confirm([this]() {
        if (!api_ || !printer_state_) return;

        auto strategy = printer_state_->get_z_offset_calibration_strategy();
        helix::zoffset::apply_and_save(
            api_, strategy,
            []() {
                spdlog::info("[PrintTuneOverlay] Z-offset saved — Klipper restarting");
                ToastManager::instance().show(ToastSeverity::WARNING,
                                              lv_tr("Z-offset saved - Klipper restarting..."),
                                              5000);
            },
            [](const std::string& error) {
                spdlog::error("[PrintTuneOverlay] Save failed: {}", error);
                NOTIFY_ERROR("Save failed: {}", error);
            });
    });
    save_z_offset_modal_.show(lv_screen_active());
}
```

Add `#include "z_offset_utils.h"` to the includes.

**Step 2: Build**

Run: `make -j`
Expected: Clean build.

**Step 3: Commit**

```bash
git add src/ui/ui_print_tune_overlay.cpp
git commit -m "fix(tune): apply Z_OFFSET_APPLY_PROBE/ENDSTOP before SAVE_CONFIG"
```

---

### Task 4: Simplify calibration panel callback pyramid

**Files:**
- Modify: `src/ui/ui_panel_calibration_zoffset.cpp` (lines 591-709)

**Step 1: Simplify `send_accept` for probe/endstop strategies**

The GCODE_OFFSET path stays as-is (it does SET_GCODE_OFFSET, which is unique to calibration).

Replace the probe/endstop branch (lines 625-708) to use `apply_and_save` after ACCEPT:

```cpp
    } else {
        // Probe/endstop: ACCEPT then apply+save
        spdlog::info("[ZOffsetCal] Sending ACCEPT");
        set_state(State::SAVING);

        api_->execute_gcode(
            "ACCEPT",
            [this, strategy]() {
                spdlog::info("[ZOffsetCal] ACCEPT success, applying and saving");
                helix::zoffset::apply_and_save(
                    api_, strategy,
                    [this]() {
                        helix::ui::async_call(
                            [](void* ud) {
                                static_cast<ZOffsetCalibrationPanel*>(ud)->on_calibration_result(
                                    true, "");
                            },
                            this);
                    },
                    [this](const std::string& error) {
                        struct Ctx {
                            ZOffsetCalibrationPanel* panel;
                            std::string msg;
                        };
                        auto ctx = std::make_unique<Ctx>(Ctx{this, error});
                        helix::ui::queue_update<Ctx>(std::move(ctx), [](Ctx* c) {
                            c->panel->on_calibration_result(false, c->msg);
                        });
                    });
            },
            [this](const MoonrakerError& err) {
                struct Ctx {
                    ZOffsetCalibrationPanel* panel;
                    std::string msg;
                };
                auto ctx =
                    std::make_unique<Ctx>(Ctx{this, "ACCEPT failed: " + err.user_message()});
                helix::ui::queue_update<Ctx>(
                    std::move(ctx),
                    [](Ctx* c) { c->panel->on_calibration_result(false, c->msg); });
            });
    }
```

Add `#include "z_offset_utils.h"` to the includes.

**Step 2: Build**

Run: `make -j`
Expected: Clean build.

**Step 3: Commit**

```bash
git add src/ui/ui_panel_calibration_zoffset.cpp
git commit -m "refactor(zoffset-cal): simplify save callback chain with ZOffsetUtils"
```

---

### Task 5: Run full test suite and update refactor plan

**Step 1: Run full test suite**

Run: `make test-run`
Expected: All tests pass, no regressions.

**Step 2: Update refactor plan**

In `docs/devel/plans/REFACTOR_PLAN.md`, update Section 2.5 status to complete with a summary note similar to other completed sections.

**Step 3: Final commit**

```bash
git add docs/devel/plans/REFACTOR_PLAN.md docs/devel/plans/2026-02-21-zoffset-extraction-design.md
git commit -m "docs: update refactor plan with Z-offset extraction completion"
```
