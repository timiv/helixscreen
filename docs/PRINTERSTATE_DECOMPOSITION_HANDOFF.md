# PrinterState God Class Decomposition - Handoff Document

**Created**: 2026-01-11
**Last Updated**: 2026-01-12
**Status**: IN PROGRESS - 7 domains extracted (Temperature, Motion, LED, Fan, Print, Capabilities, Plugin Status)

## Quick Resume

```bash
# 1. Go to the worktree
cd /Users/pbrown/Code/Printing/helixscreen-printer-state-decomp

# 2. Verify worktree is up to date
git fetch origin && git status

# 3. Build
make -j && make test-build

# 4. Run characterization tests (should all pass - 143 tests, 1173 assertions)
./build/bin/helix-tests "[characterization]"

# 5. Continue with next domain extraction (see Remaining Domains section)
```

### Recommended Next Domain: **Composite Visibility** (5 subjects)
Derived visibility flags: `can_show_bed_mesh_`, `can_show_qgl_`, `can_show_z_tilt_`, `can_show_nozzle_clean_`, `can_show_purge_line_`. These combine `helix_plugin_installed` with printer capabilities.

---

## Project Overview

### Goal
Decompose the 2808-line `PrinterState` god class (86 subjects across 11+ domains) into focused, testable domain state classes.

### Source Reference
- **REFACTOR_PLAN.md**: `docs/REFACTOR_PLAN.md` section 1.1 "PrinterState God Class Decomposition"
- **Branch**: `feature/printer-state-decomposition`
- **Worktree**: `/Users/pbrown/Code/Printing/helixscreen-printer-state-decomp`

---

## Progress Summary

### Completed Extractions

| Domain | Class | Subjects | Tests | Assertions | Commit |
|--------|-------|----------|-------|------------|--------|
| Temperature | `PrinterTemperatureState` | 4 | 26 | 145 | 36dec0bb |
| Motion | `PrinterMotionState` | 8 | 21 | 165 | dfa92d60 |
| LED | `PrinterLedState` | 6 | 18 | 146 | ee5ac704 |
| Fan | `PrinterFanState` | 2 static + dynamic | 26 | 118 | ee5ac704 |
| Print | `PrinterPrintState` | 17 | 26 | 330 | 7dfd653d |
| Capabilities | `PrinterCapabilitiesState` | 14 | 17 | 232 | 0c045f49 |
| Plugin Status | `PrinterPluginStatusState` | 2 | 9 | 37 | TBD |
| **Total** | | **53+** | **143** | **1173** | |

### Next Steps

| Step | Description | Status |
|------|-------------|--------|
| 1 | Write Plugin Status characterization tests | ✅ DONE |
| 2 | Extract PrinterPluginStatusState class | ✅ DONE |
| 3 | Continue with Composite Visibility domain (5 subjects) | ⏳ NEXT |

---

## Extracted Components

### 1. PrinterTemperatureState (4 subjects)
**File**: `include/printer_temperature_state.h`, `src/printer/printer_temperature_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `extruder_temp_` | int | Centidegrees (205.3°C → 2053) |
| `extruder_target_` | int | Centidegrees |
| `bed_temp_` | int | Centidegrees |
| `bed_target_` | int | Centidegrees |

### 2. PrinterMotionState (8 subjects)
**File**: `include/printer_motion_state.h`, `src/printer/printer_motion_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `position_x_`, `position_y_`, `position_z_` | int | mm (truncated from float) |
| `homed_axes_` | string | "xyz", "xy", "" etc |
| `speed_factor_`, `flow_factor_` | int | Percentage (100 = 100%) |
| `gcode_z_offset_` | int | Microns (mm × 1000) |
| `pending_z_offset_delta_` | int | Microns (accumulated) |

**Additional methods**: `add_pending_z_offset_delta()`, `get_pending_z_offset_delta()`, `has_pending_z_offset_adjustment()`, `clear_pending_z_offset_delta()`

### 3. PrinterLedState (6 subjects)
**File**: `include/printer_led_state.h`, `src/printer/printer_led_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `led_state_` | int | 0=off, 1=on |
| `led_r_`, `led_g_`, `led_b_`, `led_w_` | int | 0-255 (from 0.0-1.0 JSON) |
| `led_brightness_` | int | 0-100 (max channel × 100 / 255) |

**Additional methods**: `set_tracked_led()`, `get_tracked_led()`, `has_tracked_led()`

### 4. PrinterFanState (2 static + dynamic)
**File**: `include/printer_fan_state.h`, `src/printer/printer_fan_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `fan_speed_` | int | 0-100% (main part-cooling fan) |
| `fans_version_` | int | Increments on fan list changes |
| `fan_speed_subjects_[name]` | map<string, unique_ptr<lv_subject_t>> | Per-fan speeds |

