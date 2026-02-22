# Z-Offset Extraction Design

> **Created**: 2026-02-21
> **Status**: Approved
> **Refactor Plan Section**: 2.5

## Problem

Z-offset save/apply logic is duplicated across 3 files with slight variations:

| File | Lines | Issue |
|------|-------|-------|
| `ui_panel_controls.cpp` | 781-897 | Full strategy-aware save with confirmation dialog |
| `ui_print_tune_overlay.cpp` | 510-541 | Bare `SAVE_CONFIG` — **skips `Z_OFFSET_APPLY_*`** (bug) |
| `ui_panel_calibration_zoffset.cpp` | 591-709 | Deeply nested callback pyramid for ACCEPT → apply → save |

The controls panel (1,789 lines) also has ~160 lines of Z-offset display/formatting code that belongs in shared utilities.

## Approach

**Approach A: Shared Utils + Controls Extraction** — Create `ZOffsetUtils` for shared save/apply logic, move formatting helpers there, slim controls panel. Leave calibration and tune overlays as standalone files (already well-structured).

## Design

### New Files

**`include/z_offset_utils.h`** + **`src/ui/z_offset_utils.cpp`**
Namespace: `helix::zoffset`

```cpp
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
///   GCODE_OFFSET → no-op (auto-persisted, should not be called)
///
/// on_success: called after SAVE_CONFIG succeeds (Klipper will restart)
/// on_error: called with user-facing message on any failure
void apply_and_save(
    MoonrakerAPI* api,
    ZOffsetCalibrationStrategy strategy,
    std::function<void()> on_success,
    std::function<void(const std::string& error)> on_error
);

} // namespace helix::zoffset
```

### Changes to Existing Files

**`ui_panel_controls.cpp`** (~100 lines removed):
- `update_z_offset_delta_display()` → calls `helix::zoffset::format_delta()`
- `update_controls_z_offset_display()` → calls `helix::zoffset::format_offset()`
- `handle_save_z_offset()` → calls `is_auto_saved()` for early return, rest stays (confirmation dialog)
- `handle_save_z_offset_confirm()` → replaces 50-line nested callback chain with `apply_and_save()` call
- `handle_zoffset_tune()` stays (3 lines)
- `handle_save_z_offset_cancel()` stays (1 line)

**`ui_print_tune_overlay.cpp`** (bug fix + cleanup):
- `handle_save_z_offset()` → calls `is_auto_saved()` + `apply_and_save()` instead of bare `SAVE_CONFIG`
- **Fixes bug**: currently skips `Z_OFFSET_APPLY_PROBE`/`Z_OFFSET_APPLY_ENDSTOP` before save

**`ui_panel_calibration_zoffset.cpp`** (simplify callback pyramid):
- `send_accept()` → after ACCEPT succeeds, calls `apply_and_save()` for probe/endstop strategies
- Replaces ~80 lines of nested callbacks with ~10 lines

### Testing

- Unit tests for `format_offset()`, `format_delta()`, `is_auto_saved()`
- Characterization tests for `apply_and_save()` with mock API (all 3 strategies)
- Regression test: tune overlay save now executes `Z_OFFSET_APPLY_*` before `SAVE_CONFIG`

### Metrics

| Metric | Before | After |
|--------|--------|-------|
| Duplicated save logic | 3 copies | 1 (`apply_and_save`) |
| Controls panel Z-offset lines | ~160 | ~60 |
| Tune overlay save bug | Skips apply step | Fixed |
| Calibration callback nesting | 5 levels deep | 2 levels |
| New shared utility lines | 0 | ~120 |
| Net lines removed | - | ~100 |
