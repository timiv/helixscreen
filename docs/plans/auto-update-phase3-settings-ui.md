# Phase 3: Settings UI Integration

**Status**: COMPLETE
**Depends on**: Phase 2 (Update Checker Service) - COMPLETE
**Estimated complexity**: Medium (4 files modified, 1 XML addition, follows existing patterns)

## Goal

Add LVGL subjects to UpdateChecker and integrate a "Software Update" section into the Settings panel so users can see their current version, check for updates, and view status.

## What Was Implemented

### LVGL Subjects on UpdateChecker

Added 4 subjects for XML binding, managed by `SubjectManager` for RAII cleanup:

| Subject Name | Type | Purpose |
|-------------|------|---------|
| `update_status` | int | Status enum value (0=Idle, 1=Checking, 2=UpdateAvailable, 3=UpToDate, 4=Error) |
| `update_checking` | int | Boolean flag (0/1) for checking in progress |
| `update_version_text` | string | Human-readable status ("Checking...", "Up to date", "v1.2.3 available", "Error: ...") |
| `update_new_version` | string | Version number of available update, empty otherwise |

Thread safety:
- `check_for_updates()` wraps subject updates in `ui_queue_update()` (public API, could be called from any thread)
- `report_result()` dispatches subject updates + callbacks via `ui_queue_update()` to LVGL thread
- `init_subjects()` called from `init()` which runs on main thread during startup

### Settings Panel XML

Added 3 rows to the SYSTEM section of `settings_panel.xml`:
1. **Current Version** (`setting_info_row`) — bound to `update_current_version` subject
2. **Check for Updates** (`setting_action_row`) — triggers `on_check_updates_clicked` callback
3. **Update Status** (`setting_info_row`) — bound to `update_version_text` subject

### Application Lifecycle

- `UpdateChecker::instance().init()` called before panel subjects init (subjects must exist for XML binding)
- `UpdateChecker::instance().shutdown()` called during app shutdown

### Tests

Added 2 test cases to `tests/unit/test_update_checker.cpp`:
- Subject initialization: verifies all 4 accessors return non-null, correct initial values
- Subject validity after shutdown: verifies pointers remain stable

Total: 16 test cases, 127 assertions — all pass.

## Files Modified

| File | Changes |
|------|---------|
| `include/system/update_checker.h` | +24 lines: subject members, accessors, SubjectManager, init_subjects() |
| `src/system/update_checker.cpp` | +78 lines: init_subjects(), accessors, subject updates in check/report, cleanup in shutdown |
| `include/ui_panel_settings.h` | +4 lines: update_current_version_subject_ + buffer |
| `src/ui/ui_panel_settings.cpp` | +13 lines: callback registration, current version subject |
| `src/application/application.cpp` | +7 lines: UpdateChecker init() and shutdown() calls |
| `ui_xml/settings_panel.xml` | +10 lines: 3 new setting rows in SYSTEM section |
| `tests/unit/test_update_checker.cpp` | +56 lines: 2 new subject test cases |

## Code Review

Completed with 2 issues found and fixed:
1. **Buffer overflow risk** — `version_text_buf_` increased from 128 to 256 bytes for long error messages
2. **Thread safety** — Subject updates in `check_for_updates()` wrapped in `ui_queue_update()` since it's a public API callable from any thread

## Checklist

- [x] Add LVGL subjects to `UpdateChecker` class
- [x] Add subject accessors (status, checking, version_text, new_version)
- [x] Initialize subjects in `init()` via `UI_MANAGED_SUBJECT_*` macros
- [x] Update subjects in `report_result()` via `ui_queue_update()`
- [x] Update subjects in `check_for_updates()` via `ui_queue_update()`
- [x] Clean up subjects in `shutdown()` via `SubjectManager::deinit_all()`
- [x] Add `update_current_version` subject to SettingsPanel
- [x] Register `on_check_updates_clicked` callback
- [x] Add 3 rows to `settings_panel.xml` SYSTEM section
- [x] Add `UpdateChecker::init()` to application startup
- [x] Add `UpdateChecker::shutdown()` to application shutdown
- [x] Write subject tests (TDD — tests written before implementation)
- [x] Code review passed (2 issues fixed)
- [x] All 16 test cases pass (127 assertions)
- [ ] Manual UI testing with `--test -vv`
- [ ] AD5M build and deploy verification
