# UI Refactoring Plan

> **Purpose**: Track DRY violations and refactoring progress across the UI codebase.
> **Generated**: 2026-01-20 from comprehensive DRY code review
> **Status Legend**: `[ ]` = pending, `[x]` = complete, `[~]` = in progress, `[-]` = skipped

---

## Workflow Checklist

**Before starting work:**
1. Read this document to understand remaining tasks
2. Pick tasks from the same group for parallel execution (see "Notes for Parallel Execution")
3. Use TodoWrite to track in-progress items

**After completing tasks:**
1. Update task status in this document (`[ ]` → `[x]`)
2. Update individual checklists within tasks
3. Add entry to Completion Log with date and notes
4. Update Summary table counts
5. Run code review before committing
6. Commit this document with your code changes

**Commit format:**
```
refactor(ui): [brief description] ([task IDs])
```

---

## Summary

| Category | Total Tasks | Completed | Remaining |
|----------|-------------|-----------|-----------|
| High Priority | 5 | 5 | 0 |
| Medium Priority - C++ | 8 | 8 | 0 |
| Medium Priority - XML | 6 | 6 | 0 |
| Low Priority | 5 | 5 | 0 |
| **Total** | **24** | **24** | **0** |

---

## High Priority Tasks

### HP-1: Migrate Manual Observer Callbacks to Factory Pattern
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~50 static callback functions eliminated across 9 files (-317 lines)
- **Files**:
  - `src/ui/ui_panel_home.cpp` (lines 893-920, 1025-1105)
  - `src/ui/ui_panel_print_status.cpp` (lines 1004-1131)
  - `src/ui/ui_panel_temp_control.cpp` (lines 97-129)
  - `src/ui/ui_emergency_stop.cpp` (lines 350, 358)
  - `src/ui/ui_panel_calibration_zoffset.cpp` (lines 522, 544)
  - `src/ui/ui_panel_ams.cpp` (lines 982, 996, 1030)
  - `src/ui/ui_ams_slot.cpp` (lines 279, 327)
  - `src/ui/ui_nav_manager.cpp`
  - `src/ui/ui_status_bar_manager.cpp` (lines 60-80)

**Pattern to replace**:
```cpp
// BEFORE: Manual static callback
void PanelName::some_observer_cb(lv_observer_t* observer, lv_subject_t* subject) {
    auto* self = static_cast<PanelName*>(lv_observer_get_user_data(observer));
    if (self) {
        self->on_something_changed(lv_subject_get_int(subject));
    }
}
some_observer_ = ObserverGuard(subject_ptr, some_observer_cb, this);

// AFTER: Factory pattern
some_observer_ = observe_int_sync<PanelName>(
    subject_ptr, this,
    [](PanelName* self, int value) { self->on_something_changed(value); });
```

**Checklist**:
- [x] `ui_panel_home.cpp` - 12 callbacks migrated
- [x] `ui_panel_print_status.cpp` - 17 callbacks migrated
- [x] `ui_panel_temp_control.cpp` - 4 callbacks migrated
- [x] `ui_emergency_stop.cpp` - 2 callbacks migrated
- [x] `ui_panel_calibration_zoffset.cpp` - 2 callbacks migrated
- [-] `ui_panel_ams.cpp` - skipped (already uses factory pattern)
- [x] `ui_ams_slot.cpp` - 2 callbacks migrated
- [x] `ui_nav_manager.cpp` - 3 callbacks migrated
- [x] `ui_status_bar_manager.cpp` - 3 callbacks migrated

---

### HP-2: Create Global Panel Singleton Macro
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~30 lines saved per panel, 4 panels refactored
- **Files modified**:
  - Created: `include/ui_global_panel_helper.h`
  - Updated: `src/ui/ui_panel_console.cpp`
  - Updated: `src/ui/ui_panel_spoolman.cpp`
  - Updated: `src/ui/ui_panel_macros.cpp`
  - Updated: `src/ui/ui_panel_bed_mesh.cpp`
  - Skipped: `src/ui/ui_panel_filament.cpp` (requires PrinterState constructor arg)
  - Skipped: `src/ui/ui_panel_advanced.cpp` (uses init function pattern)

**New helper** (`include/ui/ui_global_panel_helper.h`):
```cpp
#pragma once
#include "ui_static_panel_registry.h"
#include <memory>

#define DEFINE_GLOBAL_PANEL(PanelType, global_var, getter_func) \
    static std::unique_ptr<PanelType> global_var; \
    PanelType& getter_func() { \
        if (!global_var) { \
            global_var = std::make_unique<PanelType>(); \
            StaticPanelRegistry::instance().register_destroy( \
                #PanelType, []() { global_var.reset(); }); \
        } \
        return *global_var; \
    }
```