**Also includes**: `FanType` enum, `FanInfo` struct, `init_fans()`, `update_fan_speed()`, `classify_fan_type()`, `is_fan_controllable()`

### 5. PrinterPrintState (17 subjects)
**File**: `include/printer_print_state.h`, `src/printer/printer_print_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `print_progress_` | int | 0-100% |
| `print_filename_` | string | Raw Klipper filename (full path) |
| `print_display_filename_` | string | Clean filename for UI |
| `print_thumbnail_path_` | string | LVGL-compatible thumbnail path |
| `print_state_` | string | "standby", "printing", "paused", etc |
| `print_state_enum_` | int | PrintJobState enum |
| `print_active_` | int | Derived: 1 when PRINTING/PAUSED |
| `print_outcome_` | int | PrintOutcome - persists terminal state |
| `print_show_progress_` | int | Derived visibility flag |
| `print_layer_current_` | int | Current layer (0-based) |
| `print_layer_total_` | int | Total layer count |
| `print_duration_` | int | Elapsed time (seconds) |
| `print_time_left_` | int | Remaining time (seconds) |
| `print_start_phase_` | int | PrintStartPhase enum |
| `print_start_message_` | string | Phase message for UI |
| `print_start_progress_` | int | 0-100% during PRINT_START |
| `print_in_progress_` | int | Workflow flag (double-tap prevention) |

**Key behaviors**: Progress guard for terminal states, outcome persistence, derived subjects update automatically.

### 6. PrinterCapabilitiesState (14 subjects)
**File**: `include/printer_capabilities_state.h`, `src/printer/printer_capabilities_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `printer_has_qgl_` | int | 0/1 (QGL capability) |
| `printer_has_z_tilt_` | int | 0/1 (Z-tilt capability) |
| `printer_has_bed_mesh_` | int | 0/1 (Bed mesh capability) |
| `printer_has_nozzle_clean_` | int | 0/1 (Override only) |
| `printer_has_probe_` | int | 0/1 (Probe/BLTouch) |
| `printer_has_heater_bed_` | int | 0/1 (Heated bed) |
| `printer_has_led_` | int | 0/1 (Controllable LED) |
| `printer_has_accelerometer_` | int | 0/1 (Input shaping) |
| `printer_has_spoolman_` | int | 0/1 (Filament manager) |
| `printer_has_speaker_` | int | 0/1 (M300 beeper) |
| `printer_has_timelapse_` | int | 0/1 (Timelapse plugin) |
| `printer_has_purge_line_` | int | 0/1 (Priming capability) |
| `printer_has_firmware_retraction_` | int | 0/1 (G10/G11) |
| `printer_bed_moves_` | int | 0=gantry, 1=bed (kinematics) |

**Key methods**: `set_hardware()`, `set_spoolman_available()` (async), `set_bed_moves()`, `set_purge_line()`, `has_probe()`

### 7. PrinterPluginStatusState (2 subjects)
**File**: `include/printer_plugin_status_state.h`, `src/printer/printer_plugin_status_state.cpp`

| Subject | Type | Storage |
|---------|------|---------|
| `helix_plugin_installed_` | int | Tri-state: -1=unknown, 0=not installed, 1=installed |
| `phase_tracking_enabled_` | int | Tri-state: -1=unknown, 0=disabled, 1=enabled |

**Key methods**: `set_installed_sync()` (for PrinterState async wrapper), `set_phase_tracking_enabled()` (async), `service_has_helix_plugin()`, `is_phase_tracking_enabled()`

**Note**: `set_helix_plugin_installed()` is still in PrinterState because it needs to call `update_gcode_modification_visibility()` after updating the subject. The component provides `set_installed_sync()` for PrinterState to use inside its async wrapper.

---

## Remaining Domains

| Domain | Subjects | Complexity | Notes |
|--------|----------|------------|-------|
| **Composite Visibility** | 5 | Low | Derived flags: `can_show_bed_mesh_`, `can_show_qgl_`, etc. **Recommended next** |
| **Network/Connection** | 6 | Medium | State machine: `printer_connection_state_`, `klippy_state_`, etc. |
| **Hardware Validation** | 11 | Medium | Validation logic: `hardware_has_issues_`, etc. |
| **Calibration/Config** | 8 | Low | Simple values |
| **Firmware Retraction** | 4 | Low | |
| **Manual Probe** | 2 | Low | |
| **Excluded Objects** | 2 | Low | |
| **Versions** | 2 | Low | |
| **Kinematics** | 1 | Low | |
| **Motors** | 1 | Low | |

---

## Extraction Pattern

All extractions follow this pattern:

