# Panel Widget Decoupling Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Decouple widgets from HomePanel so any panel can host them, rename HomeWidget → PanelWidget, introduce PanelWidgetManager singleton.

**Architecture:** Widgets self-register their factories. A PanelWidgetManager singleton owns config (per-panel), shared resources, and lifecycle. Panels become thin consumers calling `manager.populate_widgets(panel_id, container)`.

**Tech Stack:** C++17, LVGL 9.4, Catch2 (tests), XML components

**Design doc:** `docs/plans/2026-02-21-panel-widget-decoupling-design.md`

---

### Task 1: Rename files and symbols — HomeWidget → PanelWidget

This is a pure mechanical rename with no behavioral changes. Do it all at once so we have a clean baseline.

**Files to rename (git mv):**
- `include/home_widget.h` → `include/panel_widget.h`
- `include/home_widget_registry.h` → `include/panel_widget_registry.h`
- `include/home_widget_config.h` → `include/panel_widget_config.h`
- `src/ui/home_widget_registry.cpp` → `src/ui/panel_widget_registry.cpp`
- `src/system/home_widget_config.cpp` → `src/system/panel_widget_config.cpp`
- `include/ui_settings_home_widgets.h` → `include/ui_settings_panel_widgets.h`
- `src/ui/ui_settings_home_widgets.cpp` → `src/ui/ui_settings_panel_widgets.cpp`
- `src/ui/home_widgets/` → `src/ui/panel_widgets/` (entire directory)
- `tests/unit/test_home_widget_config.cpp` → `tests/unit/test_panel_widget_config.cpp`
- `tests/unit/test_thermistor_widget.cpp` → `tests/unit/test_panel_widget_thermistor.cpp`

**XML files to rename (git mv):**
- `ui_xml/home_widget_row.xml` → `ui_xml/panel_widget_row.xml`
- `ui_xml/home_widgets_overlay.xml` → `ui_xml/panel_widgets_overlay.xml`
- All 13 `ui_xml/components/home_widget_*.xml` → `ui_xml/components/panel_widget_*.xml`

**Symbol renames (find-and-replace across all affected files):**
- `HomeWidget` → `PanelWidget` (class name)
- `HomeWidgetDef` → `PanelWidgetDef`
- `HomeWidgetEntry` → `PanelWidgetEntry`
- `HomeWidgetConfig` → `PanelWidgetConfig`
- `HomeWidgetsOverlay` → `PanelWidgetsOverlay`
- `home_widget_from_event` → `panel_widget_from_event`
- `#include "home_widget` → `#include "panel_widget`
- `#include "ui_settings_home_widgets.h"` → `#include "ui_settings_panel_widgets.h"`

**String literals to update:**
- In `ui_panel_home.cpp` line 350: `"home_widget_"` prefix → `"panel_widget_"`
- In `ui_panel_home.cpp` line 357: `"home_widget_firmware_restart"` → `"panel_widget_firmware_restart"`
- In `ui_panel_home.cpp` line 361: same
- In `ui_panel_home.cpp` line 410: `.substr(12)` → `.substr(13)` (strip `"panel_widget_"` which is 13 chars)
- In `ui_panel_home.cpp` line 543-547: `"home_widget_area"` → `"panel_widget_area"` (injection point name)
- In XML component names within each XML file: update `<component name="home_widget_*">` → `<component name="panel_widget_*">`
- In `ui_xml/home_widget_row.xml`: update component name
- In `ui_xml/home_widgets_overlay.xml`: update component name and any internal references to `home_widget_row`
- In `home_widget_registry.cpp` (now `panel_widget_registry.cpp`): no string changes needed — widget IDs like `"power"`, `"temperature"` stay the same

**Test tags to update:**
- `[home][widget_config]` → `[panel_widget][widget_config]`
- `[thermistor][home_widget]` → `[thermistor][panel_widget]`

**Step 1:** `git mv` all files listed above