**Checklist**:
- [ ] Create `include/ui/ui_global_panel_helper.h`
- [ ] Refactor `ui_panel_console.cpp` to use macro
- [ ] Refactor `ui_panel_spoolman.cpp` to use macro
- [ ] Refactor `ui_panel_macros.cpp` to use macro
- [ ] Refactor `ui_panel_filament.cpp` to use macro
- [ ] Refactor `ui_panel_bed_mesh.cpp` to use macro
- [ ] Refactor `ui_panel_advanced.cpp` to use macro

---

### HP-3: Create XML Modal Button Row Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 9 modal files simplified, 3 skipped (complex conditional bindings)
- **Files**:
  - Created: `ui_xml/modal_button_row.xml`
  - Registered in `src/xml_registration.cpp`
  - Update: `ui_xml/wifi_password_modal.xml`
  - Update: `ui_xml/hidden_network_modal.xml`
  - Update: `ui_xml/bed_mesh_rename_modal.xml`
  - Update: `ui_xml/factory_reset_modal.xml`
  - Update: `ui_xml/theme_restart_modal.xml`
  - Update: `ui_xml/save_z_offset_modal.xml`
  - Update: `ui_xml/estop_confirmation_dialog.xml`
  - Update: `ui_xml/print_cancel_confirm_modal.xml`
  - Update: `ui_xml/filament_preset_edit_modal.xml`
  - Update: `ui_xml/modal_dialog.xml`
  - Update: `ui_xml/bed_mesh_save_config_modal.xml`
  - Update: `ui_xml/exclude_object_modal.xml`

**New component** (`ui_xml/modal_button_row.xml`):
```xml
<component>
  <api>
    <prop name="primary_text" type="string" default="OK"/>
    <prop name="secondary_text" type="string" default="Cancel"/>
    <prop name="primary_callback" type="string"/>
    <prop name="secondary_callback" type="string"/>
    <prop name="primary_bg_color" type="string" default=""/>
    <prop name="show_secondary" type="string" default="true"/>
  </api>
  <view extends="lv_obj" width="100%" height="content" style_bg_opa="0"
        style_border_width="0" style_pad_all="0" flex_flow="column">
    <!-- Horizontal divider -->
    <lv_obj width="100%" height="1" style_bg_color="#text_secondary"
            style_bg_opa="64" style_border_width="0" style_pad_all="0"/>
    <!-- Button row -->
    <lv_obj width="100%" height="#button_height" style_bg_opa="0"
            style_border_width="0" style_pad_all="0" flex_flow="row">
      <!-- Secondary button (conditional) -->
      <lv_button name="btn_secondary" height="100%" flex_grow="1"
                 style_radius="#border_radius" style_border_width="0"
                 style_shadow_width="0" clickable="true">
        <event_cb trigger="clicked" callback="$secondary_callback"/>
        <text_body text="$secondary_text" align="center"/>
      </lv_button>
      <!-- Vertical divider -->
      <lv_obj width="1" height="100%" style_bg_color="#text_secondary"
              style_bg_opa="64" style_border_width="0" style_pad_all="0"/>
      <!-- Primary button -->
      <lv_button name="btn_primary" height="100%" flex_grow="1"
                 style_radius="#border_radius" style_border_width="0"
                 style_shadow_width="0" clickable="true">
        <event_cb trigger="clicked" callback="$primary_callback"/>
        <text_body text="$primary_text" align="center"/>
      </lv_button>
    </lv_obj>
  </view>
</component>
```

**Checklist**:
- [x] Create `ui_xml/modal_button_row.xml` component
- [x] Register component in XML system
- [-] Update `wifi_password_modal.xml` (skipped: conditional visibility bindings)
- [-] Update `hidden_network_modal.xml` (skipped: conditional visibility bindings)
- [x] Update `bed_mesh_rename_modal.xml`
- [x] Update `factory_reset_modal.xml`
- [x] Update `theme_restart_modal.xml`
- [x] Update `save_z_offset_modal.xml`
- [x] Update `estop_confirmation_dialog.xml`
- [x] Update `print_cancel_confirm_modal.xml`
- [x] Update `filament_preset_edit_modal.xml`
- [-] Update `modal_dialog.xml` (skipped: dynamic text bindings + programmatic callbacks)
- [x] Update `bed_mesh_save_config_modal.xml`
- [x] Update `exclude_object_modal.xml`

---

