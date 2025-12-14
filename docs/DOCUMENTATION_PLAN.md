# Documentation Overhaul Plan

**Created:** 2025-12-14
**Status:** In Progress
**Priority:** End-user documentation for beta launch

---

## Executive Summary

HelixScreen documentation is currently developer-focused. For beta launch with pre-built packages, we need end-user documentation that targets **Klipper users who know their printers but don't build source code**.

**Target Platforms:**
- MainsailOS (Raspberry Pi 3/4/5, BTT Pad 7)
- Adventurer 5M/Pro (FlashForge with Klipper mod)

---

## Phase 1: End-User Documentation (Priority)

Create new documentation in `docs/` for beta testers.

### 1.1 INSTALL.md - Installation Guide

**Purpose:** Get HelixScreen running on your printer display

**Sections:**
```markdown
# Installation Guide

## Quick Start (5 minutes)
- Download pre-built package
- One-line install command
- First boot

## MainsailOS Installation
### Prerequisites
- MainsailOS with working Klipper/Moonraker
- SSH access to your Pi
- Display connected (HDMI, DSI, or SPI)

### Step-by-Step Installation
1. Download the latest release
2. SSH into your Pi
3. Run the installer script
4. Configure display (auto-detected if possible)
5. Start HelixScreen service
6. Access first-run wizard

### Display Configuration
- HDMI displays (plug and play)
- Official Pi touchscreen (DSI)
- SPI displays (requires config)
- BTT Pad 7 / similar

### Starting on Boot
- systemd service setup
- Auto-start configuration
- Log viewing

## Adventurer 5M/Pro Installation
### Prerequisites
- AD5M with Klipper mod installed
- SSH access
- Understanding of firmware risks

### Step-by-Step Installation
1. Backup existing UI (if any)
2. Download AD5M package
3. SSH and extract to /opt/helixscreen
4. Configure autostart
5. Reboot and run wizard

### Display Notes
- Built-in 4.3" touchscreen (800x480)
- Touch input via /dev/input/event4
- Framebuffer rendering

## Updating HelixScreen
- Check current version
- Download update
- Apply update (preserves config)

## Uninstalling
- Stop service
- Remove files
- Restore previous UI (if applicable)
```

**Estimated Size:** 3-4K words

---

### 1.2 USER_GUIDE.md - Using HelixScreen

**Purpose:** How to operate the touchscreen UI

**Sections:**
```markdown
# User Guide

## First-Run Wizard
(Screenshots for each step)
1. WiFi Setup - connect to your network
2. Moonraker Connection - find your printer
3. Printer Identification - verify model/settings
4. Component Selection - heaters, fans, LEDs
5. Summary - review and confirm

## Navigation Overview
- Navigation bar (left side)
- Panel descriptions
- Back button behavior
- Gestures supported

## Home Panel
- Printer status at a glance
- Temperature displays
- AMS/filament status
- Quick actions

## Starting a Print
1. Navigate to Print Select
2. Browse files (card/list view)
3. Preview G-code
4. Pre-print options
5. Start print

## During a Print
- Print status panel
- Progress and time remaining
- Pause/Resume/Cancel
- Z-offset adjustment
- Exclude object

## Controls Panel
- Motion controls (jogging)
- Temperature controls
- Extrusion panel
- Fan controls
- Bed mesh

## Settings
### Display Settings
- Theme (Dark/Light)
- Brightness
- Sleep timeout
- Scroll sensitivity

### Sound Settings
- Enable/disable sounds
- Test beep

### Network Settings
- WiFi configuration
- Moonraker connection

### Safety Settings
- E-Stop confirmation

### Calibration Tools
- Bed mesh
- Z-offset
- PID tuning

## Advanced Features
- Macro panel
- Console (G-code history)
- Power device control
- Print history
- Timelapse settings

## Tips & Tricks
- Keyboard shortcuts (S for screenshot)
- Long-press behaviors
- Hidden features
```

