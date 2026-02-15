# Mock Backend Deduplication Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Eliminate hardcoded data duplication between `AmsBackendMock` and `AmsBackendAfc` so the mock can't silently drift from real AFC behavior.

**Architecture:** Extract shared AFC definitions (sections, default actions, default capabilities) into a static data module (`afc_defaults`). Both the real AFC backend and the mock consume it. The real backend augments with config-file-driven dynamic values; the mock uses defaults as-is.

**Tech Stack:** C++17, Catch2 tests

---

## The Problem

The mock backend has **3 separate hardcoded copies** of AFC section data and ~150 lines of hardcoded action definitions that duplicate what the real AFC backend defines. When AFC evolves (new sections, new actions, new config parameters), the mock gets stale silently — tests pass but don't reflect reality.

Additionally, 7 of 10 capability fields in `system_info_` are identical between mock and real backends but written independently.

## Approach

**Shared static defaults** — a header+source pair that defines:
- Default AFC sections (the 8 sections)
- Default AFC actions (the ~11 basic actions with their metadata)
- Default AFC capabilities (the shared capability flags)

The mock becomes thin: calls the shared defaults, stores them, done. The real AFC backend calls the same defaults, then *overlays* dynamic config-file values on top.

This is NOT a base-class refactor. The backends remain separate classes with separate responsibilities. We're just extracting the *data* they share.

---

### Task 1: Create `afc_defaults` shared data module

**Files:**
- Create: `include/afc_defaults.h`
- Create: `src/printer/afc_defaults.cpp`

**Step 1: Write the failing test**

Create `tests/unit/test_afc_defaults.cpp`:

```cpp
#include "afc_defaults.h"
#include "../catch_amalgamated.hpp"

using namespace helix::printer;

TEST_CASE("AFC default sections", "[ams][afc][defaults]") {
    auto sections = afc_default_sections();

    SECTION("returns all 8 sections") {
        REQUIRE(sections.size() == 8);
    }

    SECTION("sections are in display_order") {
        for (size_t i = 1; i < sections.size(); i++) {
            CHECK(sections[i].display_order > sections[i - 1].display_order);
        }
    }

    SECTION("all sections have id, label, description") {
        for (const auto& s : sections) {
            CHECK_FALSE(s.id.empty());
            CHECK_FALSE(s.label.empty());
            CHECK_FALSE(s.description.empty());
        }
    }

    SECTION("known section IDs present") {
        auto has_id = [&](const std::string& id) {
            return std::any_of(sections.begin(), sections.end(),
                               [&](const DeviceSection& s) { return s.id == id; });
        };
        CHECK(has_id("calibration"));
        CHECK(has_id("speed"));
        CHECK(has_id("maintenance"));
        CHECK(has_id("led"));
        CHECK(has_id("hub"));
        CHECK(has_id("tip_forming"));
        CHECK(has_id("purge"));
        CHECK(has_id("config"));
    }
}

TEST_CASE("AFC default capabilities", "[ams][afc][defaults]") {
    auto caps = afc_default_capabilities();

    CHECK(caps.supports_endless_spool == true);
    CHECK(caps.supports_spoolman == true);
    CHECK(caps.supports_tool_mapping == true);
    CHECK(caps.supports_bypass == true);
    CHECK(caps.supports_purge == true);
    CHECK(caps.tip_method == TipMethod::CUT);
}

TEST_CASE("AFC default actions", "[ams][afc][defaults]") {
    auto actions = afc_default_actions();

    SECTION("returns non-empty list") {
        CHECK_FALSE(actions.empty());
    }

    SECTION("all actions have id, label, section_id") {
        for (const auto& a : actions) {
            CHECK_FALSE(a.id.empty());
            CHECK_FALSE(a.label.empty());
            CHECK_FALSE(a.section_id.empty());
        }
    }

    SECTION("calibration_wizard action exists") {
        auto it = std::find_if(actions.begin(), actions.end(),
                               [](const DeviceAction& a) { return a.id == "calibration_wizard"; });
        REQUIRE(it != actions.end());
        CHECK(it->section_id == "calibration");
    }

    SECTION("speed actions exist") {
        auto it = std::find_if(actions.begin(), actions.end(),
                               [](const DeviceAction& a) { return a.id == "speed_fwd"; });
        REQUIRE(it != actions.end());
        CHECK(it->section_id == "speed");
    }
}
```

**Step 2: Run test to verify it fails**

Run: `make test && ./build/bin/helix-tests "[defaults]"`
Expected: FAIL — `afc_defaults.h` doesn't exist

**Step 3: Write the header**

Create `include/afc_defaults.h`:

