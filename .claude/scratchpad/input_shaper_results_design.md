# Input Shaper Results Page — Full Design Vision

## Status (2026-02-11)

### DONE
- State machine collector (WAITING→SWEEPING→CALCULATING→COMPLETE)
- Progress bar in MEASURING state (timer-based mock, animated)
- Fixed recommendation regex (new Klipper format)
- Parsing max_accel per shaper, CSV path capture
- PID-style centered layout with icon + heading
- "Save" button (Apply + SAVE_CONFIG) and "Close"
- 108 tests, 468 assertions, all passing
- Committed: `fix(input-shaper): defer completion until CSV path captured`

---

## TODO — Results Page Makeover

### 1. Full Shaper Comparison Table

The original design vision (docs/archive/plans/INPUT_SHAPER_HANDOFF.md):
```
│  Recommended: MZV @ 36.7 Hz         │
│  ┌─────────────────────────────────┐│
│  │ Type      Freq    Vibration     ││
│  │ zv        35.8    22.7%         ││
│  │ mzv ★     36.7    7.2%   ← rec ││
│  │ ei        43.2    6.8%          ││
│  │ 2hump_ei  52.4    5.0%          ││
│  │ 3hump_ei  62.0    4.2%          ││
│  └─────────────────────────────────┘│
```

We have `all_shapers` vector with 5 ShaperOption entries per axis:
- type, frequency, vibrations, smoothing, max_accel
Each axis gets its own card with a comparison table. Recommended row is highlighted.

### 2. Frequency Response Chart

#### Existing Widget: `ui_frequency_response_chart`
**We already have a purpose-built widget!** 602 lines of tested code:
- `include/ui_frequency_response_chart.h` (252 lines)
- `src/ui/ui_frequency_response_chart.cpp` (602 lines)
- `tests/unit/test_frequency_response_chart.cpp`

Features:
- Hardware tier adaptation (EMBEDDED=table only, BASIC=50pts, STANDARD=200pts)
- Automatic downsampling preserving frequency range endpoints
- Peak marking with vertical marker + annotation
- Up to 8 series per chart, show/hide toggle
- C API: `create()`, `add_series()`, `set_data()`, `mark_peak()`, `configure_for_platform()`

#### Data Source: CSV from Klipper
File: `/tmp/calibration_data_x_YYYYMMDD_HHMMSS.csv`
Format: 132 rows, columns:
```
freq, psd_x, psd_y, psd_z, psd_xyz, zv(59.0), mzv(53.8), ei(56.2), 2hump_ei(71.8), 3hump_ei(89.6)
```
- `csv_path` is now captured by the collector
- `freq_response` field exists in `InputShaperResult` (vector<pair<float,float>>) — NOT yet populated
- Cache serialization for freq_response already implemented in `input_shaper_cache.cpp:133-172`

#### Chart Shows:
- Raw vibration (psd_xyz) — primary color, thick line
- Recommended shaper response — success color, filtered vibration overlay
- Peak frequency highlighted via `mark_peak()`
- Before/after comparison: raw vs filtered

### 3. CSV Access Strategy

**Problem:** Moonraker file API only serves managed roots (gcodes, config). `/tmp/` is NOT a registered root.

**Solution — local file read with graceful degradation:**

1. **Check locality:** `MoonrakerApi::get_websocket_url()` → extract host → check if localhost/127.0.0.1/::1
   - Use `extract_host_from_websocket_url()` and `is_local_host()` from `helix_plugin_installer.cpp`
   - Or inline the trivial check (3 string comparisons)

2. **If local:** Read CSV directly with `std::ifstream`. The path is already captured in `csv_path`.
   - This covers: Pi 3/4/5 running locally, AD5M, any single-machine setup
   - This is the vast majority of real deployments

3. **If remote:** Degrade gracefully — no chart, results cards still show all shaper data.
   - Future: document adding `/tmp/` as Moonraker file root in `moonraker.conf`:
     ```ini
     [file_manager]
     queue_gcode_uploads: True

     # Enable frequency response chart in HelixScreen
     [server]
     additional_file_roots:
       calibration_data: /tmp
     ```

4. **Mock mode:** Generate synthetic CSV data matching real format (132 rows, resonance peak at ~53 Hz)

### 4. Visual Quality Indicators

Already implemented in C++ (`get_vibration_quality`, `get_quality_description`):
- < 3% vibration = Excellent (green)
- < 7% = Good (success color)
- < 15% = Acceptable (warning color)
- >= 15% = High (danger color)

Color-code the vibration column in the comparison table.

### 5. Before/After Comparison

If input shaper was already configured before calibration:
- Show "Previous: MZV @ 36.7 Hz → New: EI @ 53.8 Hz"
- `InputShaperConfig` is queried on panel activate, stored in subjects
- Struct has: `shaper_type_x`, `shaper_freq_x`, `shaper_type_y`, `shaper_freq_y`, `is_configured`

---

## Hardware Constraints & Performance Budget

### Target Platforms

| Platform | RAM | Free RAM | CPU | Tier |
|----------|-----|----------|-----|------|
| AD5M | 110 MB | ~37 MB | Single core | EMBEDDED |
| Pi 3 | 1 GB | ~500 MB | Quad core 1.2GHz | BASIC |
| Pi 4 (2GB) | 2 GB | ~1.5 GB | Quad core 1.8GHz | STANDARD |
| Pi 5 / Desktop | 4+ GB | plenty | Quad+ core | STANDARD |

