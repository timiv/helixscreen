# Auto-Update Feature - Session Continuation Prompt

**Copy everything below and paste as your prompt to continue this work:**

---

## Continue: Auto-Update Feature for HelixScreen - Phase 4+

### Completed Phases

#### Phase 1: SSL Enablement ✅
- Commit: `66b7181f feat(build): enable SSL/TLS on AD5M builds for HTTPS support`
- Added OpenSSL 1.1.1w cross-compilation to `docker/Dockerfile.ad5m`
- Changed `ENABLE_SSL := yes` in `mk/cross.mk` for AD5M
- Updated `mk/deps.mk` with `--with-openssl` for libhv cross-compilation
- Verified: 8.4MB static binary with SSL symbols, deployed to AD5M at 192.168.1.67
- Plan doc: `docs/plans/auto-update-phase1-ssl.md`

#### Phase 2: Update Checker Service ✅
- Commit: `6cab1bac feat(system): add UpdateChecker service for GitHub release checking`
- Files: `include/system/update_checker.h`, `src/system/update_checker.cpp`
- Tests: `tests/unit/test_update_checker.cpp` (14 test cases at time of commit)
- Features: async HTTP to GitHub API, version comparison, rate limiting (1hr), thread-safe, callback API
- Plan doc: `docs/plans/auto-update-phase2-checker.md`

#### Phase 3: Settings UI Integration ✅
- Commit: (pending — uncommitted in worktree)
- Added 4 LVGL subjects to UpdateChecker for declarative UI binding
- Added "Software Update" section to Settings panel (3 XML rows in SYSTEM section)
- Added UpdateChecker lifecycle to application startup/shutdown
- Tests: 16 test cases, 127 assertions — all pass
- Code reviewed: 2 issues found and fixed (buffer overflow risk, thread safety)
- Plan doc: `docs/plans/auto-update-phase3-settings-ui.md`

**Branch:** `feature/update-checker` (worktree at `.worktrees/update-checker`)

---

### What Phase 3 Delivers (Current State)

The Settings panel SYSTEM section now shows:
1. **Current Version** — `setting_info_row` bound to `update_current_version` subject (HELIX_VERSION)
2. **Check for Updates** — `setting_action_row` with `on_check_updates_clicked` callback
3. **Update Status** — `setting_info_row` bound to `update_version_text` subject

UpdateChecker exposes 4 LVGL subjects:
- `update_status` (int) — Status enum (0=Idle, 1=Checking, 2=UpdateAvailable, 3=UpToDate, 4=Error)
- `update_checking` (int) — 1 if checking, 0 otherwise
- `update_version_text` (string) — "Checking...", "Up to date", "v1.2.3 available", "Error: ..."
- `update_new_version` (string) — version number of available update, empty otherwise

Application lifecycle:
- `UpdateChecker::instance().init()` — called in `application.cpp` before panel subjects init
- `UpdateChecker::instance().shutdown()` — called during app shutdown

---

### Remaining Manual Testing (Before Phase 4)

1. Run `./build/bin/helix-screen --test -vv` and navigate to Settings → SYSTEM section
2. Verify "Current Version" shows the correct version
3. Click "Check for Updates" and verify status transitions (Checking... → Up to date / error)
4. Build for AD5M: `make ad5m-docker` — verify it compiles with SSL + subjects
5. Deploy to AD5M: `make deploy-ad5m` — verify real GitHub API check works

---

### What's Next: Phase 4 — Download and Install Flow

**Goal:** When an update is available, allow the user to download and install it.

**UI Requirements:**
1. When `update_status == UpdateAvailable`, show an "Install Update" button (new action row or overlay)
2. Download progress indicator (overlay with progress bar)
3. Confirmation dialog before installing ("Install v1.2.3? Printer will restart.")
4. Error handling for download failures

**Backend Requirements:**
1. Download tarball from `ReleaseInfo::download_url` to `/data/helixscreen-update.tar.gz`
   - AD5M's `/tmp` (tmpfs) is too small for the ~8MB binary
   - `/data/` partition has persistent storage
2. Verify download integrity:
   - gzip integrity check (`gzip -t`)
   - Size sanity check (reject < 1MB or > 50MB)
   - Optional: checksum verification if we add checksums to releases
3. Execute update: `/opt/helixscreen/install.sh --local <tarball> --update`
4. Show progress overlay during install
5. Restart HelixScreen after install completes

**New subjects needed:**
- `update_download_progress` (int) — 0-100 percentage
- `update_download_status` (int) — enum: Idle, Downloading, Verifying, Installing, Complete, Error

**Files to modify/create:**
- `include/system/update_checker.h` — Add download methods and subjects
- `src/system/update_checker.cpp` — Download logic with progress tracking
- `ui_xml/settings_panel.xml` — Add conditional "Install Update" row (bind_flag_if_eq on update_status)
- OR create `ui_xml/components/update_overlay.xml` — Dedicated overlay for download progress
- `src/ui/ui_panel_settings.cpp` — Register install callback

**Implementation approach:**
- Use libhv's HTTP client for download (same as update check)
- Track download progress via content-length / bytes-received
- Parse binary asset from `ReleaseInfo::download_url`
- Release asset naming convention needs to be defined (e.g., `helixscreen-ad5m-v1.2.3.tar.gz`)

---

### Phase 5: Safety Guards (After Phase 4)

1. **Block updates during print** — Check `printer_state.is_printing()` before download/install
2. **User confirmation required** — Modal dialog before install
3. **Rollback capability** — Keep previous binary, restore if install fails
4. **Atomic update** — Use install.sh's atomic replacement (already supported)
5. **Version pinning** — Optional: allow user to skip/dismiss specific versions

---

### UpdateChecker Full API Reference

