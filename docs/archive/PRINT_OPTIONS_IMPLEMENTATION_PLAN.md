# Print-Time Options Override System

## Overview

Implement a **Bambu-style pre-print options dialog** that allows users to enable/disable operations like bed leveling, chamber soak, and nozzle cleaning before starting a print from HelixScreen.

**Key Design Decisions:**
- **Primary approach**: G-code injection (prefer not modifying files)
- **Secondary approach**: HelixScreen macros for managed printers
- **UX for long operations**: Transition immediately to print status panel with "Preparing" phase

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│            Print File Detail → Options Dialog                    │
│  ☑ Bed Leveling   ☑ Nozzle Clean   ☐ Chamber Soak (45°C, 5min)  │
└───────────────────────────┬─────────────────────────────────────┘
                            │ User clicks "Print"
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                    PrintOptionsManager                           │
│  1. Detect printer capabilities (macros, sensors)                │
│  2. Choose execution strategy (macros vs injection)              │
│  3. Execute pre-print sequence                                   │
└───────────────────────────┬─────────────────────────────────────┘
                            │
         ┌──────────────────┴──────────────────┐
         ▼                                     ▼
┌─────────────────────────┐     ┌──────────────────────────────┐
│   HELIX_START_PRINT     │     │   G-code Injection           │
│   (if macros installed) │     │   (fallback - always works)  │
├─────────────────────────┤     ├──────────────────────────────┤
│ Single macro call with  │     │ Sequential execute_gcode()   │
│ parameters:             │     │ calls before start_print():  │
│   BED_LEVEL=1           │     │   G28                        │
│   CHAMBER_SOAK=5        │     │   BED_MESH_CALIBRATE         │
│   NOZZLE_CLEAN=1        │     │   CLEAN_NOZZLE (if exists)   │
└─────────────────────────┘     └──────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│              Print Status Panel (PREPARING phase)                │
│  Shows: "Running bed leveling..." with progress                  │
│  Then transitions to normal PRINTING phase                       │
└─────────────────────────────────────────────────────────────────┘
```

---

## Data Structures

### PrintOption (extensible option definition)

```cpp
// include/print_options.h

struct PrintOption {
    std::string id;                    // "bed_leveling", "chamber_soak", etc.
    std::string display_name;          // "Automatic Bed Leveling"
    std::string description;           // Tooltip text
    bool enabled = false;              // User's current selection
    bool available = false;            // Printer supports this option

    // Detection: macros to look for (priority order)
    std::vector<std::string> supported_macros;  // {"BED_MESH_CALIBRATE", "G29"}
    std::string detected_macro;                  // Actually found macro

    // For options with parameters (chamber soak temp/time)
    bool has_parameter = false;
    std::string parameter_name;        // "temperature" or "duration"
    int parameter_value = 0;
    int parameter_min = 0;
    int parameter_max = 100;
};

struct PrintOptions {
    bool bed_level = true;
    bool nozzle_clean = false;
    bool chamber_soak = false;
    int chamber_soak_temp = 45;        // °C
    int chamber_soak_minutes = 5;
    bool purge_line = true;
    std::string filament_type;         // From file or overridden

    // Future extensibility
    // bool ams_enabled = false;
    // int ams_slot = 0;
};
```

### PrinterCapabilities (detected from Moonraker)

```cpp
struct PrinterCapabilities {
    // Detected from printer.objects.list
    bool has_bed_mesh = false;
    bool has_z_tilt = false;
    bool has_quad_gantry_level = false;
    bool has_chamber_heater = false;
    bool has_chamber_sensor = false;

    // Detected macros
    std::vector<std::string> available_macros;
    bool has_helix_macros = false;         // HELIX_START_PRINT installed
    std::string helix_macros_version;

    // Nozzle cleaning variants
    std::string nozzle_clean_macro;        // CLEAN_NOZZLE, NOZZLE_WIPE, etc.
};
```

---

## Implementation Stages

### Stage 1: PrintOptionsManager Core (2-3 days)

**New files:**
- `include/print_options.h` - Data structures
- `src/print_options.cpp` - PrintOptionsManager implementation

**PrintOptionsManager responsibilities:**
1. Detect printer capabilities from `printer.objects.list`
2. Store/load user option preferences
3. Execute pre-print sequence (macro or injection)
4. Report progress to UI

```cpp
class PrintOptionsManager {
public:
    PrintOptionsManager(MoonrakerClient& client, MoonrakerAPI& api);

