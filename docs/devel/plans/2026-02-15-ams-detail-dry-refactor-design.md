# AMS Detail View DRY Refactor Design

**Date**: 2026-02-15
**Status**: Approved
**Branch**: `feature/multi-unit-ams`

## Problem

`AmsPanel` and `AmsOverviewPanel` duplicate ~250 lines of near-identical code for rendering the
spool grid + tray + labels layer + path canvas + "currently loaded" card + bypass/action buttons.

The code was copy-pasted when `AmsOverviewPanel` was created, renamed with `detail_` prefixes,
and has since diverged accidentally:

| Divergence | AmsPanel | AmsOverviewPanel |
|------------|----------|------------------|
| Tray background | `#tray_bg` (#505050) | `#elevated_bg` (lighter) |
| Tray accent color | `#text` | `#text_muted` |
| Tray border tokens | Custom `#tray_*` tokens | Hardcoded values |
| Tray min-height | None (bug) | 20px guard (correct) |
| Weight display | Inline with dot separator | Stacked on separate line |
| Slot sizing | Inlined calculation | Extracted to `calculate_ams_slot_layout()` |

Both views should look visually identical — one shared implementation.

## Solution: Shared XML Component + C++ Helper Functions

Extract shared rendering into:
1. **`ams_unit_detail` XML component** — the spool grid + tray + labels layer structure
2. **`ui_ams_detail.h/cpp`** — free functions for slot creation, tray sizing, label management, path canvas setup
3. **`ams_loaded_card` XML component** — the "Currently Loaded" card with swatch + details
4. **`ams_action_buttons` XML component** — bypass toggle + unload/reset/settings buttons

### Shared XML Component: `ui_xml/components/ams_unit_detail.xml`

Contains the `slots_wrapper` structure that both panels currently duplicate:

```xml
<component>
  <consts>
    <color name="tray_bg" value="#505050"/>
    <color name="tray_border" value="#606060"/>
    <number name="tray_bg_opa" value="200"/>
    <number name="tray_border_opa" value="140"/>
    <px name="tray_default_height" value="30"/>
    <number name="tray_accent_border_opa" value="180"/>
  </consts>
  <view name="ams_unit_detail" extends="lv_obj"
        width="100%" height="content" style_pad_all="0" scrollable="false">
    <!-- Slot grid (rendered first = behind tray) -->
    <lv_obj name="slot_grid"
            width="content" height="content" style_pad_all="0" style_pad_row="0"
            style_pad_column="0" flex_flow="row" style_flex_main_place="start"
            style_flex_cross_place="start" scrollable="false"/>
    <!-- Labels layer (renders on top of overlapping slots) -->
    <lv_obj name="labels_layer"
            width="100%" height="80" style_pad_all="0" scrollable="false"
            clickable="false"/>
    <!-- Tray (renders on top of spools, creates "holder" effect) -->
    <lv_obj name="slot_tray"
            width="100%" height="#tray_default_height" align="bottom_mid"
            style_bg_color="#tray_bg" style_bg_opa="#tray_bg_opa"
            style_border_width="1" style_border_color="#tray_border"
            style_border_opa="#tray_border_opa"
            style_radius="#border_radius" style_pad_all="0"
            scrollable="false" clickable="false">
      <!-- Left accent line -->
      <lv_obj width="#space_xs" height="80%" align="left_mid"
              style_translate_x="#space_xs" style_border_width="#space_xxs"
              style_border_color="#text" style_border_opa="#tray_accent_border_opa"
              style_border_side="left" style_radius="#border_radius"
              style_pad_all="0" scrollable="false" clickable="false"/>
      <!-- Right accent line -->
      <lv_obj width="#space_xs" height="80%" align="right_mid"
              style_translate_x="-2" style_border_width="#space_xxs"
              style_border_color="#text" style_border_opa="#tray_accent_border_opa"
              style_border_side="right" style_radius="#border_radius"
              style_pad_all="0" scrollable="false" clickable="false"/>
    </lv_obj>
  </view>
</component>
```

Both panels embed this via `<ams_unit_detail name="slot_area"/>` and find children
via `lv_obj_find_by_name()` as usual.

**Z-order fix:** Tray renders AFTER slot_grid (on top of spools). Badges inside each
slot use `lv_obj_move_to_index(-1)` to stay on top of the tray. This matches the
correct visual: spools behind tray, badges on top.

### C++ Helper Module: `ui_ams_detail.h` + `src/ui/ui_ams_detail.cpp`

Free functions operating on widget pointers. No class, no state — just utilities.

```cpp
#pragma once

#include "ui_ams_slot_layout.h"
#include "lvgl/lvgl.h"

/// Widget pointers for a single ams_unit_detail instance
struct AmsDetailWidgets {
    lv_obj_t* slot_grid = nullptr;
    lv_obj_t* slot_tray = nullptr;
    lv_obj_t* labels_layer = nullptr;
    lv_obj_t* path_canvas = nullptr;  // nullable — overview detail has it, but optional
};

/// Resolve widget pointers from an ams_unit_detail root object
AmsDetailWidgets ams_detail_find_widgets(lv_obj_t* unit_detail_root);

/// Create slot widgets in the grid.
/// unit_index: -1 for whole backend (AmsPanel), >= 0 for specific unit (AmsOverviewPanel).
/// Returns number of slots created. Populates slot_widgets array.
int ams_detail_create_slots(
    AmsDetailWidgets& w,
    lv_obj_t* slot_widgets[],     // output: array of slot widget pointers
    int max_slots,                 // size of slot_widgets array
    int unit_index,                // -1 = whole backend, >= 0 = specific unit
    lv_event_cb_t click_cb,        // per-panel click handler
    void* click_cb_user_data);     // panel pointer for callback

/// Size tray to 1/3 of slot grid height (minimum 20px).
void ams_detail_update_tray(AmsDetailWidgets& w);

/// Move material labels to the labels layer for 5+ overlapping slots.
void ams_detail_update_labels(
    AmsDetailWidgets& w,
    lv_obj_t* slot_widgets[],
    int slot_count,
    const AmsSlotLayout& layout);

/// Configure path canvas from backend state for a specific unit (or whole system).
/// hub_only: true for overview detail mode (only draw slots → hub).
void ams_detail_setup_path_canvas(
    lv_obj_t* canvas,
    lv_obj_t* slot_grid,
    int unit_index,               // -1 = whole backend
    bool hub_only);
```

### Shared XML Component: `ui_xml/components/ams_loaded_card.xml`

The "Currently Loaded" card — swatch + material name + slot label + weight. Uses stacked
layout (weight on separate line), bound to existing AmsState subjects:

- `ams_current_material_text`
- `ams_current_slot_text`
- `ams_current_weight_text`
- `ams_current_has_weight`

Both panels embed via `<ams_loaded_card/>`.

### Shared XML Component: `ui_xml/components/ams_action_buttons.xml`

Contains bypass toggle row + unload/reset/settings buttons. Uses existing AmsState subjects
(`ams_supports_bypass`, `ams_bypass_active`, `ams_filament_loaded`). Callback names use
generic names that each panel registers:

- `on_ams_bypass_toggled` (shared — already works this way)
- `on_ams_unload_clicked` (panel-specific registration)
- `on_ams_reset_clicked` (panel-specific registration)
- `on_ams_settings_clicked` (panel-specific registration)

### What Stays Panel-Specific

**AmsPanel only:**
- System header (logo + backend name)
- Backend selector (for multi-backend)
- Endless spool arrows
- Status label + progress stepper
- Dryer card

**AmsOverviewPanel only:**
- Unit cards row (overview mode)
- System path area (overview mode)
- Detail header with back button
- Hub-only path canvas mode
- Unit-scoped slot indexing (handled by `unit_index` parameter)

## Existing Code Reuse

`ui_ams_slot_layout.h` already exists with `calculate_ams_slot_layout()` — the new helpers
use it. `ui_ams_slot.cpp` functions (`ui_ams_slot_set_index`, `ui_ams_slot_move_label_to_layer`)
are already shared between both panels.

## Files Changed

| File | Change |
|------|--------|
| New: `ui_xml/components/ams_unit_detail.xml` | Shared spool grid + tray + labels component |
| New: `ui_xml/components/ams_loaded_card.xml` | Shared "Currently Loaded" card |
| New: `ui_xml/components/ams_action_buttons.xml` | Shared bypass + action buttons |
| New: `include/ui_ams_detail.h` | Helper function declarations |
| New: `src/ui/ui_ams_detail.cpp` | Helper function implementations |
| Modify: `ui_xml/ams_panel.xml` | Replace inline slot grid/tray/labels/buttons with shared components |
| Modify: `ui_xml/ams_overview_panel.xml` | Replace inline detail section with shared components |
| Modify: `src/ui/ui_panel_ams.cpp` | Replace `create_slots()`, `update_tray_size()` with helper calls |
| Modify: `src/ui/ui_panel_ams_overview.cpp` | Replace `create_detail_slots()`, tray/labels code with helper calls |
| Modify: `src/main.cpp` | Register new XML components |
| Delete: tray consts from `ams_panel.xml` | Moved to shared component |

## Estimated Impact

- ~250 lines of duplication eliminated
- ~120 lines of new shared code (helpers + components)
- Net reduction: ~130 lines
- Both views guaranteed visually identical
- Bug fixes (tray min-height) applied everywhere automatically
