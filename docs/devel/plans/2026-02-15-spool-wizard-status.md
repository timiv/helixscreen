# Spool Wizard — Implementation Status

**Date**: 2026-02-16 (updated: graduated from beta, modal refactor complete)
**Branch**: `main` (graduated from beta)
**Phase**: Phase 3 of Spoolman Management design (`2026-02-09-spoolman-management-design.md`)

---

## Branch History

The wizard was originally built on `feature/spool-wizard` branched from `main` (10 commits). This was a mistake — it missed all the Spoolman infrastructure built in `feature/multi-unit-ams` (VendorInfo, FilamentInfo, SpoolInfo types, callback types, existing CRUD methods, SpoolmanPanel overlay, context menu, edit modal, etc.).

`feature/spool-wizard-v2` was created from `feature/multi-unit-ams` HEAD and the wizard-specific files were ported over, adapted to use the v2 types (`FilamentInfo.display_name()` instead of `.name`, `float` types instead of `double`, etc.).

**Both old branches have been deleted.** All wizard work is now merged into `feature/multi-unit-ams`:
- `feature/spool-wizard` — deleted (superseded)
- `feature/spool-wizard-v2` — merged into `feature/multi-unit-ams` at `642e5cea`, then deleted

---

## What's Built

### Key Commits
- `29231b75` — feat(spoolman): add spool creation wizard with 3-step flow (14 files, ~3400 lines)
- `aa1daa95` — fix(spoolman): address P0-P3 review findings in spool wizard
- `642e5cea` — Merge branch 'feature/spool-wizard-v2' into feature/multi-unit-ams

### Files Added (6 new)
| File | Lines | Purpose |
|------|-------|---------|
| `include/ui_spool_wizard.h` | 352 | SpoolWizardOverlay class — 3-step wizard state machine |
| `src/ui/ui_spool_wizard.cpp` | ~1550 | Full wizard implementation: vendor/filament steps, creation chain |
| `tests/unit/test_spool_wizard.cpp` | ~780 | 46 test cases, 144 assertions (all passing) |
| `ui_xml/spool_wizard.xml` | 356 | Full 3-step wizard XML layout with subject bindings |
| `ui_xml/wizard_vendor_row.xml` | 18 | Vendor list row template (name + source badge area) |
| `ui_xml/wizard_filament_row.xml` | 28 | Filament list row (color swatch + material + name + temps) |

### Files Modified (8)
| File | Changes |
|------|---------|
| `include/moonraker_api.h` | +5 new virtual methods (external vendors/filaments, vendor-filtered filaments, delete vendor/filament) |
| `include/moonraker_api_mock.h` | +5 mock overrides |
| `src/api/moonraker_api_advanced.cpp` | +146 lines: implementations via `server.spoolman.proxy` RPC |
| `src/api/moonraker_api_mock.cpp` | +147 lines: mock data for Hatchbox/Polymaker/eSUN/Prusament vendors + PLA/PETG filaments |
| `include/ui_panel_spoolman.h` | +SpoolWizardOverlay include, `wizard_panel_` member, `on_add_spool_clicked` callback |
| `src/ui/ui_panel_spoolman.cpp` | +`on_add_spool_clicked` → lazy_create_and_push_overlay with completion callback for refresh |
| `ui_xml/spoolman_panel.xml` | +action_button_2 ("+ Add" button with primary bg) alongside refresh button |
| `src/xml_registration.cpp` | +5 component registrations (wizard_vendor_row, wizard_filament_row, spool_wizard, create_vendor_modal, create_filament_modal) |

### Files Added (post-merge modal refactor)
| File | Purpose |
|------|---------|
| `ui_xml/create_vendor_modal.xml` | Modal form for creating new vendors (name + URL) |
| `ui_xml/create_filament_modal.xml` | Modal form for creating new filaments (material, name, color, temps, weight) |
| `include/json_utils.h` | Null-safe JSON extraction helpers for Spoolman API responses |
| `tests/unit/test_json_utils.cpp` | 20 test cases for JSON null safety |

### Architecture
- **SpoolWizardOverlay** extends `OverlayBase`, uses `DEFINE_GLOBAL_PANEL` singleton pattern
- **3-step flow**: Vendor (Step 0) → Filament (Step 1) → Spool Details (Step 2)
- **Modal forms**: Vendor/filament creation in modal dialogs (not inline) to maximize list scroll area
- **14 LVGL subjects** drive all UI state (step visibility, button states, labels, counts, loading states)
- **Declarative UI**: Step visibility via `bind_flag_if_not_eq`, button gating via subjects, text via `bind_text`
- **Imperative exceptions**: Dynamic list row population, color swatch styling (both accepted exceptions per CLAUDE.md)
- **Vendor-filtered filaments**: API uses `vendor.id=X` (Spoolman dot-notation) — no client-side filtering
- **Material database**: Filament creation populates dropdown from `filament::MATERIALS[]` (~48 materials)
- **Color picker**: HSV picker + preset swatches, integrated with filament creation modal
- **Atomic creation chain**: vendor → filament → spool with best-effort rollback on failure
- **Thread safety**: All async callbacks wrapped in `ui_queue_update()` for LVGL thread marshaling
- **No beta gating**: Available to all users (graduated 2026-02-16)

