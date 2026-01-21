# Overlay Lifecycle Refactoring Plan

**Created:** 2026-01-21
**Status:** Phase 1 Complete
**Scope:** Refactor legacy overlays to inherit from `OverlayBase` for proper lifecycle management

## Background

The `NavigationManager` provides lifecycle hooks (`on_activate`/`on_deactivate`) for overlays, but this requires overlays to:
1. Inherit from `OverlayBase` (which implements `IPanelLifecycle`)
2. Register with `NavigationManager::register_overlay_instance()` before pushing

Several legacy overlays predate this system and emit warnings:
```
[warning] [NavigationManager] Overlay 0x... pushed without lifecycle registration
```

## Scope

### Part 1: Settings Overlays (6 files)

These overlays have their own `show()` method but don't inherit from `OverlayBase`:

| File | Class | Complexity | Notes |
|------|-------|------------|-------|
| `ui_settings_machine_limits.cpp` | `MachineLimitsOverlay` | MEDIUM | SubjectManager, API async |
| `ui_settings_filament_sensors.cpp` | `FilamentSensorSettingsOverlay` | MEDIUM | Dynamic rows, no subjects |
| `ui_settings_display.cpp` | `DisplaySettingsOverlay` | MEDIUM | Nested overlays, 1 subject |
| `ui_print_tune_overlay.cpp` | `PrintTuneOverlay` | COMPLEX | Free-function callbacks, modal |
| `ui_settings_macro_buttons.cpp` | `MacroButtonsOverlay` | MEDIUM | 10 callbacks, empty SubjectManager |
| `ui_settings_hardware_health.cpp` | `HardwareHealthOverlay` | COMPLEX | Dynamic rows, lambdas, modal |

**All already have:**
- Singleton pattern with `StaticPanelRegistry` cleanup
- Two-phase init (`init_subjects()` → `register_callbacks()`)
- `create(parent)` and `show(parent_screen)` methods

**Need to add:**
- `OverlayBase` inheritance
- `on_activate()` override (for data refresh)
- `on_deactivate()` override (for cleanup if needed)
- `register_overlay_instance()` call before push

### Part 2: Temperature Panels (HIGH complexity)

The nozzle/bed temperature panels are raw XML widgets without lifecycle classes:

**Current architecture:**
- `TempControlPanel` (1,386 lines) manages both heaters
- Consumers (`HomePanel`, `ControlsPanel`, `PrintStatusPanel`) create raw XML widgets
- `TempControlPanel::setup_*_panel()` populates them
- No lifecycle registration possible

**Architectural decision needed:**
1. **Option A:** Create separate `NozzleTemperaturePanel` / `BedTemperaturePanel` classes
   - Each extracts ~500 lines from `TempControlPanel`
   - Each manages its own subjects, graphs, observers
   - Estimate: 12-15 hours, HIGH risk

2. **Option B:** Keep `TempControlPanel` as coordinator, make it implement `IPanelLifecycle`
   - Single registration point for both panels
   - Less code duplication
   - Estimate: 4-6 hours, MEDIUM risk

3. **Option C:** Accept warnings for temp panels (defer refactoring)
   - Temp panels work correctly without lifecycle hooks
   - Focus effort on settings overlays first
   - Revisit when TempControlPanel needs other changes

---

## Implementation Plan

### Phase 1: Settings Overlays ✅ COMPLETE

All 6 overlays refactored in parallel. Key fixes from code review:
- PrintTuneOverlay: Added `overlay_root_ = tune_panel_` for base class sync
- Added missing `override` specifiers on destructors (3 files)

#### 1.1 MachineLimitsOverlay
- Already has SubjectManager, closest to OverlayBase pattern
- Add inheritance, rename `overlay_` → `overlay_root_`
- Implement `on_activate()` to trigger `query_and_show()`
- Use `init_subjects_guarded()` helper

#### 1.2 MacroButtonsOverlay
- Similar structure, empty SubjectManager
- Can remove SubjectManager or keep for consistency
- `on_activate()` triggers `populate_dropdowns()`

#### 1.3 FilamentSensorSettingsOverlay
- No subjects needed, simpler refactor
- Dynamic row creation compatible with OverlayBase
- `on_activate()` triggers `populate_sensor_list()`

#### 1.4 DisplaySettingsOverlay
- Single manual subject (brightness)
- Convert to SubjectManager or keep manual with `deinit_subjects_base()` pattern
- `on_activate()` triggers dropdown initialization

#### 1.5 PrintTuneOverlay (COMPLEX)
- Requires splitting `show()` into proper two-phase init
- Convert free-function callbacks to static class methods
- `on_activate()` triggers `sync_sliders_to_state()`
- Modal (SaveZOffsetModal) remains as composition

#### 1.6 HardwareHealthOverlay (COMPLEX)
- Dynamic rows with lambdas need careful lifetime management
- `on_activate()` triggers `populate_hardware_issues()`
- Modal dialog cleanup on `on_deactivate()`

### Phase 2: Temperature Panels (Deferred)

**Recommendation:** Start with Option C (defer), revisit after Phase 1 complete.

The temperature panels work correctly today - the warning is cosmetic. The refactoring is architectural and high-risk. Better to:
1. Complete settings overlay refactoring
2. Gain confidence with OverlayBase patterns
3. Then revisit temp panel architecture with fresh perspective

If Phase 2 proceeds, key questions to answer first:
- Can graph registration become panel-local? (Simplifies split)
- Should subjects move to panel classes? (PrinterState already publishes temps)
- Can XML creation move to panels? (Consumers shouldn't own XML)

---

## Refactoring Template

For each settings overlay:

```cpp
// header changes
#include "overlay_base.h"

class MyOverlay : public OverlayBase {  // ADD inheritance
public:
    // KEEP existing methods, ADD overrides:
    const char* get_name() const override { return "My Overlay"; }
    void on_activate() override;
    void on_deactivate() override;

    // CHANGE: create() returns overlay_root_ (renamed from overlay_)
    lv_obj_t* create(lv_obj_t* parent) override;
};

// implementation changes
void MyOverlay::show(lv_obj_t* parent_screen) {
    // ... existing setup ...

    // ADD before push:
    NavigationManager::instance().register_overlay_instance(overlay_root_, this);

    ui_nav_push_overlay(overlay_root_);
}

void MyOverlay::on_activate() {
    OverlayBase::on_activate();  // sets visible_ = true
    // Move data refresh here (was in show())
    refresh_data();
}

void MyOverlay::on_deactivate() {
    OverlayBase::on_deactivate();  // sets visible_ = false
    // Optional: cancel pending operations
}
```

---

## Success Criteria

- [x] No "pushed without lifecycle registration" warnings for settings overlays
- [x] All settings overlays properly receive `on_activate()`/`on_deactivate()` calls
- [x] No regressions in overlay functionality
- [x] Build passes, existing tests pass
- [x] Temperature panel warnings documented as known/deferred (Option C selected)

---

## Open Questions

1. Should we create a code generator/script for the mechanical parts of OverlayBase conversion?
2. Should `on_deactivate()` be used to persist unsaved changes (e.g., machine limits)?
3. For PrintTuneOverlay: should SaveZOffsetModal also inherit from OverlayBase?

---

## References

- `include/overlay_base.h` - Base class documentation
- `src/ui/ui_overlay_network_settings.cpp` - Reference implementation
- `src/ui/ui_ams_settings_overlay.cpp` - Recently fixed example
