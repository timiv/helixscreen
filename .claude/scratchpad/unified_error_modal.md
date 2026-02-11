# Unified Error Recovery Modal

## Status: TODO (follow-up from PID calibration work, 2026-02-10)

## Problem
Two separate modals for printer errors:
1. "Printer Shutdown" (`klipper_recovery_dialog.xml` / `EmergencyStopOverlay`) — klippy enters SHUTDOWN state
2. "Printer Firmware Disconnected" (`moonraker_manager.cpp` / `NOTIFY_ERROR_MODAL`) — Klipper disconnects from Moonraker

These are two layers of the same problem. During SAVE_CONFIG both fire in sequence. Users don't care about the technical distinction.

## Proposed Design
Single unified modal that adapts buttons based on connection state:
- **Connected + SHUTDOWN**: Restart Klipper, Firmware Restart, Dismiss
- **Disconnected**: Dismiss only (or spinner + "Waiting for reconnection...")
- **State transitions**: Update buttons live if connection drops while SHUTDOWN modal is showing
- **Auto-dismiss**: When klippy returns to READY (already works in recovery dialog)

## Key Files
- `ui_xml/klipper_recovery_dialog.xml` — shutdown modal XML
- `src/ui/ui_emergency_stop.cpp` — shutdown modal logic + klippy_state observer
- `src/application/moonraker_manager.cpp` — disconnect modal via NOTIFY_ERROR_MODAL
- `include/moonraker_events.h` — MoonrakerEventType::KLIPPY_DISCONNECTED
- `include/moonraker_client.h` — suppress_disconnect_modal() API

## Implementation Notes
- Recovery dialog already has the right structure — extend it rather than building from scratch
- Need to track connection state (connected vs disconnected) to enable/disable restart buttons
- Kill the separate NOTIFY_ERROR_MODAL path for KLIPPY_DISCONNECTED
- Suppression mechanism still needed for expected restarts (SAVE_CONFIG) — both modals should respect it

## Session Log (2026-02-10)

### process_lvgl() Infinite Loop Fix (DONE, committed ec140f65)
Fixed the test infrastructure hang where `process_lvgl()` caused infinite loops.

**Root cause**: NOT the UpdateQueue timer chain as originally hypothesized. Actually 13 leaked display refresh timers with stale `last_run=0xFFFFFFDE` timestamps that all became simultaneously "ready", causing LVGL's internal do-while loop to restart endlessly.

**Solution**: `lv_timer_handler_safe()` — pauses ALL timers, calls handler (no-ops), resumes after. UpdateQueue drains manually before handler. Direct `lv_indev_read()` for input processing.

**Files changed**: `ui_update_queue.h` (pause/resume API), `ui_test_utils.h/.cpp` (safe handler + click_at fix), `lvgl_test_fixture.cpp` (process_lvgl rewrite), `test_print_preparation_manager.cpp` (drain helper fix)

### Pre-existing Test Failures (IN PROGRESS)
7 failures to fix:
- test_display_manager (2) - config defaults 0 vs test expects 800x480
- test_ams_backend_mock_realistic (3) - realistic_mode_ defaults true vs test expects false
- test_gcode_object_thumbnail_renderer (1) - pixel byte order issue
- test_moonraker_client_robustness (1) - test not runnable from binary
