# Standard Macros System - Session Handoff

**Copy the prompt below into a fresh Claude Code session to continue implementation.**

---

## Progress Summary

| Stage | Description | Status | Commit |
|-------|-------------|--------|--------|
| 1 | StandardMacros Core Class | ✅ Complete | `5888124d` |
| 2 | Hardware Discovery Integration | ✅ Complete | `5888124d` |
| 3 | Settings Overlay UI | ✅ Complete | `96cc536b` |
| 4 | Settings Panel Handlers | ✅ Complete | `96cc536b` |
| 5 | Controls Panel Integration | ✅ Complete | `96cc536b` |
| 6 | Filament Panel Integration | ✅ Complete | `6214ff88` + `4840ebb8` |
| 7 | Print Status Panel Integration | ✅ Complete | — |
| 8 | Testing & Polish | 🔲 Next | — |

---

## What Was Built (Stages 1-7)

### Files Created
- `include/standard_macros.h` - Header with `StandardMacros` singleton, `StandardMacroSlot` enum, `StandardMacroInfo` struct
- `src/standard_macros.cpp` - Full implementation
- `ui_xml/macro_buttons_overlay.xml` - Settings overlay with Quick Buttons + Standard Macros dropdowns

### Files Modified
- `src/application/application.cpp` - Added `StandardMacros::instance().init()` in `on_hardware_discovered` callback
- `src/xml_registration.cpp` - Registered `macro_buttons_overlay.xml` component
- `ui_xml/settings_panel.xml` - Added "Macro Buttons" action row in PRINTER section
- `include/ui_panel_settings.h` - Added `macro_buttons_overlay_`, handler declarations
- `src/ui_panel_settings.cpp` - Added overlay creation, dropdown population, 11 change handlers
- `src/moonraker_api_advanced.cpp` - Implemented `execute_macro()` (was stub)
- `include/ui_panel_controls.h` - Added StandardMacros integration, `refresh_macro_buttons()`
- `src/ui_panel_controls.cpp` - Quick buttons now use StandardMacros slots instead of hardcoded G-code
- `src/ui_panel_print_status.cpp` - Pause/Resume/Cancel use StandardMacros, buttons disabled if slot empty

### Key Implementation Details

**9 Semantic Slots:**
```cpp
enum class StandardMacroSlot {
    LoadFilament, UnloadFilament, Purge,
    Pause, Resume, Cancel,
    BedLevel, CleanNozzle, HeatSoak
};
```

**Priority Resolution:** `configured_macro` → `detected_macro` → `fallback_macro`

**Auto-Detection Patterns:**
| Slot | Patterns |
|------|----------|
| LoadFilament | LOAD_FILAMENT, M701 |
| UnloadFilament | UNLOAD_FILAMENT, M702 |
| Purge | PURGE, PURGE_LINE, PRIME_LINE |
| Pause | PAUSE, M601 |
| Resume | RESUME, M602 |
| Cancel | CANCEL_PRINT |
| BedLevel | BED_MESH_CALIBRATE, Z_TILT_ADJUST, QUAD_GANTRY_LEVEL, QGL |
| CleanNozzle | CLEAN_NOZZLE, NOZZLE_WIPE, WIPE_NOZZLE |
| HeatSoak | HEAT_SOAK, CHAMBER_SOAK, SOAK |

**HELIX Fallbacks:** Only `BedLevel` (HELIX_BED_LEVEL_IF_NEEDED) and `CleanNozzle` (HELIX_CLEAN_NOZZLE) have fallbacks.

**Config Path:** `/standard_macros/<slot_name>` (e.g., `/standard_macros/load_filament`)

### Issues Resolved During Implementation
1. **Missing `#include <optional>`** in header - Fixed
2. **Fallback macros not restored on re-init** - Fixed by restoring from static table at start of `init()`
3. **`class` vs `struct` forward declaration mismatch** for MoonrakerError - Fixed