**Estimated Size:** 4-5K words with screenshots

---

### 1.3 TROUBLESHOOTING.md - Common Problems

**Purpose:** Fix issues without needing to ask for help

**Sections:**
```markdown
# Troubleshooting

## Connection Issues

### "Cannot connect to Moonraker"
**Symptoms:** Red connection error, can't control printer
**Causes:**
- Wrong IP address in config
- Moonraker not running
- Firewall blocking connection

**Solutions:**
1. Verify Moonraker is running: `systemctl status moonraker`
2. Check IP address: `hostname -I`
3. Test connection: `curl http://localhost:7125/printer/info`
4. Check helixconfig.json moonraker_host setting

### "Connection lost" during print
**Symptoms:** Disconnect toast, UI shows "Disconnected"
**Causes:**
- Network instability
- Moonraker timeout
- Power management

**Solutions:**
- Check WiFi signal strength
- Use Ethernet if possible
- Disable WiFi power management

### WiFi not connecting
...

## Display Issues

### Black screen on startup
...

### Touch not responding
...

### Wrong screen size/orientation
...

## Print Issues

### Files not appearing
...

### Print won't start
...

### Can't pause/cancel print
...

## Configuration Issues

### First-run wizard keeps appearing
**Solution:** Check helixconfig.json exists and is valid

### Settings not saving
...

### Wrong printer detected
...

## Performance Issues

### UI feels slow/laggy
...

### High memory usage
...

## How to Get Help
1. Check this troubleshooting guide
2. Check GitHub Issues for known problems
3. Gather diagnostic info:
   - HelixScreen version: `helix-screen --version`
   - Logs: `journalctl -t helix -n 100`
   - Config: `cat helixconfig.json`
4. Open a GitHub Issue with:
   - What you expected
   - What happened
   - Steps to reproduce
   - Diagnostic info above
```

**Estimated Size:** 3-4K words

---

### 1.4 CONFIGURATION.md - Settings Reference

**Purpose:** Complete reference for helixconfig.json and in-app settings

**Sections:**
```markdown
# Configuration Reference

## helixconfig.json

### Location
- MainsailOS: `/home/pi/.config/helixscreen/helixconfig.json`
- AD5M: `/opt/helixscreen/helixconfig.json`

### Settings Reference

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `moonraker_host` | string | "localhost" | Moonraker IP or hostname |
| `moonraker_port` | int | 7125 | Moonraker port |
| `dark_mode` | bool | true | Dark theme enabled |
| `brightness` | int | 100 | Display brightness (0-100) |
| `sleep_timeout` | int | 300 | Screen sleep in seconds (0=never) |
| ... | ... | ... | ... |

### Example Configuration
```json
{
  "moonraker_host": "192.168.1.100",
  "moonraker_port": 7125,
  "dark_mode": true,
  "brightness": 80,
  "sleep_timeout": 300,
  ...
}
```

### Resetting Configuration
To reset to defaults, delete helixconfig.json and restart.

## In-App Settings

### Display Settings
...

### Sound Settings
...

(Continue for all settings categories)
```

**Estimated Size:** 2-3K words

---

### 1.5 FAQ.md - Frequently Asked Questions

**Purpose:** Quick answers to common questions

**Sections:**
```markdown
# Frequently Asked Questions

## General

### What is HelixScreen?
HelixScreen is a touchscreen interface for Klipper 3D printers...

### Which printers are supported?
Any printer running Klipper + Moonraker...

### How is this different from KlipperScreen/GuppyScreen?
(Feature comparison table)

### Is HelixScreen production-ready?
Currently in beta...

## Installation

### Can I run this on a Raspberry Pi Zero?
Pi Zero 2 W is recommended minimum...

### Does this work with Fluidd/Mainsail?
Yes, it connects to Moonraker...

### Can I run HelixScreen alongside KlipperScreen?
Not on the same display...

## Features

### Does it support multiple extruders?
...