**Step 2:** Find-and-replace all symbol names listed above across the renamed files AND all files that include them. Key files that include widget headers:
- `include/ui_panel_home.h` (line 11)
- `src/ui/ui_panel_home.cpp` (lines 30-31)
- All widget .h files (include base class)
- `src/application/subject_initializer.cpp` (if it includes widget headers)
- `src/ui/ui_panel_settings.cpp` (includes widget config)
- Test files

**Step 3:** Update XML component name attributes in all renamed XML files

**Step 4:** Build with `make -j` — fix any compilation errors

**Step 5:** Run widget tests: `./build/bin/helix-tests "[panel_widget]"` — verify all pass

**Step 6:** Commit
```bash
git add -A
git commit -m "refactor(widgets): rename HomeWidget → PanelWidget across codebase"
```

---

### Task 2: Create PanelWidgetManager singleton

New singleton that will own widget lifecycle, config, shared resources, and gate observers.

**Files:**
- Create: `include/panel_widget_manager.h`
- Create: `src/ui/panel_widget_manager.cpp`

**Step 1: Write failing test**

Create `tests/unit/test_panel_widget_manager.cpp`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_test_macros.hpp>
#include "panel_widget_manager.h"

TEST_CASE("PanelWidgetManager singleton access", "[panel_widget][manager]") {
    auto& mgr = helix::PanelWidgetManager::instance();
    // Should not crash, should be same instance
    auto& mgr2 = helix::PanelWidgetManager::instance();
    REQUIRE(&mgr == &mgr2);
}

TEST_CASE("PanelWidgetManager shared resources", "[panel_widget][manager]") {
    auto& mgr = helix::PanelWidgetManager::instance();

    // No resource registered yet
    REQUIRE(mgr.shared_resource<int>() == nullptr);

    // Register a shared resource
    auto val = std::make_shared<int>(42);
    mgr.register_shared_resource<int>(val);
    REQUIRE(mgr.shared_resource<int>() != nullptr);
    REQUIRE(*mgr.shared_resource<int>() == 42);

    // Cleanup
    mgr.clear_shared_resources();
    REQUIRE(mgr.shared_resource<int>() == nullptr);
}

TEST_CASE("PanelWidgetManager config change callbacks", "[panel_widget][manager]") {
    auto& mgr = helix::PanelWidgetManager::instance();

    bool called = false;
    mgr.register_rebuild_callback("test_panel", [&called]() { called = true; });

    mgr.notify_config_changed("test_panel");
    REQUIRE(called);

    // Unknown panel — no crash
    mgr.notify_config_changed("nonexistent");

    mgr.unregister_rebuild_callback("test_panel");
}
```

**Step 2:** Run test, verify it fails (header not found)

Run: `make test && ./build/bin/helix-tests "[panel_widget][manager]"`
Expected: FAIL — `panel_widget_manager.h` not found

**Step 3: Implement PanelWidgetManager**

Create `include/panel_widget_manager.h`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

struct lv_obj_t;

namespace helix {

class PanelWidget;

/// Singleton manager for panel widgets. Owns shared resources,
/// per-panel config callbacks, and widget lifecycle.
class PanelWidgetManager {
  public:
    static PanelWidgetManager& instance();

    // -- Shared resources --
    // Widgets request shared objects (e.g. TempControlPanel) by type.

    template <typename T> void register_shared_resource(std::shared_ptr<T> resource) {
        shared_resources_[std::type_index(typeid(T))] = std::move(resource);
    }

    template <typename T> T* shared_resource() const {
        auto it = shared_resources_.find(std::type_index(typeid(T)));
        if (it == shared_resources_.end())
            return nullptr;
        return std::any_cast<std::shared_ptr<T>>(it->second).get();
    }

    void clear_shared_resources();

    // -- Per-panel rebuild callbacks --
    // Panels register a callback; settings overlay or gate observers trigger it.

    using RebuildCallback = std::function<void()>;
    void register_rebuild_callback(const std::string& panel_id, RebuildCallback cb);
    void unregister_rebuild_callback(const std::string& panel_id);
    void notify_config_changed(const std::string& panel_id);

    // -- Widget lifecycle --
    // Populates a container with widgets for a given panel.
    // Returns ownership of behavioral widgets to the caller.

    std::vector<std::unique_ptr<PanelWidget>> populate_widgets(
        const std::string& panel_id, lv_obj_t* container);

    // -- Gate observers --
    // Watches hardware gate subjects and triggers rebuild on change.

    void setup_gate_observers(const std::string& panel_id, RebuildCallback rebuild_cb);
    void clear_gate_observers(const std::string& panel_id);

  private:
    PanelWidgetManager() = default;

    std::unordered_map<std::type_index, std::any> shared_resources_;
    std::unordered_map<std::string, RebuildCallback> rebuild_callbacks_;
};

} // namespace helix
```