    // Call after printer discovery
    void detect_capabilities(std::function<void()> on_complete);

    // Get options for UI
    const std::vector<PrintOption>& get_options() const;
    PrinterCapabilities get_capabilities() const;

    // User selections
    void set_option_enabled(const std::string& id, bool enabled);
    void set_option_parameter(const std::string& id, int value);

    // Execute pre-print sequence, then call on_ready_to_print
    void execute_preprint(
        const PrintOptions& options,
        std::function<void()> on_ready_to_print,
        std::function<void(const std::string& error)> on_error,
        std::function<void(const std::string& status, float progress)> on_progress
    );

private:
    void execute_with_macros(const PrintOptions& opts, ...);
    void execute_with_injection(const PrintOptions& opts, ...);
    void execute_gcode_chain(std::vector<std::string> commands, size_t index, ...);
};
```

### Stage 2: Capability Detection (1-2 days)

**Modify:** `src/moonraker_client.cpp`

Extend `parse_objects()` to detect:
- `gcode_macro *` entries → populate `available_macros`
- `bed_mesh` → `has_bed_mesh = true`
- `quad_gantry_level` → `has_quad_gantry_level = true`
- `heater_generic chamber` → `has_chamber_heater = true`
- `temperature_sensor chamber` → `has_chamber_sensor = true`

```cpp
// In parse_objects(), after existing sensor/fan detection:
for (const auto& obj : objects) {
    if (obj.rfind("gcode_macro ", 0) == 0) {
        std::string macro_name = obj.substr(12);  // After "gcode_macro "
        capabilities_.available_macros.push_back(macro_name);

        if (macro_name == "HELIX_START_PRINT") {
            capabilities_.has_helix_macros = true;
        }
    }
    else if (obj == "bed_mesh") {
        capabilities_.has_bed_mesh = true;
    }
    // ... etc
}
```

### Stage 3: G-code Injection Engine (2-3 days)

**Add to:** `src/print_options.cpp`

Build G-code command sequences based on options and detected capabilities:

```cpp
std::vector<std::string> PrintOptionsManager::build_injection_sequence(
    const PrintOptions& opts) {

    std::vector<std::string> commands;

    // Home if needed (most operations require it)
    if (opts.bed_level || opts.nozzle_clean) {
        commands.push_back("G28");  // Home all
    }

    // Chamber soak (if enabled and chamber exists)
    if (opts.chamber_soak && caps_.has_chamber_heater) {
        commands.push_back(fmt::format(
            "SET_HEATER_TEMPERATURE HEATER=chamber TARGET={}",
            opts.chamber_soak_temp));
        commands.push_back(fmt::format(
            "TEMPERATURE_WAIT SENSOR=\"heater_generic chamber\" MINIMUM={}",
            opts.chamber_soak_temp));
        // Additional soak time
        commands.push_back(fmt::format("G4 P{}", opts.chamber_soak_minutes * 60000));
    }

    // Bed leveling
    if (opts.bed_level) {
        if (caps_.has_quad_gantry_level) {
            commands.push_back("QUAD_GANTRY_LEVEL");
        }
        if (caps_.has_bed_mesh) {
            commands.push_back("BED_MESH_CALIBRATE");
        }
    }

    // Nozzle cleaning (if detected)
    if (opts.nozzle_clean && !caps_.nozzle_clean_macro.empty()) {
        commands.push_back(caps_.nozzle_clean_macro);
    }

    return commands;
}
```

### Stage 4: HelixScreen Macros (2-3 days)

**New files:**
- `config/helix_macros.cfg.template` - Klipper macro definitions
- `src/helix_macro_manager.cpp` - Installation/detection logic

**Macro design** (portable across printer types):

```ini
# config/helix_macros.cfg.template
[gcode_macro HELIX_START_PRINT]
description: HelixScreen pre-print preparation
gcode:
    {% set bed_level = params.BED_LEVEL|default(0)|int %}
    {% set chamber_soak = params.CHAMBER_SOAK|default(0)|int %}
    {% set chamber_temp = params.CHAMBER_TEMP|default(0)|int %}
    {% set nozzle_clean = params.NOZZLE_CLEAN|default(0)|int %}

    # Home if not homed
    {% if "xyz" not in printer.toolhead.homed_axes %}
        G28
    {% endif %}

    # Chamber soak
    {% if chamber_soak > 0 and chamber_temp > 0 %}
        {% if printer['heater_generic chamber'] is defined %}
            SET_HEATER_TEMPERATURE HEATER=chamber TARGET={chamber_temp}
            TEMPERATURE_WAIT SENSOR="heater_generic chamber" MINIMUM={chamber_temp}
            G4 P{chamber_soak * 60000}
        {% endif %}
    {% endif %}

    # Bed leveling
    {% if bed_level == 1 %}
        {% if printer.quad_gantry_level is defined %}
            QUAD_GANTRY_LEVEL
        {% endif %}
        {% if printer.bed_mesh is defined %}
            BED_MESH_CALIBRATE
        {% endif %}
    {% endif %}

    # Nozzle cleaning (delegates to user macro if exists)
    {% if nozzle_clean == 1 %}
        {% if printer['gcode_macro CLEAN_NOZZLE'] is defined %}
            CLEAN_NOZZLE
        {% elif printer['gcode_macro NOZZLE_WIPE'] is defined %}
            NOZZLE_WIPE
        {% endif %}
    {% endif %}