### Can I use my webcam?
Camera support coming soon...

### Does it work with Spoolman?
Coming in a future update...

## Troubleshooting

### My settings keep resetting
...

### The UI is too small/large
...

(Continue with common questions)
```

**Estimated Size:** 2K words

---

## Phase 2: Consolidation & Cleanup

After end-user docs are complete, clean up developer documentation.

### 2.1 Archive Completed Plans

Move to `docs/archive/`:
- `FEATURE_STATUS.md` (superseded by ROADMAP.md)
- `DECLARATIVE_UI_REFACTORING.md` (complete)
- `INPUT_SHAPER_HANDOFF.md` (handoff complete)
- `LOG_LEVEL_REFACTOR_PLAN.md` (duplicate in archive)
- `GCODE_MONITORING_TESTING.md` (testing complete)
- `HELIX_PRINT_PLUGIN_TESTING.md` (testing complete)
- `WIZARD_TEST_PLAN.md` (wizard complete)

### 2.2 Consolidate Testing Documentation

Merge into single comprehensive `TESTING.md`:
- Keep: `TESTING.md` as base
- Merge: `RUNNING_TESTS.md` content
- Merge: `UI_TESTING.md` content
- Archive: `TESTING_DEBT.md` (after review)
- Archive: `TESTING_MOONRAKER_API.md`
- Archive: `TESTING_PRE_PRINT_OPTIONS.md`

### 2.3 Consolidate Moonraker Documentation

Single `MOONRAKER_INTEGRATION.md`:
- Architecture (from MOONRAKER_ARCHITECTURE.md)
- API usage (from audits/*)
- Testing (from TESTING_MOONRAKER_API.md)

### 2.4 LVGL Documentation

Keep:
- `LVGL9_XML_GUIDE.md` - main reference
- `QUICK_REFERENCE.md` - patterns

Archive:
- `LVGL9_XML_CHEATSHEET.html` - redundant with guide
- `LVGL9_XML_ATTRIBUTES_REFERENCE.md` - mostly in guide
- `LV_SIZE_CONTENT_GUIDE.md` - specialized topic

---

## Phase 3: Structure Improvements

### 3.1 Proposed Directory Structure

```
docs/
├── README.md                    # Index/overview of documentation
│
├── user/                        # END-USER DOCUMENTATION
│   ├── INSTALL.md               # Installation guide
│   ├── USER_GUIDE.md            # How to use HelixScreen
│   ├── CONFIGURATION.md         # Settings reference
│   ├── TROUBLESHOOTING.md       # Common problems
│   └── FAQ.md                   # Frequently asked questions
│
├── developer/                   # DEVELOPER DOCUMENTATION
│   ├── GETTING_STARTED.md       # Quick dev setup
│   ├── ARCHITECTURE.md          # System design
│   ├── CONTRIBUTING.md          # Code standards
│   ├── BUILD_SYSTEM.md          # Build internals
│   └── TESTING.md               # Consolidated testing
│
├── reference/                   # TECHNICAL REFERENCE
│   ├── LVGL9_XML_GUIDE.md       # XML reference
│   ├── QUICK_REFERENCE.md       # Code patterns
│   ├── MOONRAKER_INTEGRATION.md # Moonraker API
│   ├── LOGGING.md               # Logging standards
│   └── COPYRIGHT_HEADERS.md     # License headers
│
├── planning/                    # ACTIVE PLANNING
│   ├── ROADMAP.md               # Feature timeline
│   └── AMS_IMPLEMENTATION_PLAN.md
│
├── audits/                      # SECURITY/QUALITY REVIEWS
│   ├── MEMORY_ANALYSIS.md
│   ├── MOONRAKER_SECURITY_REVIEW.md
│   └── TEST_COVERAGE_REPORT.md
│
├── archive/                     # HISTORICAL DOCUMENTATION
│   └── (completed plans, old docs)
│
└── images/                      # SCREENSHOTS FOR DOCS
```

### 3.2 README.md Index Update

Create `docs/README.md` as documentation index:
```markdown
# HelixScreen Documentation