Create `src/ui/panel_widget_manager.cpp`:

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_widget_manager.h"
#include "panel_widget.h"
#include "panel_widget_config.h"
#include "panel_widget_registry.h"

#include <spdlog/spdlog.h>

namespace helix {

PanelWidgetManager& PanelWidgetManager::instance() {
    static PanelWidgetManager s_instance;
    return s_instance;
}

void PanelWidgetManager::clear_shared_resources() {
    shared_resources_.clear();
}

void PanelWidgetManager::register_rebuild_callback(const std::string& panel_id,
                                                    RebuildCallback cb) {
    rebuild_callbacks_[panel_id] = std::move(cb);
}

void PanelWidgetManager::unregister_rebuild_callback(const std::string& panel_id) {
    rebuild_callbacks_.erase(panel_id);
}

void PanelWidgetManager::notify_config_changed(const std::string& panel_id) {
    auto it = rebuild_callbacks_.find(panel_id);
    if (it != rebuild_callbacks_.end()) {
        it->second();
    }
}

} // namespace helix
```

Leave `populate_widgets()` and gate observer methods as stubs for now — they'll be implemented when we extract from HomePanel in Task 4.

**Step 4:** Build and run tests

Run: `make test && ./build/bin/helix-tests "[panel_widget][manager]"`
Expected: PASS

**Step 5:** Commit
```bash
git add include/panel_widget_manager.h src/ui/panel_widget_manager.cpp tests/unit/test_panel_widget_manager.cpp
git commit -m "feat(widgets): add PanelWidgetManager singleton with shared resources and callbacks"
```

---

### Task 3: Widget self-registration

Move factory registration out of `HomePanel::init_subjects()` and into each widget's .cpp file via static initializers.

**Files:**
- Modify: `src/ui/panel_widgets/temperature_widget.cpp`
- Modify: `src/ui/panel_widgets/temp_stack_widget.cpp`
- Modify: `src/ui/panel_widgets/led_widget.cpp`
- Modify: `src/ui/panel_widgets/power_widget.cpp`
- Modify: `src/ui/panel_widgets/network_widget.cpp`
- Modify: `src/ui/panel_widgets/thermistor_widget.cpp`
- Modify: `src/ui/ui_panel_home.cpp` (remove factory registration from init_subjects)

**Step 1: Write failing test**

Add to `tests/unit/test_panel_widget_manager.cpp`:

```cpp
#include "panel_widget_registry.h"

TEST_CASE("Widget factories are self-registered", "[panel_widget][self_registration]") {
    // These factories should exist via static initializers, not HomePanel
    const char* expected_factories[] = {
        "temperature", "temp_stack", "led", "power", "network", "thermistor"
    };
    for (const auto* id : expected_factories) {
        const auto* def = helix::find_widget_def(id);
        REQUIRE(def != nullptr);
        REQUIRE(def->factory != nullptr);
    }
}
```

**Step 2:** Run test, verify it fails

The factories are currently only registered when HomePanel::init_subjects() runs, which doesn't happen in tests.

Run: `make test && ./build/bin/helix-tests "[self_registration]"`
Expected: FAIL — factories are nullptr

**Step 3: Add static self-registration to each widget**

For each widget .cpp file, add a static initializer at file scope. The factories pull dependencies from singletons and the PanelWidgetManager's shared resources instead of capturing HomePanel.

Example for `temperature_widget.cpp`:
```cpp
#include "panel_widget_manager.h"
#include "panel_widget_registry.h"
#include "printer_state.h"