```

**Installation flow:**
1. Upload `helix_macros.cfg` to printer via Moonraker HTTP file upload
2. Add `[include helix_macros.cfg]` to printer.cfg (with user confirmation)
3. Trigger Klipper restart via `printer.emergency_stop` + wait

### Stage 5: UI Integration (2-3 days)

**Modify:** `ui_xml/print_file_detail.xml`

```xml
<!-- Expand options card -->
<ui_card name="options_card" width="100%" flex_flow="column">
  <text_heading text="Pre-Print Options" width="100%"/>

  <!-- Options populated dynamically based on printer capabilities -->
  <lv_obj name="options_container" width="100%" flex_flow="column"
          style_pad_gap="#gap_small">
    <!-- Checkboxes added by C++ based on detected capabilities -->
  </lv_obj>

  <!-- Chamber soak parameters (shown when chamber_soak checked) -->
  <lv_obj name="chamber_params" width="100%" flex_flow="row" hidden="true">
    <text_body text="Temp:" width="50"/>
    <lv_spinbox name="chamber_temp_spin" width="70" min="30" max="80"/>
    <text_body text="°C  Time:" width="70"/>
    <lv_spinbox name="chamber_time_spin" width="70" min="1" max="30"/>
    <text_body text="min"/>
  </lv_obj>

  <!-- Filament dropdown (existing) -->
  <text_body text="Filament Type" width="100%"/>
  <lv_dropdown name="filament_dropdown" .../>
</ui_card>
```

**Modify:** `src/ui_panel_print_select.cpp`

```cpp
void PrintSelectPanel::populate_options() {
    auto* container = lv_obj_find_by_name(detail_view_, "options_container");
    lv_obj_clean(container);

    for (const auto& opt : print_options_mgr_->get_options()) {
        if (!opt.available) continue;

        auto* cb = lv_checkbox_create(container);
        lv_checkbox_set_text(cb, opt.display_name.c_str());
        if (opt.enabled) lv_obj_add_state(cb, LV_STATE_CHECKED);
        lv_obj_set_user_data(cb, (void*)&opt);
        lv_obj_add_event_cb(cb, on_option_changed, LV_EVENT_VALUE_CHANGED, this);
    }
}

void PrintSelectPanel::start_print() {
    PrintOptions opts = collect_options_from_ui();

    // Immediately transition to print status panel with PREPARING phase
    ui_nav_push_overlay(print_status_panel_);
    print_status_panel_->set_phase(PrintPhase::PREPARING);

    print_options_mgr_->execute_preprint(opts,
        // on_ready_to_print
        [this]() {
            api_->start_print(selected_filename_,
                []() { /* success */ },
                [](auto& err) { NOTIFY_ERROR("Start failed: {}", err.message); });
        },
        // on_error
        [this](const std::string& error) {
            print_status_panel_->set_error(error);
        },
        // on_progress
        [this](const std::string& status, float progress) {
            print_status_panel_->set_preparing_status(status, progress);
        }
    );
}
```

### Stage 6: Print Status Panel "Preparing" Phase (1-2 days)

**Modify:** `src/ui_panel_print_status.cpp`

Add `PREPARING` to print phases, shown before actual printing starts:

```cpp
enum class PrintPhase {
    PREPARING,   // NEW: Pre-print operations in progress
    PRINTING,
    PAUSED,
    COMPLETE,
    CANCELLED,
    ERROR
};