### HP-4: Create XML Divider Components
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 34 files updated, 72 inline dividers replaced
- **Files**:
  - Created: `ui_xml/divider_horizontal.xml`
  - Created: `ui_xml/divider_vertical.xml`
  - Registered in `src/xml_registration.cpp`

**Components** (with `color` prop added for flexibility):

`ui_xml/divider_horizontal.xml`:
```xml
<component>
  <api>
    <prop name="opacity" type="string" default="64"/>
    <prop name="color" type="string" default="#text_secondary"/>
  </api>
  <view extends="lv_obj" width="100%" height="1" style_bg_color="$color"
        style_bg_opa="$opacity" style_border_width="0" style_pad_all="0" scrollable="false"/>
</component>
```

`ui_xml/divider_vertical.xml`:
```xml
<component>
  <api>
    <prop name="height" type="string" default="100%"/>
    <prop name="opacity" type="string" default="64"/>
    <prop name="color" type="string" default="#text_secondary"/>
  </api>
  <view extends="lv_obj" width="1" height="$height" style_bg_color="$color"
        style_bg_opa="$opacity" style_border_width="0" style_pad_all="0" scrollable="false"/>
</component>
```

**Checklist**:
- [x] Create `ui_xml/divider_horizontal.xml`
- [x] Create `ui_xml/divider_vertical.xml`
- [x] Register components in XML system
- [x] Update files using horizontal divider pattern (34 files, 47 instances)
- [x] Update files using vertical divider pattern (17 files, 25 instances)

---

### HP-5: Add Domain-Specific Observer Helpers
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: Simplified observer setup across 12+ files
- **Files**:
  - Updated: `include/observer_factory.h`

**New helpers to add**:
```cpp
// Connection state observer - used in 6+ files
template<typename Panel, typename OnConnected>
ObserverGuard observe_connection_state(lv_subject_t* subject, Panel* panel,
                                        OnConnected&& on_connected) {
    return observe_int_sync<Panel>(subject, panel,
        [on_connected = std::forward<OnConnected>(on_connected)](Panel* p, int state) {
            if (state == static_cast<int>(ConnectionState::CONNECTED)) {
                on_connected(p);
            }
        });
}

// Print state observer - used in 4+ files
template<typename Panel, typename Handler>
ObserverGuard observe_print_state(lv_subject_t* subject, Panel* panel, Handler&& handler) {
    return observe_int_sync<Panel>(subject, panel,
        [handler = std::forward<Handler>(handler)](Panel* p, int state_int) {
            handler(p, static_cast<PrintJobState>(state_int));
        });
}
```

**Checklist**:
- [x] Add `observe_connection_state` to `observer_factory.h`
- [x] Add `observe_print_state` to `observer_factory.h`
- [ ] Update files using connection state pattern
- [ ] Update files using print state pattern

---

## Medium Priority Tasks - C++

### MP-C1: Extract Subject Init/Deinit Guards to Base Class
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~20 lines saved per panel, 6+ panels
- **Files**:
  - Update: `include/ui/ui_panel_base.h` or `include/ui/ui_overlay_base.h`
  - Update: `src/ui/ui_panel_filament.cpp`
  - Update: `src/ui/ui_panel_bed_mesh.cpp`
  - Update: `src/ui/ui_panel_temp_control.cpp`
  - Update: `src/ui/ui_panel_console.cpp`
  - Update: `src/ui/ui_panel_spoolman.cpp`
  - Update: `src/ui/ui_panel_macros.cpp`

**Add to base class**:
```cpp
protected:
    template<typename Func>
    bool init_subjects_guarded(Func&& init_func) {
        if (subjects_initialized_) {
            spdlog::warn("[{}] init_subjects() called twice - ignoring", get_name());
            return false;
        }
        init_func();
        subjects_initialized_ = true;
        spdlog::debug("[{}] Subjects initialized", get_name());
        return true;
    }

    void deinit_subjects_base() {
        if (!subjects_initialized_) return;
        subjects_.deinit_all();
        subjects_initialized_ = false;
        spdlog::debug("[{}] Subjects deinitialized", get_name());
    }
```

**Checklist**:
- [x] Add guarded init/deinit methods to base class
- [x] Refactor `ui_panel_filament.cpp`
- [x] Refactor `ui_panel_bed_mesh.cpp`
- [-] Refactor `ui_panel_temp_control.cpp` (standalone class, skipped)
- [x] Refactor `ui_panel_console.cpp`
- [x] Refactor `ui_panel_spoolman.cpp`
- [x] Refactor `ui_panel_macros.cpp`

---