// Self-register factory
static const bool s_registered = [] {
    helix::register_widget_factory("temperature", []() {
        auto* temp_panel =
            helix::PanelWidgetManager::instance().shared_resource<TempControlPanel>();
        return std::make_unique<helix::TemperatureWidget>(
            PrinterState::instance(), temp_panel);
    });
    return true;
}();
```

Repeat pattern for each widget:
- `power_widget.cpp`: `MoonrakerAPI` from `PrinterState::instance().api()` or shared resource
- `led_widget.cpp`: `PrinterState::instance()`, `MoonrakerAPI` from shared resource or singleton
- `network_widget.cpp`: No deps (already takes none)
- `temp_stack_widget.cpp`: Same as temperature — `PrinterState`, `TempControlPanel*` from manager
- `thermistor_widget.cpp`: `PrinterState::instance()`

**Step 4:** Remove factory registration from `HomePanel::init_subjects()`

In `src/ui/ui_panel_home.cpp`, delete lines 229-243 (the `register_widget_factory` block). HomePanel no longer registers any factories.

**Step 5:** In `subject_initializer.cpp`, register TempControlPanel as a shared resource:

After the existing `set_temp_control_panel()` calls (line 248), add:
```cpp
helix::PanelWidgetManager::instance().register_shared_resource<TempControlPanel>(
    m_temp_control_panel);
```

This makes TempControlPanel available to widget factories via the manager.

**Step 6:** Build and run tests

Run: `make test && ./build/bin/helix-tests "[panel_widget]"`
Expected: All pass, including the new self-registration test

**Step 7:** Commit
```bash
git add -A
git commit -m "refactor(widgets): self-register widget factories, remove HomePanel coupling"
```

---

### Task 4: Extract populate_widgets() into PanelWidgetManager

Move the widget creation logic from HomePanel into PanelWidgetManager so any panel can use it.

**Files:**
- Modify: `include/panel_widget_manager.h`
- Modify: `src/ui/panel_widget_manager.cpp`
- Modify: `src/ui/ui_panel_home.cpp` — replace populate_widgets() body with manager call
- Modify: `include/ui_panel_home.h` — remove active_widgets_ and widget_gate_observers_ members

**Step 1: Write failing test**

Add to `tests/unit/test_panel_widget_manager.cpp`:

```cpp
TEST_CASE("PanelWidgetManager populate returns widgets", "[panel_widget][manager][populate]") {
    auto& mgr = helix::PanelWidgetManager::instance();

    // With default config and no LVGL context, populate should handle gracefully
    // (returns empty vector since lv_xml_create will fail without LVGL init)
    auto widgets = mgr.populate_widgets("home", nullptr);
    REQUIRE(widgets.empty()); // No LVGL context = no widgets created
}
```

**Step 2:** Run test, verify it fails (method is stub)

**Step 3: Implement populate_widgets()**

Move the logic from `HomePanel::populate_widgets()` (lines 314-434) into `PanelWidgetManager::populate_widgets()`. Key changes:
- Takes `panel_id` and `container` parameters instead of using HomePanel members
- Uses `PanelWidgetConfig(panel_id)` for per-panel config
- Returns `vector<unique_ptr<PanelWidget>>` instead of storing in member
- Widgets call `lv_scr_act()` for parent screen (no `parent_screen_` param)
- XML prefix is `"panel_widget_"` (13 chars to strip)

Also implement `setup_gate_observers()` and `clear_gate_observers()` — extracted from `HomePanel::setup_widget_gate_observers()`.

**Step 4: Simplify HomePanel**

Replace HomePanel's populate_widgets() body:
```cpp
void HomePanel::populate_widgets() {
    auto& mgr = helix::PanelWidgetManager::instance();
    // Detach old widgets
    for (auto& w : active_widgets_) w->detach();
    active_widgets_ = mgr.populate_widgets("home", widget_container_);
}
```

Remove `setup_widget_gate_observers()` from HomePanel — replace in setup() with:
```cpp
auto& mgr = helix::PanelWidgetManager::instance();
mgr.setup_gate_observers("home", [this]() { populate_widgets(); });
```

Move `widget_gate_observers_` out of HomePanel into the manager (per panel ID).

**Step 5:** Build and run tests

Run: `make -j && make test && ./build/bin/helix-tests "[panel_widget]"`

**Step 6:** Run full app to verify widgets still render:
```bash
./build/bin/helix-screen --test -vv
```

**Step 7:** Commit
```bash
git add -A
git commit -m "refactor(widgets): extract populate_widgets into PanelWidgetManager"
```

---

### Task 5: Per-panel config with migration

Update PanelWidgetConfig to support per-panel config sections and migrate the old `home_widgets` key.

**Files:**
- Modify: `include/panel_widget_config.h`
- Modify: `src/system/panel_widget_config.cpp`
- Modify: `tests/unit/test_panel_widget_config.cpp`

**Step 1: Write failing test**

Add to `tests/unit/test_panel_widget_config.cpp`:

```cpp
TEST_CASE("PanelWidgetConfig loads per-panel section", "[panel_widget][widget_config]") {
    // Write config with new per-panel format
    nlohmann::json config = {
        {"panel_widgets", {
            {"home", {{{"id", "power"}, {"enabled", true}, {"config", {}}}}},
            {"controls", {{{"id", "temperature"}, {"enabled", true}, {"config", {}}}}}
        }}
    };
    // ... test that PanelWidgetConfig("home", ...) loads the home section
    // ... test that PanelWidgetConfig("controls", ...) loads the controls section
}