void PrintStatusPanel::set_preparing_status(const std::string& status, float progress) {
    // Update UI: "Preparing: Running bed leveling..." with progress bar
    lv_label_set_text(status_label_, status.c_str());
    lv_bar_set_value(progress_bar_, static_cast<int>(progress * 100), LV_ANIM_ON);
}
```

---

## File Changes Summary

| File | Change | Description |
|------|--------|-------------|
| `include/print_options.h` | **NEW** | PrintOption, PrintOptions, PrinterCapabilities structs |
| `src/print_options.cpp` | **NEW** | PrintOptionsManager implementation |
| `include/helix_macro_manager.h` | **NEW** | Macro detection/installation |
| `src/helix_macro_manager.cpp` | **NEW** | Macro upload and printer.cfg modification |
| `config/helix_macros.cfg.template` | **NEW** | HelixScreen Klipper macros |
| `include/moonraker_client.h` | MODIFY | Add PrinterCapabilities, macro list |
| `src/moonraker_client.cpp` | MODIFY | Detect macros in `parse_objects()` |
| `include/moonraker_api.h` | MODIFY | Add HTTP file upload method |
| `src/moonraker_api.cpp` | MODIFY | Implement file upload via libhv HTTP |
| `ui_xml/print_file_detail.xml` | MODIFY | Expand options UI |
| `include/ui_panel_print_select.h` | MODIFY | Add PrintOptionsManager member |
| `src/ui_panel_print_select.cpp` | MODIFY | Wire options UI, update start_print() |
| `src/ui_panel_print_status.cpp` | MODIFY | Add PREPARING phase handling |

---

## Future Extensibility

The system is designed for future options:

| Future Option | Detection | Implementation |
|---------------|-----------|----------------|
| **AMS/MMU** | `mmu` or `ercf` object | Pass `AMS_SLOT=X` to macro |
| **Filament runout** | `filament_switch_sensor` | Enable/disable sensor before print |
| **First layer calibration** | `z_calibration` macro | Run `CALIBRATE_Z` in sequence |
| **Pressure advance** | Per-filament PA values | Set `SET_PRESSURE_ADVANCE` |

---

## Success Criteria

- [x] Options dialog shows only available options for detected printer
- [x] Bed leveling option triggers `BED_MESH_CALIBRATE` or equivalent
- [x] Chamber soak waits for temperature + duration
- [x] Nozzle clean runs detected cleaning macro
- [x] Print status panel shows "Preparing" phase with progress
- [x] Works on printers without HelixScreen macros (injection fallback)
- [x] Macro installation available for managed printers
- [x] Options preferences persist across sessions

---

## Implementation Status

> **✅ IMPLEMENTATION COMPLETE** - All stages implemented and verified (2025-11-29)

**Completed Stages:**
- Stages 1-6: Core pre-print options, capability detection, G-code injection, preparing UI
- Stage 7: GCodeFileModifier - comments out operations when user disables them
- Stage 8: Capability overrides - user can force enable/disable features in helixconfig.json
- Stage 9: Concurrent print prevention - blocks starting new print while one is active
- Stage 10: Memory-safe streaming for large G-code files on embedded devices
- Startup cleanup of stale temp files integrated into main.cpp

**Key files for understanding current implementation:**
- `src/ui_panel_print_select.cpp` - start_print() with pre-print sequencer
- `src/gcode_file_modifier.cpp` - modifies G-code to disable embedded operations
- `src/capability_overrides.cpp` - three-state override system (auto/enable/disable)
- `include/printer_state.h` - PrintJobState enum and can_start_new_print()

**Remaining work:** Modified file cleanup after print completes (minor enhancement).

---

## Stage 7: GCodeFileModifier Integration (IMPLEMENTED)

### Overview

When a user **unchecks** a pre-print option but the G-code file already contains that operation embedded, we need to **disable the embedded operation** rather than just skipping it in the sequencer. This prevents the operation from running even though the user disabled it.

### Architecture

```
User unchecks "Bed Leveling" in UI
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│               PrintSelectPanel::start_print()            │
│  1. Read checkbox states                                 │
│  2. Scan G-code file for embedded operations             │
│  3. Compare: user wants OFF but file has it ON?          │
└─────────────────────────────┬───────────────────────────┘
                              │ Yes - need modification
                              ▼