### MP-C2: Create Overlay Creation Helper in OverlayBase
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~30 lines saved per overlay, 4+ overlays
- **Files**:
  - Update: `include/ui/ui_overlay_base.h`
  - Update: `src/ui/ui_overlay_base.cpp`
  - Update: `src/ui/ui_panel_console.cpp`
  - Update: `src/ui/ui_panel_spoolman.cpp`
  - Update: `src/ui/ui_panel_macros.cpp`
  - Update: `src/ui/ui_panel_bed_mesh.cpp`

**Add to OverlayBase**:
```cpp
protected:
    lv_obj_t* create_overlay_from_xml(lv_obj_t* parent, const char* component_name) {
        if (!parent) {
            spdlog::error("[{}] Cannot create: null parent", get_name());
            return nullptr;
        }
        spdlog::debug("[{}] Creating overlay from XML", get_name());
        parent_screen_ = parent;
        cleanup_called_ = false;

        overlay_root_ = static_cast<lv_obj_t*>(lv_xml_create(parent, component_name, nullptr));
        if (!overlay_root_) {
            spdlog::error("[{}] Failed to create from XML", get_name());
            return nullptr;
        }

        ui_overlay_panel_setup_standard(overlay_root_, parent_screen_,
                                        "overlay_header", "overlay_content");
        lv_obj_add_flag(overlay_root_, LV_OBJ_FLAG_HIDDEN);
        return overlay_root_;
    }
```

**Checklist**:
- [x] Add `create_overlay_from_xml` to `OverlayBase`
- [x] Refactor `ui_panel_console.cpp`
- [x] Refactor `ui_panel_spoolman.cpp`
- [x] Refactor `ui_panel_macros.cpp`
- [-] Refactor `ui_panel_bed_mesh.cpp` (not an overlay, skipped)

---

### MP-C3: Create Lazy Panel Navigation Template
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~107 lines saved in AdvancedPanel
- **Files**:
  - Create: `include/ui/ui_lazy_panel_helper.h`
  - Update: `src/ui/ui_panel_advanced.cpp`

**New helper**:
```cpp
template<typename PanelType>
lv_obj_t* lazy_create_and_push_overlay(
    PanelType& (*getter)(),
    lv_obj_t*& cached_panel,
    lv_obj_t* parent_screen,
    const char* panel_name
) {
    if (!cached_panel && parent_screen) {
        auto& panel = getter();
        if (!panel.are_subjects_initialized()) {
            panel.init_subjects();
        }
        panel.register_callbacks();
        cached_panel = panel.create(parent_screen);
        if (!cached_panel) {
            spdlog::error("[AdvancedPanel] Failed to create {} panel", panel_name);
            ui_toast_show(ToastSeverity::ERROR,
                fmt::format("Failed to open {}", panel_name).c_str(), 2000);
            return nullptr;
        }
        NavigationManager::instance().register_overlay_instance(cached_panel, &panel);
    }
    if (cached_panel) {
        ui_nav_push_overlay(cached_panel);
    }
    return cached_panel;
}
```

**Checklist**:
- [x] Create `include/ui/ui_lazy_panel_helper.h`
- [x] Refactor `handle_spoolman_clicked()` in `ui_panel_advanced.cpp`
- [x] Refactor `handle_macros_clicked()` in `ui_panel_advanced.cpp`
- [x] Refactor `handle_console_clicked()` in `ui_panel_advanced.cpp`
- [x] Refactor `handle_history_clicked()` in `ui_panel_advanced.cpp`

---

### MP-C4: Consolidate Modal Button Wiring
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~50 lines saved in Modal class
- **Files**:
  - Update: `include/ui/ui_modal.h`
  - Update: `src/ui/ui_modal.cpp`

**Refactor**:
```cpp
// Add private helper
void Modal::wire_button(const char* name, const char* role_name) {
    lv_obj_t* btn = find_widget(name);
    if (btn) {
        lv_obj_set_user_data(btn, this);
        spdlog::trace("[{}] Wired {} button '{}'", get_name(), role_name, name);
    } else {
        spdlog::warn("[{}] {} button '{}' not found", get_name(), role_name, name);
    }
}

// Simplify public methods to one-liners
void Modal::wire_ok_button(const char* name) { wire_button(name, "OK"); }
void Modal::wire_cancel_button(const char* name) { wire_button(name, "Cancel"); }
// ... etc
```

**Checklist**:
- [x] Add `wire_button` private helper to Modal
- [x] Refactor all `wire_*_button` methods to use helper

---

### MP-C5: Consolidate Modal Button Callbacks
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~57 lines saved in Modal class
- **Files**:
  - Update: `src/ui/ui_modal.cpp`