### New API Methods
| Method | Endpoint | Purpose |
|--------|----------|---------|
| `get_spoolman_external_vendors()` | GET `/v1/external/vendor` | SpoolmanDB vendor catalog |
| `get_spoolman_external_filaments(vendor_name)` | GET `/v1/external/filament?vendor_name=...` | SpoolmanDB filaments by vendor |
| `get_spoolman_filaments(vendor_id)` | GET `/v1/filament?vendor.id=...` | Server filaments filtered by vendor |
| `delete_spoolman_vendor(id)` | DELETE `/v1/vendor/{id}` | Rollback support |
| `delete_spoolman_filament(id)` | DELETE `/v1/filament/{id}` | Rollback support |

### Test Coverage
- Step navigation (forward, back, boundaries, labels)
- Vendor merge/dedup (server priority, DB-only, case-insensitive)
- Vendor filtering (substring, empty query, no matches)
- Vendor selection (existing + new vendor creation)
- Filament merge/dedup (material+color key, server priority)
- Filament selection (existing + new with material auto-fill)
- Material auto-fill from compiled-in filament_database.h
- Spool details pre-fill and validation
- Create request lifecycle (no-API path, completion callback)
- Edge cases (empty inputs, invalid indices, default state)

---

## Known Bugs — FIXED

All P0-P3 bugs from code review were fixed in `aa1daa95`:

| Priority | Issue | Fix |
|----------|-------|-----|
| ~~P0~~ | No error toast | Added `ui_toast_show(ToastSeverity::ERROR, ...)` in `on_creation_error()` |
| ~~P0~~ | Wizard stays open on success | Added `ui_toast_show(SUCCESS)` + `ui_nav_go_back()` in `on_creation_success()` |
| ~~P0~~ | Silent API response swallow | Added else-branch with `on_error` callback in all 3 create methods |
| ~~P1~~ | Incomplete state reset | Added `reset_state()` that clears ALL wizard state, called from `on_activate()` |
| ~~P1~~ | Creation callbacks on deactivated overlay | Added `is_visible()` guard in all 3 creation chain callbacks |
| ~~P3~~ | No input length validation | Added `MAX_VENDOR_NAME_LEN`/`MAX_VENDOR_URL_LEN` constants + `substr()` clamping |

### Code Quality Notes (non-blocking, deferred)
- Hardcoded colors in 3 places (color swatches — acceptable exception)
- `lv_obj_add_event_cb` for scroll (pre-existing in SpoolmanPanel)
- No test coverage for creation chain (would need mock API injection)
- Inconsistent log tag format (static callbacks use `[SpoolWizard]`, instance methods use `get_name()`)

---

## What's NOT Built Yet

### Remaining Plan Steps

| Step | Status | Description |
|------|--------|-------------|
| Step 9: filamentcolors.xyz | **NOT STARTED** | Optional network color enhancement — query `filamentcolors.xyz/api/swatch/` for better color names, in-memory cache, graceful degradation |
| Creation chain tests | **NOT STARTED** | Mock API injection to test vendor→filament→spool chain, partial failure rollback |

### Visual Test Plan (NOT STARTED)

All 10 visual tests require running `./build/bin/helix-screen --test -vv` and manually verifying UI behavior.

#### VT-1: Launch Wizard from SpoolmanPanel
1. Navigate to Spoolman panel overlay
2. Verify "+ Add" button visible in header bar (alongside refresh button)
3. Tap "+ Add" button
4. Verify wizard overlay slides in showing "New Spool: Step 1 of 3" in header
5. Verify Next button is disabled (no vendor selected)
6. Verify Back button is hidden on step 1

#### VT-2: Vendor Search and Selection
1. On Step 1, verify vendor list populates (mock: Hatchbox, Polymaker, eSUN, Prusament from external + server vendors merged)
2. Type "hatch" in search box — verify list filters to matching vendors
3. Tap a vendor row — verify highlight/selection, Next button enables
4. Clear search — verify full list returns
5. Tap "+ New" — verify vendor creation modal opens

#### VT-3: Create New Vendor Flow
1. Tap "+ New" button next to search box
2. Verify modal dialog opens with name and URL fields
3. Fill in vendor name (e.g., "Test Vendor Co")
4. Verify "Add Vendor" button enables when name non-empty
5. Tap "Add Vendor" — verify modal closes, vendor appears in list highlighted
6. Tap Next to proceed to Step 2

#### VT-4: Filament Step with Existing Vendor
1. After selecting existing vendor with filaments
2. Step 2 shows: selected vendor name at top, filament list with color swatches + material + temp range
3. Tap a filament row — verify selection, Next button enables
4. Verify Back goes to Step 1 (vendor selection preserved)