┌─────────────────────────────────────────────────────────┐
│                   GCodeFileModifier                      │
│  • Download original file from Moonraker                 │
│  • disable_operations(scan_result, ops_to_disable)       │
│  • apply_to_content() → modified G-code string           │
│  • Upload to .helix_temp/ directory                      │
└─────────────────────────────┬───────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────┐
│  api_->start_print(modified_filename)                    │
│  Cleanup after print completes                           │
└─────────────────────────────────────────────────────────┘
```

### Implementation (Already Complete)

**Files implemented:**
- `include/gcode_file_modifier.h` - Modification types and GCodeFileModifier class
- `src/gcode_file_modifier.cpp` - Full implementation
- `tests/unit/test_gcode_file_modifier.cpp` - 10 test cases, 66 assertions

**Key features:**
- Comment out lines/ranges: `; BED_MESH_CALIBRATE  ; [HelixScreen: Disabled]`
- Inject G-code before/after specific lines
- Replace lines with new content
- Handle macro parameters: `FORCE_LEVELING=true` → `FORCE_LEVELING=FALSE`
- Integration with `GCodeOpsDetector::ScanResult`
- Temp file generation with automatic cleanup

### Integration with start_print()

**File: `src/ui_panel_print_select.cpp`**

```cpp
void PrintSelectPanel::start_print() {
    // ... read checkbox states ...

    // Check if file has embedded operations that need disabling
    bool file_has_bed_mesh = cached_scan_result_ &&
        cached_scan_result_->get_operation(gcode::OperationType::BED_LEVELING).has_value();

    std::vector<gcode::OperationType> ops_to_disable;
    if (!do_bed_leveling && file_has_bed_mesh) {
        ops_to_disable.push_back(gcode::OperationType::BED_LEVELING);
    }
    // ... similar for QGL, Z_TILT, NOZZLE_CLEAN, HEAT_SOAK

    if (!ops_to_disable.empty()) {
        // Download → modify → upload → print modified file
        modify_and_print(filename, ops_to_disable);
    } else {
        // Original flow: sequencer for checked ops, or direct print
        // ... existing code ...
    }
}
```

---

## Stage 8: Capability Override System

### Problem

Auto-detection from Moonraker can't cover everything:
- Heat soak doesn't require chamber heater (bed-only soak works)
- Some printers have macros with non-standard names
- Users may want to force enable/disable features regardless of detection

### Solution: Three-State Override

```cpp
// In helixconfig.json
{
  "capability_overrides": {
    "heat_soak": "auto",      // "auto" | "enable" | "disable"
    "bed_leveling": "auto",
    "qgl": "auto",
    "z_tilt": "auto",
    "nozzle_clean": "enable"  // Force enable even if not auto-detected
  }
}
```

### Implementation

**New files:**
- `include/capability_overrides.h`
- `src/capability_overrides.cpp`

```cpp
class CapabilityOverrides {
public:
    enum class OverrideState { AUTO, ENABLE, DISABLE };

    bool is_available(const std::string& capability) const {
        auto override = get_override(capability);
        if (override == OverrideState::ENABLE) return true;
        if (override == OverrideState::DISABLE) return false;
        // AUTO: delegate to PrinterCapabilities
        return printer_caps_.is_available(capability);
    }

private:
    PrinterCapabilities& printer_caps_;
    std::map<std::string, OverrideState> overrides_;
};
```

---

## Stage 9: Concurrent Print Prevention

### Problem

User shouldn't be able to start a new print while one is already running.

### Solution

**UI-level check in start_print():**
```cpp
void PrintSelectPanel::start_print() {
    if (printer_state_.get_print_state() != PrintState::STANDBY) {
        NOTIFY_ERROR("Cannot start print: printer is {}",
                     printer_state_.get_print_state_string());
        return;
    }
    // ... rest of start_print
}
```

**Reactive UI binding (disable Print button when not standby):**
```xml
<lv_button name="print_button">
  <lv_obj-bind_state_if_not_eq subject="print_state" state="disabled" ref_value="0"/>
  <text_body text="Print"/>
