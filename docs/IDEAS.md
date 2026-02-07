# Ideas & Future Improvements

* More test coverage
* Have libhv use spdlog for logging if possible
* **Lazy panel initialization** - Defer `init_subjects()` and `setup()` until first navigation. Challenge: LVGL XML binding requires subjects to exist before parsing. Solution: register empty subjects at startup, populate on first use. Would reduce startup time and memory.
* Belt tension visualization: controlled belt excitation + stroboscopic LED feedback to visualize resonance
* **LVGL lv_bar value=0 bug** (upstream issue) - Bar shows FULL instead of empty when created with cur_value=0 and XML sets value=0. `lv_bar_set_value()` returns early without invalidation. Workaround: set to 1 then 0.
* Improve filament sensor widget on home screen
* **Controller fan display/control** - Show controller fan status and allow on/off control from the UI
* **Z-axis direction flip toggle** - Settings option to invert Z movement buttons for printers where the heuristic gets it wrong
* **Customizable home screen layout** - Let users configure which widgets/cards appear on the home screen (e.g., hide WiFi if not needed)
* **Custom printer image on home page** - Allow manually choosing the printer picture shown on the home screen, and optionally uploading a custom image to replace the auto-detected one
* **Smart plug control via MQTT** - Turn printer power on/off via controllable plug (Tasmota, etc.)

---

## Deferred: Agent Documentation Compression

Agents (~3,700 lines total) could be compressed ~60% using table format and external examples.
Currently not needed - agents work well. Revisit if startup times degrade or context limits hit.
See git history (2025-11-10) for full analysis.

---

## Design Philosophy: Local vs Remote UI

HelixScreen is a **local touchscreen** - users are physically present at the printer. This fundamentally differs from web UIs (Mainsail/Fluidd) designed for remote monitoring.

**What this means:**
- **Camera** is low priority - you can see the printer with your eyes
- **Job Queue** is not useful - you need to manually remove prints between jobs
- **System stats** (CPU/memory) are less important - you're not diagnosing remote issues
- **Remote access features** don't apply to this form factor

**What we prioritize instead:**
- Tactile controls optimized for touch
- At-a-glance information for the user standing at the machine
- Calibration workflows (PID, Z-offset, screws tilt, input shaper)
- Real-time tuning (speed, flow, firmware retraction)

Don't copy features from web UIs just because "competitors have it" - evaluate whether it makes sense for a local touchscreen.