```cpp
// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ams_types.h"
#include <vector>

namespace helix::printer {

/// Shared AFC capability flags (type-independent defaults)
struct AfcCapabilities {
    bool supports_endless_spool = true;
    bool supports_spoolman = true;
    bool supports_tool_mapping = true;
    bool supports_bypass = true;
    bool supports_purge = true;
    TipMethod tip_method = TipMethod::CUT;
};

/// Default AFC device sections (all 8)
std::vector<DeviceSection> afc_default_sections();

/// Default AFC device actions (static metadata — no config-file values)
std::vector<DeviceAction> afc_default_actions();

/// Default AFC capabilities
AfcCapabilities afc_default_capabilities();

} // namespace helix::printer
```

**Step 4: Write the implementation**

Create `src/printer/afc_defaults.cpp` — move the hardcoded section/action/capability data here. This becomes the single source of truth.

The section list comes from `AmsBackendAfc::get_device_sections()`.
The action list comes from `AmsBackendAfc::get_device_actions()` (only the static metadata — slider ranges, labels, types — not config-file-driven current values).
The capabilities come from the shared flags in both constructors.

**Step 5: Run tests, verify pass**

Run: `make test && ./build/bin/helix-tests "[defaults]"`

**Step 6: Commit**

```bash
git add include/afc_defaults.h src/printer/afc_defaults.cpp tests/unit/test_afc_defaults.cpp
git commit -m "feat(ams): extract shared AFC defaults module"
```

---

### Task 2: Wire `AmsBackendAfc` to use shared defaults

**Files:**
- Modify: `src/printer/ams_backend_afc.cpp`

**Step 1: Replace `get_device_sections()` body**

```cpp
std::vector<helix::printer::DeviceSection> AmsBackendAfc::get_device_sections() const {
    return afc_default_sections();
}
```

**Step 2: Replace capability initialization in constructor**

Replace the 7 duplicated `system_info_.*` lines with:

```cpp
auto caps = afc_default_capabilities();
system_info_.supports_endless_spool = caps.supports_endless_spool;
system_info_.supports_spoolman = caps.supports_spoolman;
system_info_.supports_tool_mapping = caps.supports_tool_mapping;
system_info_.supports_bypass = caps.supports_bypass;
system_info_.supports_purge = caps.supports_purge;
system_info_.tip_method = caps.tip_method;
```

**Step 3: Replace static action metadata in `get_device_actions()`**

Start from `afc_default_actions()`, then overlay config-file-driven current values (bowden length from `bowden_length_`, config-parsed slider values, etc.). The structure becomes:

```cpp
auto actions = afc_default_actions();
// Overlay dynamic values from config
for (auto& action : actions) {
    if (action.id == "bowden_length") {
        action.current_value = bowden_length_;
    }
    // ... other config-driven overrides
}
// Append config-file-only actions (hub settings, tip forming, purge)
// These are ONLY available when config files are parsed
```

**Step 4: Run existing AFC tests**

Run: `make test && ./build/bin/helix-tests "[afc]"`
Expected: All pass — behavior unchanged

**Step 5: Commit**

```bash
git commit -m "refactor(ams): use shared defaults in AFC backend"
```

---

### Task 3: Wire `AmsBackendMock` to use shared defaults

**Files:**
- Modify: `src/printer/ams_backend_mock.cpp`

**Step 1: Replace constructor section/action initialization**

Replace the hardcoded section and action blocks with calls to `afc_default_sections()` and `afc_default_actions()`.

For the default (Happy Hare-like) mode, filter to just calibration+speed sections:
```cpp
auto all = afc_default_sections();
// Default mode: just calibration and speed
std::copy_if(all.begin(), all.end(), std::back_inserter(mock_device_sections_),
             [](const DeviceSection& s) { return s.id == "calibration" || s.id == "speed"; });
```

**Step 2: Replace `set_afc_mode()` section/action initialization**

Use `afc_default_sections()` and `afc_default_actions()` directly instead of hardcoded copies.

**Step 3: Replace `set_tool_changer_mode()` revert path**

Same pattern — filter shared defaults instead of hardcoding.

**Step 4: Replace capability initialization**

Use `afc_default_capabilities()` instead of hardcoded flags.

**Step 5: Run all tests**

Run: `make test-run`
Expected: All pass

**Step 6: Commit**

```bash
git commit -m "refactor(ams): use shared defaults in mock backend"
```

---

### Task 4: Clean up and verify no remaining duplication

**Step 1: Grep for any remaining hardcoded section/action strings**

Search for hardcoded section IDs ("calibration", "speed", "maintenance") in mock backend — should only appear in `set_*_mode()` filter predicates, not in struct initializers.

**Step 2: Verify test coverage**

Run: `make test-run`
All existing tests must pass — this is a pure refactor.

**Step 3: Commit**

```bash
git commit -m "chore(ams): verify no remaining mock/AFC data duplication"
```