</lv_button>
```

**Fallback:** Moonraker also returns an error if already printing, which we handle.

---

## Stage 10: Memory-Safe Streaming for Large Files

### Problem

Embedded Linux devices may have 256MB-512MB RAM. G-code files can be 100MB+. Loading entire files into memory is unacceptable.

### Solution: Configurable Threshold

```cpp
constexpr size_t MAX_BUFFERED_FILE_SIZE = 5 * 1024 * 1024;  // 5MB default

// For small files: in-memory (fast)
std::string apply_to_content(const std::string& content);

// For large files: streaming (memory-safe)
bool apply_streaming(std::istream& input, std::ostream& output,
                     const std::vector<Modification>& mods);
```

### Streaming Implementation

1. **First pass:** Scan file line-by-line for operations (already works)
2. **Build lookup map:** `std::map<line_number, Modification>` for O(1) per-line
3. **Stream forward:** Read line, check map, apply modification if found, write
4. **Chunked upload:** Use HTTP chunked transfer to Moonraker

**Files to modify:**
- `src/gcode_file_modifier.cpp` - Add `apply_streaming()` method
- `src/moonraker_api.cpp` - Add chunked upload if not present

---

## Print History Filename Handling

### Problem

Modified files show as `helix_mod_*.gcode` in Moonraker history.

### Research Finding

Moonraker's history namespace is **read-only** for clients - no `server.history.modify` API.

### Options

| Option | Description | Complexity |
|--------|-------------|------------|
| **A: Accept** | History shows temp name, user still knows what they printed | Simple |
| **B: Metadata** | Inject comment: `; HelixScreen Modified: Original: 3DBenchy.gcode` | Medium |
| **C: Local DB** | Store mapping in local SQLite | Complex |

**Recommendation:** Option A for v1, Option B as enhancement.

---

## Future: HelixScreen Moonraker Plugin

Some limitations could be better solved server-side via `moonraker-helixscreen` plugin:

### What a Plugin Could Solve

| Problem | Client-side Workaround | Plugin Solution |
|---------|------------------------|-----------------|
| **Print history** | Embed metadata in G-code | Modify history records directly |
| **G-code modification** | Download → modify → upload | Modify in-place (no transfer!) |
| **Large files** | Chunked HTTP transfer | Direct filesystem, zero overhead |
| **Cleanup** | Track & delete temp files | Plugin manages temp directory |

### Plugin Architecture

```
moonraker-helixscreen/
├── __init__.py            # Moonraker component registration
├── helix_file_modifier.py # Server-side G-code modification
├── helix_history.py       # History enrichment
└── helix_api.py           # Custom JSON-RPC endpoints
```

### Custom API Endpoints (Future)

```python
# POST server.helix.start_print_with_options
{
    "filename": "3DBenchy.gcode",
    "pre_print_ops": {
        "heat_soak": true,
        "bed_leveling": false,  # Disable if in G-code
        "qgl": true
    }
}
# Plugin handles: modify file, update history, start print
```

**Phases:**
- **Phase 1-2:** Client-side solution (works without plugin)
- **Phase 3+:** Develop Moonraker plugin for optimization

---

## Updated Success Criteria

> **✅ ALL CRITERIA MET** - Verified 2025-11-29

- [x] Options dialog shows only available options for detected printer
- [x] Bed leveling option triggers `BED_MESH_CALIBRATE` or equivalent
- [x] Chamber soak waits for temperature + duration
- [x] Nozzle clean runs detected cleaning macro
- [x] Print status panel shows "Preparing" phase with progress
- [x] Works on printers without HelixScreen macros (injection fallback)
- [x] Macro installation available for managed printers
- [x] Options preferences persist across sessions
- [x] **GCodeFileModifier comments out operations when user disables them**
- [x] **GCodeFileModifier handles macro parameters (FORCE_LEVELING=true → FALSE)**
- [x] **Capability override system allows force enable/disable** (Stage 8)
- [x] **Concurrent print prevention at UI level** (Stage 9)
- [x] **Modified file cleanup on startup** (age-based, 1 hour threshold)
- [x] Large file streaming for memory-constrained devices (Stage 10)
