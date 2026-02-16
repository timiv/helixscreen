# Error State Visualization Design

**Date**: 2026-02-15
**Status**: Implemented — awaiting smoke test / visual verification
**Branch**: feature/multi-unit-ams

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Visualize per-slot error states and buffer health in the AMS detail and overview views.

**Architecture:** Add `SlotError` and `BufferHealth` to `SlotInfo`, populated per-backend from status data. AFC provides per-lane error states and buffer fault data; Happy Hare provides system-level error mapped to the active gate. UI renders error indicators on slot components (detail) and unit cards (overview).

---

## Data Model

### SlotError (new struct in ams_types.h)

```cpp
struct SlotError {
    std::string message;        // Human-readable: "Lane 1 load failed"
    enum Severity {
        INFO,
        WARNING,
        ERROR
    } severity = ERROR;
};
```

### BufferHealth (new struct in ams_types.h)

```cpp
struct BufferHealth {
    bool fault_detection_enabled = false;
    float distance_to_fault = 0;  // 0 = no fault proximity
    std::string state;            // "Advancing", "Trailing"
};
```

### SlotInfo additions

```cpp
struct SlotInfo {
    // ... existing fields ...
    std::optional<SlotError> error;              // Per-slot error state
    std::optional<BufferHealth> buffer_health;    // AFC buffer health (others leave nullopt)
};
```

### AmsUnit helper

```cpp
[[nodiscard]] bool has_any_error() const {
    return std::any_of(slots.begin(), slots.end(),
        [](const SlotInfo& s) { return s.error.has_value(); });
}
```

---

## Backend Integration

### AFC (ams_backend_afc.cpp)

**Per-lane error state:**
- When `lane.status` == AFCLaneState "Error", set `slot.error` with:
  - `message`: system `message.message` if available, else "Lane error"
  - `severity`: from system `message.type` ("error" → ERROR, "warning" → WARNING)
- Clear `slot.error` when lane status leaves "Error" state

**Buffer health:**
- Subscribe to `AFC_buffer` status objects (new subscription)
- Parse `fault_detection_enabled`, `distance_to_fault`, `state` per buffer
- Map buffer → lane (each buffer knows its lanes) → populate `slot.buffer_health`
- When buffer faults (distance_to_fault triggers), also create a WARNING-level `SlotError`

**Buffer status mapping (existing):**
- Already parsing `buffer_status` per lane into `lane_sensors_`
- Wire this into `BufferHealth.state`

### Happy Hare (ams_backend_happy_hare.cpp)

**System-level error mapped to active gate:**
- When `action` == AmsAction::ERROR, set `slot.error` on `current_slot` with:
  - `message`: `reason_for_pause` if available (from `printer.mmu` status), else `operation_detail`
  - `severity`: always ERROR
- Clear when action transitions back to IDLE

**No buffer health** — HH has encoder-based detection but different model. Leave `buffer_health` as nullopt.

### Mock Backend

- Support setting per-slot errors for testing
- Support buffer health simulation for AFC mock mode

---

## UI Visualization

### Detail View (AMS Panel — per-slot)

**Error indicator on slot component:**
- Slot with `error.has_value()`:
  - Red border/outline for ERROR severity
  - Yellow/amber border for WARNING severity
  - Small error icon overlay (top-right corner of slot swatch)
- Tapping errored slot: show error message in existing slot detail/popup
  - For AFC: action:prompt handles interactive recovery
  - For HH: action:prompt handles recovery via error_dialog_macro

**Buffer health indicator (AFC only):**
- Small health dot below or beside the slot:
  - Green: advancing normally, no fault proximity
  - Yellow: `distance_to_fault` approaching threshold
  - Red: fault detected (also creates SlotError)
  - Hidden: `fault_detection_enabled == false` or no buffer data

### Overview View (AMS Overview Panel — per-unit)

**Unit error badge:**
- When `unit.has_any_error()` returns true:
  - Small red dot or warning triangle on the unit card
  - Positioned top-right of the unit card
- Badge severity follows worst slot error in unit (ERROR > WARNING)

---

## Error Lifecycle

```
Backend detects error → populate slot.error → emit slot update event
    → Detail view: slot component observes, shows error indicator
    → Overview view: unit card checks has_any_error(), shows badge

Backend error clears → clear slot.error → emit slot update event
    → Detail view: error indicator removed
    → Overview view: badge removed if no other slot errors
```

---

## Implementation Tasks

### Task 1: Data model — SlotError, BufferHealth, SlotInfo additions

**Files:**
- Modify: `include/ams_types.h`
- Test: `tests/unit/test_ams_types.cpp` (or new test file)

Add `SlotError`, `BufferHealth` structs and `std::optional` fields to `SlotInfo`.
Add `has_any_error()` to `AmsUnit`.

**Tests:**
- SlotError construction and severity
- BufferHealth defaults
- SlotInfo with/without error
- AmsUnit::has_any_error() with mixed error states

### Task 2: AFC backend — populate slot errors from lane status