```cpp
#include "system/update_checker.h"

auto& checker = UpdateChecker::instance();

// Lifecycle
checker.init();      // Call once at startup (idempotent)
checker.shutdown();  // Call before exit (idempotent)

// Check for updates (async, result on LVGL thread)
checker.check_for_updates([](UpdateChecker::Status status,
                             std::optional<UpdateChecker::ReleaseInfo> info) {
    switch (status) {
        case UpdateChecker::Status::UpdateAvailable:
            spdlog::info("Update available: {}", info->version);
            break;
        case UpdateChecker::Status::UpToDate:
            spdlog::info("Already up to date");
            break;
        case UpdateChecker::Status::Error:
            spdlog::warn("Check failed: {}", checker.get_error_message());
            break;
    }
});

// Thread-safe getters
UpdateChecker::Status status = checker.get_status();
bool has_update = checker.has_update_available();
auto info = checker.get_cached_update();  // std::optional<ReleaseInfo>
std::string error = checker.get_error_message();

// Cache control
checker.clear_cache();  // Force fresh check on next call

// LVGL subjects for XML binding
lv_subject_t* s1 = checker.status_subject();        // int: Status enum
lv_subject_t* s2 = checker.checking_subject();       // int: 0/1
lv_subject_t* s3 = checker.version_text_subject();   // string: status text
lv_subject_t* s4 = checker.new_version_subject();    // string: version or ""
```

**ReleaseInfo struct:**
```cpp
struct ReleaseInfo {
    std::string version;       // e.g., "1.2.3" (stripped of 'v' prefix)
    std::string tag_name;      // e.g., "v1.2.3" (original tag)
    std::string download_url;  // Asset URL for .tar.gz
    std::string release_notes; // Body markdown
    std::string published_at;  // ISO 8601 timestamp
};
```

**Status enum:**
```cpp
enum class Status {
    Idle = 0,            // No check in progress
    Checking = 1,        // HTTP request pending
    UpdateAvailable = 2, // New version found
    UpToDate = 3,        // Already on latest
    Error = 4            // Check failed
};
```

---

### Build & Test Commands

```bash
# Working in worktree
cd /Users/pbrown/Code/Printing/helixscreen/.worktrees/update-checker

# Build for local testing
make -j

# Run update checker tests
./build/bin/helix-tests "[update_checker]"

# Run with debug logging
./build/bin/helix-screen --test -vv

# Build for AD5M
make ad5m-docker

# Deploy to test device
make deploy-ad5m AD5M_HOST=192.168.1.67

# Check logs on AD5M
ssh root@192.168.1.67 'tail -f /var/log/messages | grep helix'
```

---

### Key Technical Context

**Thread safety pattern (UpdateChecker uses this):**
```cpp
#include "ui_update_queue.h"

// From background thread:
ui_queue_update([this, result]() {
    // Safe to update LVGL subjects here
    lv_subject_set_int(&status_subject_, static_cast<int>(status));
});
```

**Subject registration (UpdateChecker uses UI_MANAGED_SUBJECT macros):**
```cpp
#include "subject_managed_panel.h"

UI_MANAGED_SUBJECT_INT(status_subject_, 0, "update_status", subjects_);
UI_MANAGED_SUBJECT_STRING(version_text_subject_, version_text_buf_, "", "update_version_text", subjects_);
```

**LVGL subject binding in XML:**
```xml
<setting_info_row label="Update Status" icon="sysinfo">
  <text_muted name="value" bind_text="update_version_text"/>
</setting_info_row>
```

**Event callback registration (BEFORE XML parsing):**
```cpp
lv_xml_register_event_cb(nullptr, "on_check_updates_clicked", on_check_updates_clicked);
```

---

### Project Patterns to Follow

1. **SPDX headers:** `// SPDX-License-Identifier: GPL-3.0-or-later`
2. **Logging:** `spdlog::info()`, `spdlog::debug()`, etc. — NOT printf/cout
3. **JSON:** `#include "hv/json.hpp"` — NOT `<nlohmann/json.hpp>`
4. **Threading:** `ui_queue_update()` for LVGL thread dispatch — NEVER direct `lv_subject_set_*()` from background
5. **Subjects:** `UI_MANAGED_SUBJECT_*` macros + `SubjectManager` for RAII cleanup
6. **XML callbacks:** `lv_xml_register_event_cb()` — NEVER `lv_obj_add_event_cb()`
7. **Design tokens:** `#space_lg`, `#text`, `#card_bg` — NEVER hardcoded values
8. **Settings rows:** `setting_info_row`, `setting_action_row`, `setting_toggle_row` — reusable components

---

### Worktree Location

```
/Users/pbrown/Code/Printing/helixscreen/.worktrees/update-checker
```

Branch: `feature/update-checker`

```bash
cd /Users/pbrown/Code/Printing/helixscreen/.worktrees/update-checker
git log --oneline -5  # See recent commits
```

---

### Plan Documents

| Phase | Doc | Status |
|-------|-----|--------|
| 1 - SSL | `docs/plans/auto-update-phase1-ssl.md` | ✅ Complete |
| 2 - Checker | `docs/plans/auto-update-phase2-checker.md` | ✅ Complete |
| 3 - Settings UI | `docs/plans/auto-update-phase3-settings-ui.md` | ✅ Complete |
| 4 - Download | (not yet created) | Not started |
| 5 - Safety | (not yet created) | Not started |

---

### Begin Phase 4

1. Read `docs/plans/auto-update-phase3-settings-ui.md` for Phase 3 details
2. Define release asset naming convention (needed for download URL matching)
3. Design download/install overlay UI
4. Write plan to `docs/plans/auto-update-phase4-download.md` before implementing
5. Implement with TDD: tests first, then implementation, code review before commit