## For Users
- [Installation Guide](user/INSTALL.md) - Get HelixScreen running
- [User Guide](user/USER_GUIDE.md) - How to use the interface
- [Configuration](user/CONFIGURATION.md) - Settings reference
- [Troubleshooting](user/TROUBLESHOOTING.md) - Fix common issues
- [FAQ](user/FAQ.md) - Quick answers

## For Developers
- [Getting Started](developer/GETTING_STARTED.md) - Dev environment setup
- [Architecture](developer/ARCHITECTURE.md) - System design
- [Contributing](developer/CONTRIBUTING.md) - Code standards
- [Build System](developer/BUILD_SYSTEM.md) - Build internals
- [Testing](developer/TESTING.md) - Test infrastructure

## Reference
- [LVGL XML Guide](reference/LVGL9_XML_GUIDE.md) - Complete XML reference
- [Quick Reference](reference/QUICK_REFERENCE.md) - Common patterns
- [Moonraker Integration](reference/MOONRAKER_INTEGRATION.md) - API docs
```

---

## CLAUDE.md Optimization

### Current Issues
- 11K characters loaded every conversation
- Mix of essential rules and reference patterns
- Some redundancy with docs

### Proposed Changes
1. Keep CRITICAL RULES section
2. Keep VERBOSITY FLAGS (frequently forgotten)
3. Keep FILE ACCESS PERMISSIONS
4. Reduce DECLARATIVE UI PATTERN to summary with link to ARCHITECTURE.md
5. Keep API CHANGES section (LVGL 9.4)
6. Remove/reduce sections that are reference material

### Target Size: ~6-8K characters (vs current 11K)

---

## Implementation Checklist

### Phase 1: End-User Docs
- [ ] Create `docs/user/` directory
- [ ] Write `docs/user/INSTALL.md`
  - [ ] MainsailOS installation section
  - [ ] Adventurer 5M installation section
  - [ ] Display configuration
  - [ ] Starting on boot
  - [ ] Updating/uninstalling
- [ ] Write `docs/user/USER_GUIDE.md`
  - [ ] First-run wizard walkthrough
  - [ ] Navigation overview
  - [ ] Panel descriptions
  - [ ] Screenshot each panel
- [ ] Write `docs/user/TROUBLESHOOTING.md`
  - [ ] Connection issues
  - [ ] Display issues
  - [ ] Print issues
  - [ ] How to get help
- [ ] Write `docs/user/CONFIGURATION.md`
  - [ ] helixconfig.json reference
  - [ ] In-app settings
- [ ] Write `docs/user/FAQ.md`
  - [ ] Expand from README FAQ
  - [ ] Add common beta tester questions

### Phase 2: Consolidation
- [ ] Archive completed plan docs
- [ ] Consolidate testing docs
- [ ] Consolidate Moonraker docs
- [ ] Archive redundant LVGL docs

### Phase 3: Structure
- [ ] Create directory structure
- [ ] Move docs to new locations
- [ ] Update all cross-references
- [ ] Create docs/README.md index
- [ ] Update main README.md links

### Phase 4: CLAUDE.md Optimization
- [ ] Review and slim down
- [ ] Move reference content to docs
- [ ] Test with fresh context

---

## Notes

### Screenshots Needed
- First-run wizard (all 7 steps)
- Home panel (connected, printing, disconnected)
- Controls panel
- Print select panel
- Print status panel
- Settings panel
- Each overlay/modal

### External Links to Add
- MainsailOS documentation
- Klipper documentation
- Moonraker documentation
- GitHub Issues page

---

## Progress Log

### 2025-12-14
- Created documentation plan
- Identified gaps in end-user documentation
- Prioritized Phase 1 (end-user docs for beta)
- Target platforms: MainsailOS + Adventurer 5M