**Files:**
- Modify: `src/printer/ams_backend_afc.cpp` (parse_afc_state)
- Modify: `include/ams_backend_afc.h` (if needed)
- Test: `tests/unit/test_ams_backend_afc.cpp`

When parsing lane status, check for "Error" state and populate `slot.error`.
Use system message queue text as the error message.
Clear error when lane exits error state.

**Tests:**
- Lane entering error state → slot.error populated
- Lane exiting error state → slot.error cleared
- Error message from message queue flows to slot.error.message
- Error severity mapping from message type

### Task 3: AFC backend — buffer health subscription and parsing

**Files:**
- Modify: `src/printer/ams_backend_afc.cpp`
- Modify: `include/ams_backend_afc.h`
- Test: `tests/unit/test_ams_backend_afc.cpp`

Subscribe to AFC_buffer status objects.
Parse fault_detection_enabled, distance_to_fault, state.
Map buffer → lanes → populate slot.buffer_health.
Create WARNING SlotError when buffer faults.

**Tests:**
- Buffer health parsing from JSON
- Buffer fault → WARNING SlotError created
- Buffer recovery → SlotError cleared
- Buffer data mapped to correct lanes

### Task 4: Happy Hare backend — populate slot errors from system error

**Files:**
- Modify: `src/printer/ams_backend_happy_hare.cpp` (parse_mmu_state)
- Test: `tests/unit/test_ams_backend_happy_hare.cpp`

When action == ERROR, set slot.error on current_slot.
Parse reason_for_pause for error message text.
Clear when action returns to IDLE.

**Tests:**
- System error → current slot gets error
- Error cleared on IDLE transition
- reason_for_pause used as error message
- Error only on current_slot, not all slots

### Task 5: Detail view — slot error indicator UI

**Files:**
- Modify: `ui_xml/components/ams_slot.xml` (or relevant slot component)
- Modify: `src/ui/overlays/ams_panel.cpp`
- Possibly: `ui_xml/ams_panel.xml`

Add error border/icon overlay to slot component.
Bind to slot error state via subject.
Show error message on slot tap/detail popup.

### Task 6: Detail view — buffer health indicator (AFC)

**Files:**
- Modify: `ui_xml/components/ams_slot.xml` (or relevant)
- Modify: `src/ui/overlays/ams_panel.cpp`

Add buffer health dot indicator.
Color based on fault proximity (green/yellow/red).
Hidden when no buffer data.

### Task 7: Overview view — unit error badge

**Files:**
- Modify: `ui_xml/components/ams_overview_card.xml` (or relevant)
- Modify: `src/ui/overlays/ams_overview_panel.cpp`

Add error badge to unit card.
Show when has_any_error() is true.
Badge severity follows worst slot error.

### Task 8: Mock backend — error simulation support

**Files:**
- Modify: `src/printer/ams_backend_mock.cpp`
- Test: verify in test mode

Support setting per-slot errors via mock config.
Support buffer health simulation.
Useful for `--test` mode development.

---

## Smoke Test / Visual Verification (TODO)

All tasks are code-complete with unit tests (31 tests, 751k assertions passing). Visual verification of the UI indicators has **not** been done yet. Run through these checks before merging:

### Setup
```bash
# AFC multi-unit (shows overview + detail + errors + buffer health):
HELIX_MOCK_AMS_TYPE=afc HELIX_MOCK_AMS_MULTI=1 ./build/bin/helix-screen --test -vv

# AFC single-unit (detail view only):
HELIX_MOCK_AMS_TYPE=afc ./build/bin/helix-screen --test -vv

# Happy Hare (no buffer health, no pre-populated errors):
./build/bin/helix-screen --test -vv
```

### Checklist

- [ ] **Overview: unit error badge** — red dot on unit card with ERROR slot, yellow for WARNING-only
- [ ] **Overview: mini-bar status lines** — red/yellow lines under errored slot bars
- [ ] **Detail: slot error indicator** — 14px red dot (ERROR) or yellow dot (WARNING) at top-right of spool
- [ ] **Detail: buffer health dot** — 8px dot at bottom-center: green (healthy), yellow (approaching fault), red (fault)
- [ ] **Detail: no overlap** — error dot doesn't collide with tool badge (top-left) or status badge (bottom-right)
- [ ] **HH mode: no buffer dots** — buffer health dots hidden (HH has no buffer data)
- [ ] **Navigate away/back** — indicators reappear correctly after panel re-activation

### Mock data in AFC mode
- Lane 1 (ASA, black): loaded, healthy buffer → green health dot, no error
- Lane 2 (PLA, red): available, healthy buffer → green health dot, no error
- Lane 3 (PETG, green): available, approaching fault (12.5mm) → yellow health dot, yellow error indicator
- Lane 4 (TPU, orange): available, ERROR "Lane 4 load failed" → red health dot, red error indicator

---

## Non-Goals

- Custom recovery UI (defer to action:prompt)
- HH encoder/clog visualization (different system, future work)
- Error history/logging (notification history already captures toasts)
- Per-slot error sound/haptic feedback