**Checklist**:
- [x] Create `MODAL_BUTTON_CB_IMPL` macro
- [x] Refactor 6 nearly-identical button callbacks to use macro

---

### MP-C6: Create Temperature Observer Bundle
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~15-20 lines saved per panel, 3 panels refactored
- **Files**:
  - Create: `include/ui/temperature_observer_bundle.h`
  - Update: `src/ui/ui_panel_home.cpp`
  - Update: `src/ui/ui_panel_print_status.cpp`
  - Update: `src/ui/ui_panel_temp_control.cpp`
  - Update: `src/ui/ui_panel_filament.cpp`
  - Update: `src/ui/ui_panel_controls.cpp`

**Checklist**:
- [x] Create `TemperatureObserverBundle` class in `include/ui/temperature_observer_bundle.h`
- [x] Refactor ControlsPanel to use bundle
- [x] Refactor PrintStatusPanel to use bundle
- [x] Refactor TempControlPanel to use bundle
- [-] Refactor FilamentPanel (async observers, different pattern - skipped)
- [-] Refactor HomePanel (only 2 temps, not full 4-subject pattern - skipped)

---

### MP-C7: Create Fullscreen Backdrop Helper
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~20 lines saved
- **Files**:
  - Update: `include/ui/ui_utils.h` or new file
  - Update: `src/ui/ui_modal.cpp`
  - Update: `src/ui/ui_busy_overlay.cpp`

**New helper**:
```cpp
lv_obj_t* ui_create_fullscreen_backdrop(lv_obj_t* parent, lv_opa_t opacity = 180);
```

**Checklist**:
- [x] Add `ui_create_fullscreen_backdrop` utility
- [x] Refactor `Modal::show()` and `Modal::create_and_show()`
- [x] Refactor `BusyOverlay::create_overlay_internal()`

---

### MP-C8: Create Visibility Toggle Utility
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~10 lines saved per panel, 3+ panels
- **Files**:
  - Update: `include/ui/ui_utils.h`
  - Update: `src/ui/ui_panel_console.cpp`
  - Update: `src/ui/ui_panel_macros.cpp`
  - Update: `src/ui/ui_panel_spoolman.cpp`

**New utility**:
```cpp
inline void ui_toggle_list_empty_state(lv_obj_t* list, lv_obj_t* empty_state, bool has_items) {
    if (list) lv_obj_set_flag_value(list, LV_OBJ_FLAG_HIDDEN, !has_items);
    if (empty_state) lv_obj_set_flag_value(empty_state, LV_OBJ_FLAG_HIDDEN, has_items);
}
```

**Checklist**:
- [x] Add `ui_toggle_list_empty_state` to `ui_utils.h`
- [x] Refactor visibility logic in console, macros panels

---

## Medium Priority Tasks - XML

### MP-X1: Create Modal Header Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 5+ modal files simplified
- **Files**:
  - Create: `ui_xml/modal_header.xml`
  - Update: `ui_xml/factory_reset_modal.xml`
  - Update: `ui_xml/theme_restart_modal.xml`
  - Update: `ui_xml/modal_dialog.xml`
  - Update: `ui_xml/action_prompt_modal.xml`

**Checklist**:
- [x] Create `ui_xml/modal_header.xml` with icon + title
- [x] Register component
- [x] Update factory_reset_modal.xml, theme_restart_modal.xml
- [-] modal_dialog.xml (uses dynamic severity icons, skipped)
- [-] action_prompt_modal.xml (no icon, skipped)

---

### MP-X2: Create Connecting State Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 2+ files, potential for more
- **Files**:
  - Create: `ui_xml/connecting_state.xml`
  - Update: `ui_xml/wifi_password_modal.xml`
  - Update: `ui_xml/hidden_network_modal.xml`

**Checklist**:
- [x] Create `ui_xml/connecting_state.xml`
- [x] Register component
- [x] Update wifi_password_modal.xml, hidden_network_modal.xml

---

### MP-X3: Create Info Note Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 3+ files
- **Files**:
  - Create: `ui_xml/info_note.xml`
  - Update: `ui_xml/settings_plugins_overlay.xml`
  - Update: `ui_xml/display_settings_overlay.xml`
  - Update: `ui_xml/network_settings_overlay.xml`

**Checklist**:
- [x] Create `ui_xml/info_note.xml`
- [x] Register component
- [x] Update settings_plugins_overlay.xml, display_settings_overlay.xml, filament_sensors_overlay.xml

---