TEST_CASE("PanelWidgetConfig migrates legacy home_widgets key", "[panel_widget][widget_config][migration]") {
    // Write config with legacy format
    nlohmann::json config = {
        {"home_widgets", {{{"id", "power"}, {"enabled", true}, {"config", {}}}}}
    };
    // ... test that PanelWidgetConfig("home", ...) migrates and loads correctly
    // ... test that after save, "panel_widgets.home" exists and "home_widgets" is removed
}
```

**Step 2:** Run test, verify it fails

**Step 3: Update PanelWidgetConfig**

- Constructor takes `panel_id` parameter: `PanelWidgetConfig(const std::string& panel_id, Config& config)`
- `load()` reads from `config["panel_widgets"][panel_id_]`
- If `panel_id_ == "home"` and `config["home_widgets"]` exists but `config["panel_widgets"]["home"]` does not, migrate the old key
- `save()` writes to `config["panel_widgets"][panel_id_]`
- After migration, remove `"home_widgets"` key from config

**Step 4:** Build and run tests

Run: `make test && ./build/bin/helix-tests "[panel_widget][widget_config]"`
Expected: All pass

**Step 5:** Commit
```bash
git add -A
git commit -m "feat(widgets): per-panel widget config with legacy migration"
```

---

### Task 6: Decouple settings overlay from HomePanel

Make the settings overlay use PanelWidgetManager instead of directly calling `get_global_home_panel().populate_widgets()`.

**Files:**
- Modify: `src/ui/ui_settings_panel_widgets.cpp`

**Step 1:** Replace the `get_global_home_panel().populate_widgets()` call (line 145) with:
```cpp
helix::PanelWidgetManager::instance().notify_config_changed("home");
```

**Step 2:** Remove `#include "ui_panel_home.h"` if no longer needed and remove the `extern HomePanel&` declaration.

**Step 3:** Build and run

Run: `make -j && ./build/bin/helix-screen --test -vv`
Verify: Toggle a widget in settings, confirm it appears/disappears on home panel.

