# G-code Monitoring: Testing Plan

**Branch:** `feature/gcode-monitoring`
**Features:** PRINT_START Progress Indicator, Console Panel Phase 2

---

## Summary of Implementation

### Console Panel Phase 2 ✅
| Feature | Status | Description |
|---------|--------|-------------|
| Real-time streaming | ✅ Done | `notify_gcode_response` subscription in `on_activate()` |
| G-code input field | ✅ Done | Text input + send button, keyboard integration |
| Temperature filtering | ✅ Done | Filters `T:/B:` temperature status lines |
| Clear button | ✅ Done | Clears console history display |
| Smart auto-scroll | ✅ Done | Only auto-scrolls if user hasn't scrolled up |

### PrintStartCollector ✅
| Feature | Status | Description |
|---------|--------|-------------|
| Pattern matching | ✅ Done | Regex patterns for all PRINT_START phases |
| Voron V2 support | ✅ Done | QGL, VORON_PURGE, clean_nozzle, status_* macros |
| AD5M Pro support | ✅ Done | KAMP, LINE_PURGE, _PRINT_STATUS messages |
| Unit tests | ✅ Done | 31 test cases, 287 assertions |

### Pattern Coverage
The following patterns are detected (case-insensitive):

| Phase | Patterns |
|-------|----------|
| **Homing** | `G28`, `Homing`, `Home All Axes`, `status_homing` |
| **Heating Bed** | `M190`, `M140 S[1-9]`, `Heating bed`, `BED_TEMP`, `status_heating` |
| **Heating Nozzle** | `M109`, `M104 S[1-9]`, `Heating nozzle/hotend/extruder`, `EXTRUDER_TEMP`, `status_heating` |
| **QGL** | `QUAD_GANTRY_LEVEL`, `QGL`, `status_leveling` |
| **Z Tilt** | `Z_TILT_ADJUST`, `status_leveling` |
| **Bed Mesh** | `BED_MESH_CALIBRATE`, `BED_MESH_PROFILE LOAD=`, `status_meshing`, `_KAMP_BED_MESH_CALIBRATE` |
| **Cleaning** | `CLEAN_NOZZLE`, `NOZZLE_CLEAN`, `WIPE_NOZZLE`, `status_cleaning` |
| **Purging** | `VORON_PURGE`, `LINE_PURGE`, `PURGE_LINE`, `Prime Line`, `KAMP_*_PURGE` |

---

## Live Testing Checklist

### Prerequisites
- [ ] HelixScreen connected to printer via Moonraker
- [ ] Printer ready for test print (bed clear, filament loaded)

### Console Panel Tests
1. **Real-time streaming**
   - [ ] Navigate to Console panel
   - [ ] Send a G-code command via Moonraker/Mainsail
   - [ ] Verify response appears in console within 1 second
   - [ ] Verify temperature status lines are filtered (not shown)

2. **G-code input**
   - [ ] Tap the input field → on-screen keyboard appears
   - [ ] Type `M115` and tap Send → command sent, response shown
   - [ ] Type `G28 X` → homing executes
   - [ ] Physical keyboard Enter key submits command

3. **Clear button**
   - [ ] Tap Clear → all entries removed
   - [ ] Empty state shown ("No History")

4. **Auto-scroll behavior**
   - [ ] Send multiple commands rapidly → console scrolls to bottom
   - [ ] Scroll up manually → new messages don't auto-scroll
   - [ ] Scroll back to bottom → auto-scroll resumes

### PRINT_START Progress Tests

#### Test on Voron V2 (192.168.1.112)
1. **Start a test print**
   - [ ] Select a small test file
   - [ ] Observe home panel during PRINT_START

2. **Phase detection** (verify each phase shows):
   - [ ] Homing → "Homing..."
   - [ ] Heating Bed → "Heating Bed..."
   - [ ] Heating Nozzle → "Heating Nozzle..."
   - [ ] QGL → "Leveling Gantry..."
   - [ ] Bed Mesh → "Probing Bed..."
   - [ ] Cleaning → "Cleaning Nozzle..."
   - [ ] Purging → "Purging..."

3. **Progress bar**
   - [ ] Progress bar appears on print_card
   - [ ] Progress increases as phases complete
   - [ ] Progress reaches 100% before layer 1

4. **Completion**
   - [ ] Progress UI hides when print starts
   - [ ] Print status shows normally

#### Test on AD5M Pro (192.168.1.67)
1. **Repeat above tests**
   - [ ] Phases detected via KAMP patterns
   - [ ] _PRINT_STATUS messages recognized
   - [ ] LINE_PURGE detected

### Mock Mode Tests
```bash
./build/bin/helix-screen --test -p home -vv
```
- [ ] Simulated PRINT_START phases cycle through
- [ ] Progress bar animates
- [ ] Console shows simulated G-code responses

---

## Known Limitations

1. **Pattern matching is best-effort** - Custom macros with non-standard naming may not be detected
2. **Temperature filtering is simple** - Uses `T:/B:` heuristic, may miss some formats
3. **No command history (up/down arrows)** - Deferred to future phase

---

## Files Changed

### New Files
- `include/print_start_collector.h`
- `src/print_start_collector.cpp`
- `tests/unit/test_print_start_collector.cpp`

### Modified Files
- `include/printer_state.h` - PrintStartPhase enum, subjects
- `src/printer_state.cpp` - Subject initialization
- `include/ui_panel_console.h` - Phase 2 methods
- `src/ui_panel_console.cpp` - Real-time, input, filtering
- `ui_xml/console_panel.xml` - Input row, clear button
- `ui_xml/home_panel.xml` - Progress UI elements
- `src/main.cpp` - PrintStartCollector initialization
- `src/moonraker_client_mock.cpp` - Phase simulation
- `tests/unit/test_ui_panel_console.cpp` - Temp filter tests
- `include/ui_icon_codepoints.h` - Send icon
- `scripts/regen_mdi_fonts.sh` - Send icon codepoint
- `assets/fonts/mdi_icons_*.c` - Regenerated fonts

---

## Merge Checklist

- [ ] All unit tests pass (`make tests`)
- [ ] Build succeeds on native (`make -j`)
- [ ] Build succeeds on Pi (`make remote-pi`)
- [ ] Live testing complete on at least one printer
- [ ] No regressions in existing functionality
- [ ] Code reviewed for CLAUDE.md compliance