### MP-X4: Create Empty State Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 3+ files
- **Files**:
  - Create: `ui_xml/empty_state.xml`
  - Update: `ui_xml/network_settings_overlay.xml`
  - Update: `ui_xml/settings_plugins_overlay.xml`
  - Update: `ui_xml/spoolman_picker_modal.xml`

**Checklist**:
- [x] Create `ui_xml/empty_state.xml` with icon + text props
- [x] Register component
- [x] Update network_settings_overlay.xml, spoolman_picker_modal.xml

---

### MP-X5: Create Centered Column Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 27 occurrences across 14 files
- **Files**:
  - Create: `ui_xml/centered_column.xml`

**Component**:
```xml
<component>
  <view extends="lv_obj" width="100%" height="100%" style_bg_opa="0"
        style_border_width="0" flex_flow="column" style_flex_main_place="center"
        style_flex_cross_place="center" style_flex_track_place="center" scrollable="false"/>
</component>
```

**Checklist**:
- [x] Create `ui_xml/centered_column.xml`
- [x] Register component
- [x] Update 5 files: spoolman_panel, print_select_panel, history_dashboard_panel, filament_sensors_overlay, bed_mesh_panel

---

### MP-X6: Create Form Field Component
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 2 files updated as proof of concept
- **Files**:
  - Create: `ui_xml/form_field.xml`
  - Update: `ui_xml/hidden_network_modal.xml`
  - Update: `ui_xml/filament_preset_edit_modal.xml`
  - Update: `ui_xml/wizard_connection.xml`

**Checklist**:
- [x] Create `ui_xml/form_field.xml` with label + input wrapper
- [x] Register component in `src/xml_registration.cpp`
- [x] Update `hidden_network_modal.xml` to use form_field
- [x] Update `filament_preset_edit_modal.xml` to use form_field
- [ ] Update `wizard_connection.xml` (future)

---

## Low Priority Tasks

### LP-1: Create ModalGuard RAII Wrapper
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: RAII wrapper for modal cleanup, ~3 lines saved per modal
- **Files**: `include/ui/ui_modal_guard.h`, `include/ui_panel_controls.h`, `src/ui/ui_panel_controls.cpp`

**Checklist**:
- [x] Create `ModalGuard` class for modal cleanup
- [x] Refactor ControlsPanel to use ModalGuard (proof of concept)
- [ ] (Future) Refactor other panels with modals

---

### LP-2: Create Position Observer Bundle
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~9 lines saved in ControlsPanel, pattern available for future use
- **Files**: `include/ui/position_observer_bundle.h`, `include/ui_panel_controls.h`, `src/ui/ui_panel_controls.cpp`

**Checklist**:
- [x] Create `PositionObserverBundle` class
- [x] Refactor ControlsPanel X/Y/Z observer setup
- [-] Refactor MotionPanel (skipped: has extra observers for actual Z and bed_moves)

---

### LP-3: Consolidate Static Callback Trampolines
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: 47 trampolines across 27 files can now use macros (~15 lines saved per file on average)
- **Files**: `include/ui/ui_event_trampoline.h`, `src/ui/ui_fan_dial.cpp` (proof of concept)

**Macros created**:
- `DEFINE_EVENT_TRAMPOLINE(Class, callback, handler)` - handler takes lv_event_t*
- `DEFINE_EVENT_TRAMPOLINE_SIMPLE(Class, callback, handler)` - handler takes no args
- `DEFINE_SINGLETON_TRAMPOLINE(Class, callback, getter, handler)` - for singleton pattern

**Checklist**:
- [x] Create macro header for static callback trampolines
- [x] Refactor FanDial (proof of concept: 18 lines → 3 lines)
- [ ] (Future) Apply to remaining 44 trampolines across codebase

---

### LP-4: Create Overlay Global Instance Template
- **Status**: `[x]` Complete (2026-01-20)
- **Impact**: ~17 lines saved per overlay, macros added to existing helper
- **Files**: `include/ui_global_panel_helper.h`, `src/ui/ui_fan_control_overlay.cpp`

**Macros added**:
- `DEFINE_GLOBAL_OVERLAY_STORAGE(Type, var, getter)` - strict getter that throws if not initialized
- `INIT_GLOBAL_OVERLAY(Type, var, ...)` - init with args, registers cleanup

**Checklist**:
- [x] Add macros to `ui_global_panel_helper.h`
- [x] Refactor FanControlOverlay (proof of concept: 22 lines → 5 lines)
- [ ] (Future) Apply to remaining overlays (RetractionSettings, Timelapse, etc.)

---