#### VT-5: Create New Filament with Material Auto-Fill
1. On Step 2, tap "+ New" button next to vendor name
2. Verify modal opens with material dropdown (~48 materials), name, color, temp ranges, weight fields
3. Select material from dropdown (e.g., ABS) — verify temp fields auto-fill from filament database
4. Tap "Pick" color button — verify ColorPicker modal opens (stacked on top of filament modal)
5. Pick a color — verify swatch updates in filament modal
6. Tap "Add Filament" — verify modal closes, filament appears in list highlighted
7. Verify validation: tapping "Add Filament" without material/color shows error highlighting on labels

#### VT-6: Spool Details Step and Pre-Fill
1. After selecting filament, tap Next to Step 3
2. Verify summary card shows vendor name + filament info
3. Verify summary color swatch shows selected filament color
4. Verify remaining weight pre-filled from filament net weight
5. Verify price, lot, notes fields are editable
6. Verify "Create Spool" button is enabled (weight > 0)

#### VT-7: Full Creation Flow (Existing Vendor + Existing Filament)
1. Select existing vendor → select existing filament → fill spool details → "Create Spool"
2. Verify creating spinner appears
3. Verify wizard closes on success with success toast
4. Verify SpoolmanPanel refreshes spool list
5. Verify new spool appears in list

#### VT-8: Full Creation Flow (New Vendor + New Filament)
1. Create new vendor → create new filament (custom material/color/temps) → fill spool details → "Create Spool"
2. Verify atomic creation chain: vendor created, then filament, then spool
3. Verify spinner shown during creation
4. Verify success closes wizard with toast

#### VT-9: Back Navigation Preserves State
1. Fill Step 1 (select vendor), go to Step 2, Back to Step 1 — vendor still selected
2. From Step 2, go to Step 3, Back — filament selection preserved
3. From Step 1, Back should close/pop the wizard overlay

#### VT-10: Error Handling and Edge Cases
1. Empty vendor list (no Spoolman configured) — verify empty state or helpful message
2. Vendor with no filaments — verify empty filament list with create option visible
3. Clear remaining weight to 0 — verify Create button disables
4. Simulate network error mid-creation — verify spinner stops, error toast shown

---

## Design Decisions Made

| Decision | Rationale |
|----------|-----------|
| Panel-width overlay (not modal) | Consistent with SpoolmanPanel pattern; more room for lists |
| 3-step wizard (not single form) | Reduces cognitive load; allows vendor/filament reuse |
| SpoolmanDB via external API | Spoolman server already has `/v1/external/vendor` and `/v1/external/filament` endpoints — no need to bundle the 4MB JSON |
| Dual-source merge | Shows both what's already on server AND what's available in the database |
| Atomic creation with rollback | If filament creation fails after vendor was created, attempt to delete the vendor |
| `DEFINE_GLOBAL_PANEL` singleton | Consistent with all other overlays; static callbacks access global instance |
| Subject-driven step visibility | XML `bind_flag_if_not_eq` on step index — no imperative `lv_obj_add_flag(HIDDEN)` |
| Beta feature gating | "+" button reactively hidden via `show_beta_features` subject + C++ guard in handler |

---

## Future Work (Beyond Current Scope)

From the original Spoolman Management design (`2026-02-09`):

| Feature | Status |
|---------|--------|
| Spool CRUD (browse, edit, delete) | **Done** (in multi-unit-ams, SpoolmanPanel) |
| New Spool wizard | **Done** (merged into multi-unit-ams, gated behind beta features) |
| QR code label printing | **Not started** |
| USB barcode scanner input | **Not started** |
| Shared reusable components | **Done** (ams_unit_detail, ams_loaded_card, etc.) |

---

## How to Test

```bash
# On feature/multi-unit-ams branch
make -j

# Run wizard unit tests
./build/bin/helix-tests "[spool_wizard]"
# Expected: 144 assertions in 46 test cases — all pass

# Visual testing (mock printer mode)
./build/bin/helix-screen --test -vv
# Navigate: Home → Filament → Spoolman → tap "+" button
# Note: "+" button only visible with beta features enabled (7-tap version in Settings)
```

---

## File Map

```
include/
  ui_spool_wizard.h          ← Wizard class (352 lines)
  moonraker_api.h            ← +5 methods
  moonraker_api_mock.h       ← +5 mock overrides
  ui_panel_spoolman.h        ← +wizard_panel_ member, +on_add_spool_clicked

src/
  ui/ui_spool_wizard.cpp     ← Wizard implementation (~1550 lines)
  ui/ui_panel_spoolman.cpp   ← +on_add_spool_clicked callback, beta gating
  api/moonraker_api_advanced.cpp  ← +5 API implementations
  api/moonraker_api_mock.cpp      ← +5 mock implementations
  xml_registration.cpp       ← +3 component registrations

ui_xml/
  spool_wizard.xml           ← 3-step wizard layout (356 lines)
  wizard_vendor_row.xml      ← Vendor row template
  wizard_filament_row.xml    ← Filament row template
  spoolman_panel.xml         ← +action_button_2 ("+" button)

tests/
  unit/test_spool_wizard.cpp ← 46 tests, 144 assertions
```