**Step 4:** Commit
```bash
git add -A
git commit -m "refactor(widgets): decouple settings overlay from HomePanel"
```

---

### Task 7: Move NetworkWidget subjects into the widget

NetworkWidget currently looks up `home_network_icon_state` and `network_label` subjects that HomePanel creates. Move those into NetworkWidget itself.

**Files:**
- Modify: `src/ui/panel_widgets/network_widget.cpp`
- Modify: `src/ui/panel_widgets/network_widget.h`
- Modify: `src/ui/ui_panel_home.cpp` — remove network subject creation from init_subjects()

**Step 1:** In `network_widget.h`, add subject members:
```cpp
lv_subject_t network_icon_state_;
lv_subject_t network_label_subject_;
char network_label_buffer_[64] = "WiFi";
```

**Step 2:** In `network_widget.cpp` `attach()`, create the subjects instead of looking them up:
```cpp
lv_subject_init_int(&network_icon_state_, 0);
lv_subject_init_string(&network_label_subject_, network_label_buffer_);
// Register with XML system if needed for binding
```

**Step 3:** In `detach()`, clean up:
```cpp
lv_subject_deinit(&network_icon_state_);
lv_subject_deinit(&network_label_subject_);
```

**Step 4:** Remove the network subject creation from `HomePanel::init_subjects()` — find and remove the `UI_MANAGED_SUBJECT_INT(network_icon_state_` and `UI_MANAGED_SUBJECT_STRING(network_label_subject_` lines.

**Step 5:** Build and run

Run: `make -j && ./build/bin/helix-screen --test -vv`
Verify: Network widget still shows correct state.

**Step 6:** Commit
```bash
git add -A
git commit -m "refactor(widgets): NetworkWidget owns its own subjects"
```

---

### Task 8: Remove remaining get_global_home_panel() from widgets

Clean up any remaining `extern HomePanel& get_global_home_panel()` references in widget files. Widgets should route through NavigationManager, singletons, or PanelWidgetManager — not HomePanel directly.

**Files to audit (grep for `get_global_home_panel` in panel_widgets/):**
- `power_widget.cpp` — uses it for `handle_power_toggle()` and `handle_power_long_press()`
- `led_widget.cpp` — uses it for light toggle/long press
- `temperature_widget.cpp` — uses it for temp clicked
- `network_widget.cpp` — uses it for network clicked

**Step 1:** For each widget, identify what HomePanel method it calls and find the right replacement:
- If it's opening an overlay → use `NavigationManager` directly
- If it's calling a HomePanel-specific method → move that method to the widget or a shared utility
- If it's accessing HomePanel state → use PrinterState or PanelWidgetManager shared resources

**Step 2:** Remove all `extern HomePanel& get_global_home_panel()` declarations from widget files.

**Step 3:** Build and run

Run: `make -j && ./build/bin/helix-screen --test -vv`

**Step 4:** Verify no remaining references:
```bash
grep -r "get_global_home_panel" src/ui/panel_widgets/
```
Expected: No matches

**Step 5:** Commit
```bash
git add -A
git commit -m "refactor(widgets): remove all get_global_home_panel references from widgets"
```

---

### Task 9: Final cleanup and verification

**Step 1:** Verify no remaining `HomeWidget` references (should all be `PanelWidget`):
```bash
grep -r "HomeWidget" include/ src/ tests/ --include="*.h" --include="*.cpp"
```
Expected: No matches (except possibly in git history comments)

**Step 2:** Verify no remaining `home_widget` in include paths:
```bash
grep -r '#include.*home_widget' include/ src/ tests/
```
Expected: No matches

**Step 3:** Run full test suite:
```bash
make test-run
```
Expected: All tests pass

**Step 4:** Run the app and test:
```bash
./build/bin/helix-screen --test -vv
```
Verify: Home panel widgets render, toggle in settings works, hardware gating works.

**Step 5:** Commit any remaining cleanup
```bash
git add -A
git commit -m "refactor(widgets): final cleanup of panel widget decoupling"
```