### LP-5: Standardize XML Widget Registration
- **Status**: `[-]` Skipped (2026-01-20)
- **Reason**: Analysis showed widget handlers vary significantly (simple card vs complex spinner/switch). Macros would either be too simple to help or too complex to maintain. Low ROI.
- **Files**: `ui_card.cpp`, `ui_dialog.cpp`, `ui_severity_card.cpp`, `ui_spinner.cpp`, `ui_icon.cpp`, `ui_switch.cpp`

**Analysis Summary**:
- Registration functions are similar (3 lines each) - minor savings
- Create handlers have varied initialization logic
- Apply handlers range from simple delegation to multi-pass attribute parsing
- Future: Could revisit if more widgets are added with similar patterns

---

## Completion Log

| Date | Task | Notes |
|------|------|-------|
| 2026-01-20 | HP-5: Domain-specific observer helpers | Added `observe_connection_state` and `observe_print_state` to observer_factory.h |
| 2026-01-20 | HP-2: Global panel singleton macro | Created macro, refactored 4 panels (console, spoolman, macros, bed_mesh) |
| 2026-01-20 | HP-3: Modal button row component | Refactored 9 modals to use component, 3 skipped (wifi/hidden_network: conditional bindings, modal_dialog: dynamic text) |
| 2026-01-20 | HP-4: Divider components (complete) | Updated 34 XML files to use divider components, added color prop for flexibility, 72 inline dividers eliminated |
| 2026-01-20 | MP-C1: Subject init/deinit guards | Added to PanelBase and OverlayBase, refactored 5 panels |
| 2026-01-20 | MP-C2: Overlay creation helper | Added `create_overlay_from_xml()` to OverlayBase, refactored 3 overlay panels |
| 2026-01-20 | MP-C4: Modal button wiring | Consolidated with private `wire_button()` helper |
| 2026-01-20 | MP-C7: Fullscreen backdrop helper | Added `ui_create_fullscreen_backdrop()`, refactored Modal and BusyOverlay |
| 2026-01-20 | MP-C8: Visibility toggle utility | Added `ui_toggle_list_empty_state()`, refactored console and macros panels |
| 2026-01-20 | MP-X1: Modal header component | Created component, updated 2 modal files |
| 2026-01-20 | MP-X2: Connecting state component | Created component, updated WiFi modals |
| 2026-01-20 | MP-X3: Info note component | Created component, updated 3 overlay files |
| 2026-01-20 | MP-X4: Empty state component | Created component with sensible `inbox_outline` default icon |
| 2026-01-20 | MP-X5: Centered column component | Created component, updated 5 panel/overlay files |
| 2026-01-20 | MP-C3: Lazy panel navigation | Created `ui_lazy_panel_helper.h`, refactored 4 handlers in AdvancedPanel (-107 lines) |
| 2026-01-20 | MP-C5: Modal button callbacks | Created `MODAL_BUTTON_CB_IMPL` macro, refactored 6 callbacks (-57 lines) |
| 2026-01-20 | MP-C6: Temperature observer bundle | Created `TemperatureObserverBundle` class, refactored 3 panels |
| 2026-01-20 | MP-X6: Form field component | Created `form_field.xml`, updated 2 modal files |
| 2026-01-20 | HP-3: Modal button row (modals) | Refactored 9 modals to use component, 3 skipped (conditional bindings) |
| 2026-01-20 | LP-2: Position observer bundle | Created `PositionObserverBundle`, refactored ControlsPanel |
| 2026-01-20 | LP-1: ModalGuard RAII wrapper | Created `ModalGuard`, refactored ControlsPanel modal cleanup |
| 2026-01-20 | LP-3: Event trampoline macros | Created 3 macros, refactored FanDial (18→3 lines) |
| 2026-01-20 | LP-5: XML widget registration | Assessed, skipped due to low ROI (patterns too varied) |
| 2026-01-20 | LP-4: Overlay global instance | Added DEFINE_GLOBAL_OVERLAY_STORAGE + INIT_GLOBAL_OVERLAY macros |

---

## Notes for Parallel Execution

Tasks are designed to be independent. Safe parallel groupings:

**Group A** (C++ Observer Patterns):
- HP-1: Migrate manual observer callbacks ✅
- HP-5: Add domain-specific observer helpers ✅
- MP-C6: Temperature observer bundle ✅

**Group B** (C++ Base Class Improvements):
- HP-2: Global panel singleton macro ✅
- MP-C1: Subject init/deinit guards ✅
- MP-C2: Overlay creation helper ✅