### Stage 5 Implementation Details
- Implemented `MoonrakerAPI::execute_macro()` (was a stub)
- Quick buttons read slot names from config: `/standard_macros/quick_button_1`, `/standard_macros/quick_button_2`
- `refresh_macro_buttons()` updates labels and hides empty slots
- `on_activate()` refreshes buttons after StandardMacros initialization

### Stage 6 Implementation Details
- Replaced old `Config::get_macro()` system with `StandardMacros::instance().execute()`
- `execute_load()` uses `StandardMacroSlot::LoadFilament` - shows warning if slot empty
- `execute_unload()` uses `StandardMacroSlot::UnloadFilament` - shows warning if slot empty
- `handle_purge_button()` uses `StandardMacroSlot::Purge` with fallback to inline G-code (`M83\nG1 E{amount} F300`)
- Removed unused `load_filament_gcode_` and `unload_filament_gcode_` member variables

### Stage 7 Implementation Details
- Added `#include "standard_macros.h"` to print status panel
- `handle_pause_button()`:
  - Checks `StandardMacroSlot::Pause` availability when in Printing state
  - Checks `StandardMacroSlot::Resume` availability when in Paused state
  - Uses `StandardMacros::instance().execute()` for both pause and resume
  - Shows warning notification if slot is empty
- `handle_cancel_button()`:
  - Checks `StandardMacroSlot::Cancel` availability before showing confirmation dialog
  - Uses `StandardMacros::instance().execute()` instead of `api_->cancel_print()`
- `show_runout_guidance_modal()`:
  - Resume callback uses `StandardMacroSlot::Resume` via StandardMacros
  - Cancel callback uses `StandardMacroSlot::Cancel` via StandardMacros
- `update_button_states()`:
  - Pause button disabled if Pause slot empty (when printing) or Resume slot empty (when paused)
  - Cancel button disabled if Cancel slot empty
  - Enhanced debug logging shows individual button states

---

## Handoff Prompt for Stage 8

```
I'm continuing the "Standard Macros" system for HelixScreen. Stages 1-7 are complete.

## Quick Context

HelixScreen is a C++/LVGL touchscreen UI for 3D printers. The StandardMacros system maps semantic operations (Load Filament, Pause, Clean Nozzle, etc.) to printer-specific G-code macros.

## What's Already Done

- `include/standard_macros.h` - StandardMacros singleton with 9 slots
- `src/standard_macros.cpp` - Full implementation with auto-detection, config, execute()
- `src/moonraker_api_advanced.cpp` - `execute_macro()` implemented
- Integration in `src/application/application.cpp` - init() called on hardware discovery
- `ui_xml/macro_buttons_overlay.xml` - UI overlay with Quick Buttons + Standard Macros dropdowns
- `ui_xml/settings_panel.xml` - "Macro Buttons" action row added
- `src/xml_registration.cpp` - Overlay component registered
- `src/ui_panel_settings.cpp` - Settings overlay handlers wired up with dropdown population
- `src/ui_panel_controls.cpp` - Quick buttons integrated with StandardMacros
- `src/ui_panel_filament.cpp` - Load/Unload/Purge integrated with StandardMacros

## Read First

1. Read the spec: docs/STANDARD_MACROS_SPEC.md (especially Panel Integration section)
2. Read current print status panel: src/ui_panel_print_status.cpp
3. Look for existing PAUSE/RESUME/CANCEL handling

## Current Stage: 7 - Print Status Panel Integration

Task: Integrate Print Status Panel with StandardMacros

Implementation:
1. Find existing pause/resume/cancel calls in print status panel
2. Replace hardcoded calls with `StandardMacros::instance().execute()`
3. Disable (grey out) buttons if slot is empty - don't hide

Example pattern from spec:
```cpp
void PrintStatusPanel::handle_pause() {
    if (!StandardMacros::instance().execute(
            StandardMacroSlot::Pause, api_,
            []() { NOTIFY_SUCCESS("Pausing print..."); },
            [](auto& err) { NOTIFY_ERROR("Pause failed: {}", err.user_message()); })) {
        NOTIFY_WARNING("Pause macro not configured");
    }
}
```

## Workflow

1. Read print status panel to understand current pause/resume/cancel handling
2. Add StandardMacros integration
3. Run `make -j` to verify
4. Test with `./build/bin/helix-screen --test -p print-status -vv`
5. Update docs/STANDARD_MACROS_HANDOFF.md
6. Commit when working

Begin with Stage 7.
```