### 1. Write Characterization Tests First
```cpp
TEST_CASE("Domain characterization: ...", "[characterization][domain]") {
    lv_init_safe();
    PrinterState& state = get_printer_state();
    state.reset_for_testing();
    state.init_subjects(false);
    // Test current behavior
}
```

### 2. Create Domain State Class
```cpp
// include/printer_domain_state.h
namespace helix {
class PrinterDomainState {
public:
    void init_subjects(bool register_xml = true);
    void deinit_subjects();
    void update_from_status(const nlohmann::json& status);
    void reset_for_testing();

    lv_subject_t* get_subject_name() { return &subject_name_; }

private:
    SubjectManager subjects_;
    bool subjects_initialized_ = false;
    lv_subject_t subject_name_{};
};
}
```

### 3. Update PrinterState to Delegate
```cpp
// In PrinterState
lv_subject_t* get_subject_name() {
    return domain_state_.get_subject_name();
}
private:
    helix::PrinterDomainState domain_state_;
```

### 4. Verify All Tests Pass
- Characterization tests
- Existing printer tests
- Full test suite

### 5. Review and Commit

---

## Key Files

### Worktree
```
/Users/pbrown/Code/Printing/helixscreen-printer-state-decomp/
```

### Extracted Components
```
include/printer_temperature_state.h      src/printer/printer_temperature_state.cpp
include/printer_motion_state.h           src/printer/printer_motion_state.cpp
include/printer_led_state.h              src/printer/printer_led_state.cpp
include/printer_fan_state.h              src/printer/printer_fan_state.cpp
include/printer_print_state.h            src/printer/printer_print_state.cpp
include/printer_capabilities_state.h     src/printer/printer_capabilities_state.cpp
include/printer_plugin_status_state.h    src/printer/printer_plugin_status_state.cpp
```

### Test Files
```
tests/unit/test_printer_temperature_char.cpp   # 26 tests
tests/unit/test_printer_motion_char.cpp        # 21 tests
tests/unit/test_printer_led_char.cpp           # 18 tests
tests/unit/test_printer_fan_char.cpp           # 26 tests
tests/unit/test_printer_print_char.cpp         # 26 tests
tests/unit/test_printer_capabilities_char.cpp  # 17 tests
tests/unit/test_printer_plugin_char.cpp        # 9 tests
```

---

## Commands Reference

```bash
# Build
cd /Users/pbrown/Code/Printing/helixscreen-printer-state-decomp
make -j

# Run all characterization tests
./build/bin/helix-tests "[characterization]"

# Run specific domain tests
./build/bin/helix-tests "[temperature]"
./build/bin/helix-tests "[motion]"
./build/bin/helix-tests "[led]"
./build/bin/helix-tests "[fan]"
./build/bin/helix-tests "[print]"
./build/bin/helix-tests "[capabilities]"

# Run full printer state tests
./build/bin/helix-tests "[printer]" "~[slow]"

# Full test suite
make test-run

# Git status
git status --short
git log --oneline -5
```

---

## Resumption Checklist

When resuming this work:

- [ ] Read this document
- [ ] `cd /Users/pbrown/Code/Printing/helixscreen-printer-state-decomp`
- [ ] `git fetch origin && git status`
- [ ] `make -j` (verify builds)
- [ ] `./build/bin/helix-tests "[characterization]"` (134 tests, 1136 assertions)
- [ ] Pick next domain from Remaining Domains table
- [ ] Follow test-first methodology below

---

## Methodology (PROVEN WORKFLOW)

### 1. Test-First (MANDATORY)
Write characterization tests BEFORE extraction:
```bash
# Create test file
tests/unit/test_printer_<domain>_char.cpp

# Tags: [characterization][<domain>]
# Test CURRENT behavior, not desired behavior
```

### 2. Delegate to Agents
- **Explore agent**: Codebase searches, understanding existing code
- **general-purpose agent**: Implementation work
- **feature-dev:code-reviewer agent**: Review BEFORE commit

### 3. Review Catches These Bugs
Code review before commit has caught:
- Missing XML subject registrations (CRITICAL - causes runtime failures)
- Thread-safety issues (read outside async lambda)
- Missing delegation in PrinterState