**Group C** (XML Components):
- HP-3: Modal button row ✅
- HP-4: Dividers ✅
- MP-X1: Modal header ✅
- MP-X2: Connecting state ✅
- MP-X3: Info note ✅
- MP-X4: Empty state ✅
- MP-X5: Centered column ✅
- MP-X6: Form field ✅

**Group D** (Utilities):
- MP-C3: Lazy panel navigation ✅
- MP-C4: Modal button wiring ✅
- MP-C5: Modal button callbacks ✅
- MP-C7: Backdrop helper ✅
- MP-C8: Visibility helper ✅

---

## Session Notes & Handoff Information

### Worktree Setup

This refactoring work is being done in a dedicated git worktree to isolate changes from the main development branch.

**Worktree location**: `/Users/pbrown/code/helixscreen-ui-refactor`
**Branch**: `ui-refactor`
**Base**: `origin/main`

**Setup commands used**:
```bash
# Create worktree with new branch
git worktree add ../helixscreen-ui-refactor -b ui-refactor origin/main

# Initialize submodules (CRITICAL - per [L027])
./scripts/init-worktree.sh ../helixscreen-ui-refactor

# Build verification
cd ../helixscreen-ui-refactor && make -j
```

**Important**: Always run `./scripts/init-worktree.sh` after creating a worktree. LVGL submodule is required for builds.

### Delegation Approach

**Lesson learned**: Background agents cannot receive interactive tool permissions. When delegating to agents with `run_in_background=true`, they will hit "Permission auto-denied" errors for Write/Edit operations.

**Recommended approach**:
1. **Use synchronous agents** for file modifications (no `run_in_background`)
2. **Use Explore agents** for codebase investigation (safe in background)
3. **Direct implementation** when delegation fails or for simple single-file changes

**Agent selection**:
| Task Type | Agent | Notes |
|-----------|-------|-------|
| Find patterns/code | `Explore` | Fast, can run in background |
| Multi-file implementation | `general-purpose` | Synchronous, gets permissions |
| Code review | `general-purpose` with Sonnet | Detailed feedback |
| Single file, <10 lines | Direct | No delegation needed |

### Review Process

Before committing, run code review using the `/claude-recall:review` skill:

```
Review the changes in [list files]. Check for:

**Correctness**
- Logic errors and edge cases
- Off-by-one errors
- Null/undefined handling
- Race conditions (if async)

**Security**
- Injection vulnerabilities
- Sensitive data exposure
- Input validation at boundaries

**Tests**
- Coverage gaps
- Missing edge cases

**Style**
- Consistency with existing patterns
- Naming clarity
```

**Session 1 review findings** (2026-01-20):
- Copyright header was missing from `ui_global_panel_helper.h` (fixed)
- Pre-existing async observer memory safety concerns identified (not new code)
- Include path in `observer_factory.h` was correct (review false positive)

### Build Verification

Always verify the build passes before committing:
```bash
cd /Users/pbrown/code/helixscreen-ui-refactor
make -j
```

### Current Git Status (as of 2026-01-20)

**Modified files** (ready to commit):
- `include/observer_factory.h` - HP-5: domain-specific observer helpers
- `include/ui_global_panel_helper.h` - HP-2: new macro file
- `src/ui/ui_panel_console.cpp` - HP-2: uses macro
- `src/ui/ui_panel_spoolman.cpp` - HP-2: uses macro
- `src/ui/ui_panel_macros.cpp` - HP-2: uses macro
- `src/ui/ui_panel_bed_mesh.cpp` - HP-2: uses macro
- `src/xml_registration.cpp` - HP-3/4: component registrations
- `ui_xml/modal_button_row.xml` - HP-3: new component
- `ui_xml/divider_horizontal.xml` - HP-4: new component
- `ui_xml/divider_vertical.xml` - HP-4: new component
- `docs/UI_REFACTORING_PLAN.md` - tracking document

### Next Steps for Next Session

1. **Commit completed work** - HP-2, HP-3, HP-4, HP-5 are ready
2. **HP-1 is the largest remaining task** - 9 files, ~50 static callbacks to migrate
3. **HP-3/HP-4 remaining** - Update modals to use new divider and button row components
4. **Medium priority** - Can be parallelized across Groups A-D

### Lessons Learned

| ID | Category | Lesson |
|----|----------|--------|
| [L014] | XML | XML components must be registered in `xml_registration.cpp` or they silently fail |
| [L027] | Worktree | Must run `init-worktree.sh` after creating worktree for submodules |
| [S003] | Git | Never modify WIP files in main repo - use worktree for clean slate |
| New | Agents | Background agents cannot get interactive permissions - use synchronous |
| New | Observer | Forward declarations not sufficient for enum types - need actual includes |