---

## Stage Prompts (Stages 7-8)

### Stage 4: Settings Panel Handlers ✅ COMPLETE
Settings panel handlers are fully implemented:
- `handle_macro_buttons_clicked()` creates overlay lazily
- 11 dropdown change handlers registered (2 quick buttons + 9 slots)
- Dropdowns populate with "(Auto: X)" / "(Empty)" + sorted printer macros
- Config saved via `StandardMacros::set_macro()` and direct config writes

### Stage 5: Controls Panel Integration ✅ COMPLETE
Controls panel quick buttons now use StandardMacros:
- Implemented `MoonrakerAPI::execute_macro()` (builds G-code from name + params)
- Quick buttons read slot assignment from config (`/standard_macros/quick_button_1`, etc.)
- `refresh_macro_buttons()` updates labels from `StandardMacroInfo::display_name`
- Empty slots hide the button via `LV_OBJ_FLAG_HIDDEN`
- `on_activate()` refreshes after StandardMacros initialization

### Stage 6: Filament Panel Integration ✅ COMPLETE
Filament panel now uses StandardMacros for load/unload/purge:
- Replaced old `Config::get_macro()` system with `StandardMacros::instance().execute()`
- `execute_load()` uses `StandardMacroSlot::LoadFilament` - shows warning if empty
- `execute_unload()` uses `StandardMacroSlot::UnloadFilament` - shows warning if empty
- `handle_purge_button()` uses `StandardMacroSlot::Purge` with inline G-code fallback
- Removed unused `load_filament_gcode_` and `unload_filament_gcode_` member variables

### Stage 7: Print Status Panel Integration ✅ COMPLETE
Print status panel now uses StandardMacros for pause/resume/cancel:
- `handle_pause_button()` uses `StandardMacroSlot::Pause` or `StandardMacroSlot::Resume` based on state
- `handle_cancel_button()` uses `StandardMacroSlot::Cancel` via StandardMacros
- `show_runout_guidance_modal()` callbacks use StandardMacros for resume/cancel
- `update_button_states()` disables buttons if corresponding slot is empty
- Buttons are greyed out (not hidden) when slots are empty

### Stage 8: Testing & Polish
```
Final stage of Standard Macros system.

Task: Testing & Polish
1. Run full test suite: make test-run
2. Test with --test flag (mock printer):
   - Verify auto-detection logs show discovered macros
   - Verify config persistence (set a macro, restart, verify it's restored)
   - Verify empty slot handling (buttons hidden/disabled appropriately)
3. Test UI flow:
   - Settings → Macro Buttons → Change a dropdown → Verify saved
   - Controls panel quick buttons update after config change
4. Consider adding unit tests for StandardMacros if time permits

When all tests pass, we're done!
```

---

## Reference Files

| Purpose | File |
|---------|------|
| Overlay pattern | `ui_xml/display_settings_overlay.xml` |
| Setting components | `ui_xml/setting_toggle_row.xml`, `setting_dropdown_row.xml` |
| XML registration | `src/xml_registration.cpp` |
| Settings panel | `ui_xml/settings_panel.xml`, `src/ui_panel_settings.cpp` |
| Macro detection | `include/printer_capabilities.h` |
| HELIX macros | `src/helix_macro_manager.cpp` |
| Config access | `include/config.h` |

---

## Notes

- Each stage should be a single commit with conventional commit format
- Run `make -j` after each stage to verify compilation
- Run agentic code review before committing
- The SPEC file (`docs/STANDARD_MACROS_SPEC.md`) is the source of truth for design decisions
- Use the approaches system to track multi-step work