### 4. Key Lessons
- `[L048]` Async tests need queue drain after async setters
- `[S002]` Stage files explicitly when committing (don't stage unrelated changes)
- `[S001]` Use conventional commits: `refactor(printer): extract Printer<Domain>State`

### 5. Commit Pattern
```bash
# Stage only relevant files
git add include/printer_<domain>_state.h src/printer/printer_<domain>_state.cpp \
        include/printer_state.h src/printer/printer_state.cpp

# Conventional commit
git commit -m "refactor(printer): extract Printer<Domain>State from PrinterState"
```

---

## Session History

### 2026-01-11 Session 1
1. Created worktree `feature/printer-state-decomposition`
2. Analyzed PrinterState: 86 subjects across 11+ domains
3. Created temperature characterization tests (26 tests)
4. **PAUSED** - Next: Extract PrinterTemperatureState class

### 2026-01-11/12 Session 2
1. Extracted PrinterTemperatureState (4 subjects)
2. Wrote motion characterization tests (21 tests)
3. Extracted PrinterMotionState (8 subjects)
4. Wrote LED characterization tests (18 tests)
5. Extracted PrinterLedState (6 subjects)
6. Wrote Fan characterization tests (26 tests)
7. Extracted PrinterFanState (2 static + dynamic per-fan subjects)
8. All 91 characterization tests passing (574 assertions)
9. **PAUSED** - Next: Print State domain (17 subjects)

### 2026-01-12 Session 3
1. Wrote Print State characterization tests (26 tests, 330 assertions)
   - Core state: print_state_, print_state_enum_, print_active_, print_outcome_
   - File info: print_filename_, print_display_filename_, print_thumbnail_path_
   - Progress: print_progress_, print_show_progress_
   - Layers: print_layer_current_, print_layer_total_
   - Time: print_duration_, print_time_left_
   - Start phases: print_start_phase_, print_start_message_, print_start_progress_
   - Workflow: print_in_progress_, can_start_new_print()
2. Critical behaviors tested: terminal state persistence, progress guards, derived subjects
3. All 117 characterization tests passing (904 assertions)
4. **PAUSED** - Next: Extract PrinterPrintState class

### 2026-01-12 Session 4
1. Extracted PrinterPrintState class (17 subjects)
   - Created `include/printer_print_state.h` and `src/printer/printer_print_state.cpp`
   - Updated PrinterState to delegate via `print_domain_` member
2. Code review identified and fixed 3 bugs:
   - Missing XML registration for `print_thumbnail_path` subject
   - Missing XML registration for `print_in_progress` subject
   - Thread-safety race condition in `set_print_start_state` (moved `old_phase` read inside lambda)
3. All 117 characterization tests still passing (904 assertions)
4. **DONE** - 5 domains now extracted

### 2026-01-12 Session 5
1. Wrote Capabilities characterization tests (17 tests, 232 assertions)
   - All 14 `printer_has_*` capability subjects
   - `printer_bed_moves_` kinematics subject
   - Tests for `set_hardware()`, `set_spoolman_available()`, `set_kinematics()`
2. Extracted PrinterCapabilitiesState class (14 subjects)
   - Created `include/printer_capabilities_state.h` and `src/printer/printer_capabilities_state.cpp`
   - Updated PrinterState to delegate via `capabilities_state_` member
3. Code review passed - no issues found
4. All 134 characterization tests passing (1136 assertions)
5. **DONE** - 6 domains now extracted

### 2026-01-12 Session 6
1. Wrote Plugin Status characterization tests (9 tests, 37 assertions)
   - Tri-state semantics: -1=unknown, 0=no, 1=yes
   - `helix_plugin_installed_`, `phase_tracking_enabled_`
   - Tests for async updates and query methods
2. Extracted PrinterPluginStatusState class (2 subjects)
   - Created `include/printer_plugin_status_state.h` and `src/printer/printer_plugin_status_state.cpp`
   - Updated PrinterState to delegate via `plugin_status_state_` member
   - Note: `set_helix_plugin_installed()` remains in PrinterState to handle visibility update
3. Code review found and fixed: missing `get_phase_tracking_enabled_subject()` getter
4. All 143 characterization tests passing (1173 assertions)
5. **DONE** - 7 domains now extracted

---

## Commits on Branch

```
TBD      refactor(printer): extract PrinterPluginStatusState from PrinterState
TBD      test(char): add plugin status characterization tests (2 subjects)
0c045f49 refactor(printer): extract PrinterCapabilitiesState from PrinterState
1da837bf test(char): add capabilities domain characterization tests (14 subjects)
ebc1971c refactor(ui): migrate to hardware() API for macros and sensors
39730e16 docs: update PrinterState decomposition handoff for Print State extraction
7dfd653d refactor(printer): extract PrinterPrintState from PrinterState
99da8b67 test(char): add print domain characterization tests (17 subjects)
ee5ac704 refactor(printer): extract PrinterLedState and PrinterFanState
dfa92d60 refactor(printer): extract PrinterMotionState from PrinterState
36dec0bb refactor(printer): extract PrinterTemperatureState from PrinterState
cf74706d test(char): add temperature domain characterization tests
```

---

HANDOFF: PrinterState God Class Decomposition - 7 domains extracted (Temperature, Motion, LED, Fan, Print, Capabilities, Plugin Status)