### Chart Memory Budget
- LVGL chart widget: ~5-10 KB overhead
- Per series: `point_count × sizeof(lv_coord_t)` = 200 × 4 = 800 bytes
- 2 series (raw + filtered): ~1.6 KB data
- **Total: ~12 KB per chart** — trivial even on AD5M
- For comparison: temperature graph uses 300 points × 8 series = ~9.6 KB

### CSV Parsing Memory
- 132 rows × ~10 columns × 4 bytes/float = ~5.3 KB
- String buffer for file read: ~15 KB
- **Total: ~20 KB transient** — freed after populating chart
- Even on AD5M (37 MB free), this is 0.05% of available RAM

### Tier Strategy (via existing `ui_frequency_response_chart`)
- **EMBEDDED (AD5M):** `is_chart_mode = false` → table view only, no chart rendering
  - Show comparison table with all 5 shapers per axis
  - No frequency response graph
  - Memory: essentially zero extra (just the table rows)
- **BASIC (Pi 3):** Simplified chart, max 50 points, no animations
  - Downsampled from 132 → 50 points automatically
  - Still shows peak marker
- **STANDARD (Pi 4+):** Full chart, max 200 points, with animations
  - 132 points < 200 limit, so no downsampling needed
  - Full peak marking, smooth rendering

### Rendering Performance
- Bed mesh 3D visualization: 20+ FPS on Pi 3 with ~90 KB memory (much heavier than our chart)
- Temperature graph: 300 points × 8 series renders smoothly on Pi 3
- Our chart (132 points × 2 series) is lighter than both existing chart uses

---

## Key Data Available

### From InputShaperResult
- `axis` ('X' or 'Y')
- `shaper_type`, `shaper_freq` (recommended)
- `max_accel`, `smoothing`, `vibrations`
- `csv_path` (path to frequency response CSV)
- `freq_response` (vector<pair<float,float>>) — exists but NOT yet populated
- `all_shapers`: vector<ShaperOption> — ALL 5 fitted alternatives

### From ShaperOption
- `type` (zv, mzv, ei, 2hump_ei, 3hump_ei)
- `frequency`, `vibrations`, `smoothing`, `max_accel`

### From InputShaperConfig (current config, queried on panel activate)
- `shaper_type_x`, `shaper_freq_x`
- `shaper_type_y`, `shaper_freq_y`
- `is_configured`

---

## Key Files

| File | Role |
|------|------|
| `ui_xml/input_shaper_panel.xml` | state_results section (lines ~146-210) |
| `src/ui/ui_panel_input_shaper.cpp` | populate_axis_result(), result subjects |
| `include/ui_panel_input_shaper.h` | subjects, MAX_SHAPERS=5 |
| `include/calibration_types.h` | InputShaperResult, ShaperOption, InputShaperConfig |
| `src/api/moonraker_api_advanced.cpp` | InputShaperCollector (state machine) |
| `src/api/moonraker_client_mock.cpp` | timer-based mock dispatch |
| `include/ui_frequency_response_chart.h` | **Existing chart widget** (252 lines) |
| `src/ui/ui_frequency_response_chart.cpp` | **Chart implementation** (602 lines) |
| `tests/unit/test_frequency_response_chart.cpp` | Chart tests |
| `src/calibration/input_shaper_cache.cpp` | freq_response serialization (already done) |
| `include/platform_capabilities.h` | PlatformTier enum, hardware detection |
| `include/helix_plugin_installer.h` | `is_local_moonraker()`, `is_local_host()` |
| `include/moonraker_api.h` | `get_websocket_url()` for locality check |

---

## Implementation Strategy

### Phase 1: Shaper Comparison Table
1. Redesign results XML: scrollable area with per-axis cards
2. Each card shows recommended shaper prominently + 5-row comparison table
3. Table columns: Type, Freq, Vibration%, Max Accel
4. Recommended row highlighted (star icon or color)
5. Quality indicator colors on vibration column
6. Imperative population (5 rows × 4 cols per axis = 40 values, not practical as subjects)

### Phase 2: Frequency Response Chart
1. Detect platform tier via `PlatformCapabilities::detect()`
2. Check locality: parse `get_websocket_url()`, compare to localhost
3. If local + csv_path exists: read CSV with `std::ifstream`, parse into freq/amplitude vectors
4. Populate `freq_response` in `InputShaperResult`
5. Create `ui_frequency_response_chart` in results container
6. Add series: raw psd_xyz + recommended shaper response
7. Mark peak at resonance frequency
8. Cache freq_response data via existing serialization
9. Graceful degradation: if remote or CSV missing, hide chart container

### Phase 3: Before/After Comparison
1. On panel activate, store current `InputShaperConfig` as "before" snapshot
2. If `is_configured` was true, show comparison row: "Previous → New"
3. Only show if shaper actually changed

### Phase 4: Mock Data
1. Generate synthetic CSV file in mock mode (write to /tmp/)
2. Realistic frequency response curve with resonance peak
3. All 5 shaper response curves

---

## XML Centering Lesson
LVGL flex needs ALL THREE properties for centering:
- `style_flex_main_place="center"`
- `style_flex_cross_place="center"`
- `style_flex_track_place="center"` ← REQUIRED even without wrap, for items with explicit widths
