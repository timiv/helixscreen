# HelixScreen Feature Parity Research & Implementation Guide

**Created:** 2025-12-08
**Purpose:** Comprehensive research document for making HelixScreen the definitive Klipper touchscreen UI
**Scope:** Moonraker API, competitor analysis, extensions, community feedback, implementation details

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current State Assessment](#current-state-assessment)
3. [Competitor Deep Dive](#competitor-deep-dive)
4. [Moonraker API Complete Reference](#moonraker-api-complete-reference)
5. [Feature Gap Analysis](#feature-gap-analysis)
6. [Klipper Extensions Integration](#klipper-extensions-integration)
7. [Community Pain Points & Requests](#community-pain-points--requests)
8. [Implementation Specifications](#implementation-specifications)
9. [UI/UX Considerations](#uiux-considerations)
10. [Code Patterns & Templates](#code-patterns--templates)
11. [Testing Strategy](#testing-strategy)
12. [Future Ideas & Possibilities](#future-ideas--possibilities)

---

# Executive Summary

## Mission
Make HelixScreen the **definitive** Klipper touchscreen UI by achieving feature parity with all competitors while adding unique differentiating features that no one else offers.

## Target Configuration
- **Screen Size:** 5-7" (medium) as primary, with responsive support for small (3.5-5") and large (7"+)
- **Printer Types:** CoreXY (Voron) and Multi-material (ERCF, Happy Hare, AFC) as priorities
- **Competition:** Beat KlipperScreen, match Mainsail/Fluidd touchscreen-appropriate features, achieve Bambu Lab polish

## Key Findings

### What We Excel At ✅
- Native C++/LVGL performance (no Python/Xorg lag)
- Responsive design system with breakpoints
- Modern reactive architecture (Subject-Observer)
- Declarative XML layouts
- Solid Moonraker integration foundation (59 methods)
- First-run wizard experience
- WiFi configuration
- LED control
- Exclude object support

### Critical Gaps ❌
1. Temperature Presets - Every competitor has this
2. Macro Panel - Cannot run macros from touchscreen
3. Console Panel - No G-code terminal
4. Screws Tilt Adjust - KlipperScreen's killer feature
5. Camera/Webcam - Zero integration
6. Print History - No job tracking
7. Power Device Control - Cannot control Moonraker power devices

### Unique Opportunities 🚀
1. PID Tuning UI - No touchscreen has this
2. Pressure Advance UI - Console-only everywhere
3. First-Layer Calibration Wizard - No guided workflow exists
4. Material Database - Built-in filament presets
5. Maintenance Tracker - Track nozzle changes, belt tension, etc.

---

# Current State Assessment

## Panel Inventory (14 Production + 6 Overlays)

### Main Navigation Panels
| Panel | File | Status | Features |
|-------|------|--------|----------|
| Home | `home_panel.xml` | ✅ Complete | Status cards, temps, LED, quick actions |
| Controls | `controls_panel.xml` | ✅ Complete | 6-card launcher |
| Motion | `motion_panel.xml` | ✅ Complete | Jog pad, Z-axis, distance, homing |
| Nozzle Temp | `nozzle_temp_panel.xml` | ✅ Complete | Presets, graph, custom entry |
| Bed Temp | `bed_temp_panel.xml` | ✅ Complete | Presets, graph, custom entry |
| Extrusion | `extrusion_panel.xml` | ✅ Complete | Extrude/retract, amount, speed |
| Filament | `filament_panel.xml` | ✅ Complete | Load/unload, profiles |
| Fan | `fan_panel.xml` | ✅ Complete | Multi-fan control |
| Print Select | `print_select_panel.xml` | ✅ Complete | Grid/list, sort, USB tabs |
| Print Status | `print_status_panel.xml` | ✅ Complete | Progress, time, pause/resume/cancel |
| Settings | `settings_panel.xml` | ✅ Complete | 18 settings, 8 categories |
| Advanced | `advanced_panel.xml` | ✅ Complete | Bed mesh 3D visualization |
| Calibration Z-Offset | `calibration_zoffset_panel.xml` | ✅ Complete | Z-offset adjustment |
| Calibration PID | `calibration_pid_panel.xml` | ✅ Complete | PID tuning |

### Overlays & Modals
| Overlay | File | Purpose |
|---------|------|---------|
| WiFi Settings | `wifi_settings_overlay.xml` | Network scan/connect |
| Network Settings | `network_settings_overlay.xml` | IP configuration |
| Display Settings | `display_settings_overlay.xml` | Brightness, rotation |
| Print File Detail | `print_file_detail.xml` | File metadata, start print |
| Numeric Keypad | `numeric_keypad_panel.xml` | Number input |
| E-Stop Confirmation | `estop_confirmation_dialog.xml` | E-stop guard |

### First-Run Wizard (7 Steps)
1. WiFi Setup (`wizard_wifi_setup.xml`)
2. Moonraker Connection (`wizard_connection.xml`)
3. Printer Identification (`wizard_printer_identify.xml`)
4. Heater Selection (`wizard_heater_select.xml`)
5. Fan Selection (`wizard_fan_select.xml`)
6. LED Selection (`wizard_led_select.xml`)
7. Summary (`wizard_summary.xml`)

## Moonraker Integration Status

### Currently Implemented (59 Methods in moonraker_api.h)

**File Operations:**
- `list_files()` - List G-code files with pagination
- `get_file_metadata()` - Slicer info, temps, thumbnails
- `delete_file()` - Remove files
- `move_file()` / `copy_file()` - File management
- `create_directory()` / `delete_directory()` - Folder management
- `upload_file()` - HTTP multipart upload

**Print Control:**
- `start_print()` - Begin printing
- `pause_print()` - Pause current job
- `resume_print()` - Resume paused job
- `cancel_print()` - Cancel job
- `exclude_object()` - Mid-print object cancellation

**Motion Control:**
- `home_axes()` - Home X, Y, Z, or all
- `move_axis()` - Relative movement with feedrate
- `move_to_position()` - Absolute positioning
- `set_speed_factor()` - Override print speed
- `set_flow_factor()` - Override extrusion

**Temperature/Fan/LED:**
- `set_heater_temperature()` - Set target temp
- `set_fan_speed()` - Control fans
- `set_led_color()` - RGB + white channel
- `led_on()` / `led_off()` - Quick toggles

**System:**
- `emergency_stop()` - E-stop
- `restart_firmware()` - Full MCU restart
- `restart_klipper()` - Soft restart
- `run_gcode()` - Execute raw G-code

**State Queries:**
- `is_printer_ready()` - Check state
- `get_print_state()` - Current job status
- `get_excluded_objects()` - Exclusion list
- `get_available_objects()` - All print objects

### NOT Implemented (Need to Add)

| Category | Endpoints | Priority |
|----------|-----------|----------|
| Job Queue | 6 endpoints | HIGH |
| Print History | 4 endpoints | CRITICAL |
| Webcam | 5 endpoints | CRITICAL |
| Power Devices | 4 endpoints | HIGH |
| Update Manager | 5 endpoints | HIGH |
| Spoolman | 3 endpoints | HIGH |
| GCode Store | 1 endpoint | CRITICAL |
| Temperature Store | 1 endpoint | MEDIUM |
| System Info | 3 endpoints | MEDIUM |
| Sensors | 2 endpoints | LOW |

## Settings Inventory (18 Total)

### Appearance
- Dark/Light theme toggle
- Display brightness (0-100%, hardware sync)
- Display rotation (0°, 90°, 180°, 270°)
- Display sleep timeout (30s - 30min)

### Input
- Scroll momentum (1-99 decay rate)
- Scroll sensitivity (pixel threshold)

### Sound & Notifications
- Sound toggle (with speaker detection)
- M300 beep test
- Print completion mode (Off/Notification/Alert)

### Printer-Specific
- LED light control (capability-aware)
- E-Stop confirmation requirement

### Calibration Launchers
- Z-offset calibration
- PID tuning
- Bed mesh visualization

### Network
- WiFi SSID/password
- Moonraker IP/port/API key

### System
- Factory reset
- Log level (via command line)

---

# Competitor Deep Dive

## KlipperScreen (Primary Touchscreen Competitor)

### Overview
- **Technology:** Python + GTK/SDL
- **Target:** Raspberry Pi touchscreens
- **Repo:** https://github.com/KlipperScreen/KlipperScreen
- **Docs:** https://klipperscreen.readthedocs.io/

### Complete Panel List
| Panel | Description | HelixScreen Has? |
|-------|-------------|------------------|
| `job_status` | Active print monitoring | ✅ Yes |
| `main` | Home dashboard | ✅ Yes |
| `bed_level` | **Screws tilt adjust visual** | ❌ **MISSING** |
| `bed_mesh` | Mesh visualization | ✅ Yes |
| `console` | G-code terminal | ❌ MISSING |
| `extrude` | Filament extrusion | ✅ Yes |
| `fan` | Fan speed control | ✅ Yes |
| `fine_tune` | Speed/flow during print | ⚠️ Partial |
| `gcode_macros` | Macro execution | ❌ MISSING |
| `input_shaper` | Resonance calibration | ❌ MISSING |
| `limits` | Velocity/accel limits | ❌ MISSING |
| `move` | Motion jog controls | ✅ Yes |
| `network` | WiFi configuration | ✅ Yes |
| `notifications` | System notifications | ✅ Yes (toasts) |
| `power` | Device power control | ❌ MISSING |
| `print` | File browser | ✅ Yes |
| `retraction` | Firmware retraction | ❌ MISSING |
| `settings` | Configuration | ✅ Yes |
| `spoolman` | Filament tracking | ❌ MISSING |
| `splash` | Boot screen | ✅ Yes |
| `system` | System info/restart | ⚠️ Partial |
| `temperature` | Temp control | ✅ Yes |
| `zcalibrate` | Z-offset calibration | ✅ Yes |
| `updater` | Software updates | ❌ MISSING |

### KlipperScreen Killer Features We Must Match

#### 1. Screws Tilt Adjust Visual Panel
This is KlipperScreen's most praised feature. It shows:
- Visual bed diagram with screw positions
- Support for up to 9 screws (3x3 grid)
- Rotation indicators ("turn CW 1/4", "turn CCW 1/8")
- Iterative workflow (probe → adjust → re-probe)
- Different bed shapes (rectangular, circular)

**Klipper Command:** `SCREWS_TILT_CALCULATE`

**Implementation Needs:**
- Parse screw positions from printer config
- Visual bed representation with clickable screws
- Real-time probe results display
- Rotation amount calculator
- Re-probe button for iteration

#### 2. Input Shaper Panel
- Run `SHAPER_CALIBRATE` button
- Run `MEASURE_AXES_NOISE` button
- Progress indicator during measurement
- Display current settings (shaper type, frequency)
- View resonance graphs (PNG files in /tmp)

#### 3. Firmware Retraction Panel
- View current settings from `[firmware_retraction]`
- Adjust `retract_length`, `retract_speed`
- Adjust `unretract_extra_length`, `unretract_speed`
- Apply changes via SET_RETRACTION

### KlipperScreen Weaknesses We Can Exploit
1. **Performance** - Python/GTK can be laggy on Pi
2. **UI Scaling** - Font sizes don't scale well on small screens
3. **Touch Reliability** - Depends on Xorg, breaks after updates
4. **No PID UI** - Uses console for PID tuning
5. **No Print History** - Cannot view past jobs
6. **Limited Theming** - CSS-based, crashes on errors

---

## Mainsail (Web UI Leader)

### Overview
- **Technology:** Vue.js
- **Target:** Any browser
- **Repo:** https://github.com/mainsail-crew/mainsail
- **Docs:** https://docs.mainsail.xyz/

### Features Relevant for Touchscreen

| Feature | Touchscreen Appropriate? | Priority |
|---------|-------------------------|----------|
| Print History & Statistics | YES | CRITICAL |
| Job Queue | YES | HIGH |
| Temperature Presets | YES - CRITICAL | CRITICAL |
| Console with Regex Filters | YES | HIGH |
| Timelapse Controls | YES | HIGH |
| Multi-Webcam | YES | CRITICAL |
| Update Manager | YES | HIGH |
| Spoolman QR Scanner | YES - killer feature | HIGH |
| Bed Mesh 3D | YES | ✅ Have it |
| G-code 3D Viewer | NO - too heavy | Skip |
| Theme Ecosystem | Maybe | LOW |
| Macro Editor | NO - needs keyboard | Skip |
| Config Editor | NO - needs keyboard | Skip |

### Mainsail UX Patterns to Adopt
1. **Temperature Presets** - Material buttons with quick-apply
2. **Console Filters** - Predefined + custom regex to hide temp spam
3. **Dashboard Cards** - Draggable, resizable (not for touch, but card concept)
4. **Exclude Object Modal** - Visual selection on build plate
5. **History Statistics** - Total time, filament, success rate

---

## Fluidd (Alternative Web UI)

### Overview
- **Technology:** Vue.js
- **Repo:** https://github.com/fluidd-core/fluidd
- **Docs:** https://docs.fluidd.xyz/

### Unique Fluidd Features
1. **User Management** - Multi-user support
2. **Colorful UI** - More vibrant than Mainsail
3. **Simpler Layout** - Less overwhelming for beginners
4. **Plugin System** - Community extensions
5. **2D G-code Viewer** - Lighter than Mainsail's 3D

### Fluidd vs Mainsail for HelixScreen
- Both have similar core features
- Mainsail has more advanced features
- Fluidd is slightly simpler/cleaner
- **Recommendation:** Target Mainsail feature parity, adopt Fluidd's cleaner aesthetic

---

## Mobileraker (Mobile App)

### Overview
- **Technology:** Flutter
- **Target:** iOS and Android
- **Repo:** https://github.com/Clon1998/mobileraker
- **Site:** https://mobileraker.com/

### Unique Mobile Features
1. **Push Notifications** - Print status anywhere (requires Companion)
2. **iOS Live Activities** - Lock screen widget
3. **Custom Notifications** - M117 $MR$:message or MR_NOTIFY macro
4. **Multi-Printer Dashboard** - View all printers at once
5. **Remote Access** - OctoEverywhere/Obico integration

### Notification Patterns to Consider
```gcode
; In Klipper macros, trigger Mobileraker notifications:
M117 $MR$:NOTIFY|Title|Message
; or
MR_NOTIFY TITLE="Print Done" MESSAGE="Your print is ready"
```

### Mobileraker vs HelixScreen
- **Mobileraker:** Remote monitoring, notifications, multi-printer
- **HelixScreen:** On-printer control, calibration, full feature access
- **Relationship:** Complementary, not competitive

---

## OctoScreen (OctoPrint Reference)

### Overview
- **Technology:** GTK+3
- **Target:** OctoPrint users
- **Repo:** https://github.com/Z-Bolt/OctoScreen

### Relevant Ideas
1. Large, touch-friendly buttons
2. Screen rotation support
3. Resolution scaling
4. Simple, learnable interface

### Why It's Less Relevant
- OctoPrint-focused, not Klipper-native
- Less feature-rich
- Smaller community
- Not actively developed

---

# Moonraker API Complete Reference

## Authentication & Connection

### Connection Identification
```
JSON-RPC: server.connection.identify
```
Register client for persistent connection. Returns `connection_id`.

### Authentication Endpoints
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/access/login` | POST | JWT authentication |
| `/access/logout` | POST | End session |
| `/access/refresh_jwt` | POST | Renew token |
| `/access/oneshot_token` | GET | 5-second token for WebSocket |
| `/access/api_key` | GET/POST | API key management |
| `/access/user` | POST/DELETE | User management |
| `/access/users/list` | GET | List all users |
| `/access/user/password` | POST | Change password |

---

## Printer Control

### Core Operations
| Endpoint | Method | Description | Priority |
|----------|--------|-------------|----------|
| `/printer/emergency_stop` | POST | E-stop | CRITICAL |
| `/printer/info` | GET | System state, version | CRITICAL |
| `/printer/restart` | POST | Soft restart | CRITICAL |
| `/printer/firmware_restart` | POST | Full MCU restart | CRITICAL |
| `/printer/objects/list` | GET | Available Klipper objects | CRITICAL |
| `/printer/objects/query` | POST | Query object status | CRITICAL |
| `/printer/query_endstops/status` | GET | Endstop states | HIGH |

### G-code Operations
| Endpoint | Method | Description | Priority |
|----------|--------|-------------|----------|
| `/printer/gcode/script` | POST | Execute G-code | CRITICAL |
| `/printer/gcode/help` | GET | List commands with descriptions | MEDIUM |

### Print Job Control
| Endpoint | Method | Description | Priority |
|----------|--------|-------------|----------|
| `/printer/print/start?filename=<file>` | POST | Start print | CRITICAL |
| `/printer/print/pause` | POST | Pause | CRITICAL |
| `/printer/print/resume` | POST | Resume | CRITICAL |
| `/printer/print/cancel` | POST | Cancel | CRITICAL |

---

## File Management

### File Operations
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/files/list?root=gcodes` | GET | List files |
| `/server/files/directory?path=<path>&extended=true` | GET | Directory with disk usage |
| `/server/files/upload` | POST | Upload (multipart) |
| `/server/files/{root}/{filename}` | GET | Download file |
| `/server/files/{root}/{filename}` | DELETE | Delete file |
| `/server/files/move` | POST | Move/rename |
| `/server/files/copy` | POST | Duplicate |

### Directory Operations
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/files/directory` | POST | Create directory |
| `/server/files/directory?path=<path>&force=false` | DELETE | Delete directory |
| `/server/files/roots` | GET | List storage roots |

### Metadata & Thumbnails
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/files/metadata?filename=<path>` | GET | Slicer info, temps, times |
| `/server/files/thumbnails?filename=<path>` | GET | Embedded preview images |
| `/server/files/metascan?filename=<file>` | POST | Force re-extraction |

---

## Job Queue

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/job_queue/status` | GET | Current state and jobs |
| `/server/job_queue/job` | POST | Add files to queue |
| `/server/job_queue/job?job_ids=<ids>` | DELETE | Remove by ID |
| `/server/job_queue/pause` | POST | Pause queue |
| `/server/job_queue/start` | POST | Start queue |
| `/server/job_queue/jump?job_id=<id>` | POST | Move job to front |

**Configuration Options:**
- `load_on_startup` - Auto-load queue on boot
- `automatic_transition` - Start next job automatically
- `job_transition_gcode` - G-code between jobs
- `job_transition_delay` - Delay between jobs

---

## Print History

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/history/list?limit=50&start=0` | GET | Past jobs with pagination |
| `/server/history/job?uid=<uid>` | GET | Single job details |
| `/server/history/totals` | GET | Aggregate statistics |
| `/server/history/job?uid=<uid>` | DELETE | Remove from history |

**Job Record Fields:**
```json
{
  "job_id": "000001",
  "filename": "benchy.gcode",
  "status": "completed",  // completed, cancelled, error, klippy_shutdown
  "start_time": 1699000000.0,
  "end_time": 1699003600.0,
  "print_duration": 3600.0,
  "total_duration": 3650.0,
  "filament_used": 5000.0,  // mm
  "metadata": { ... },
  "auxiliary_data": []
}
```

**Totals Response:**
```json
{
  "total_jobs": 150,
  "total_time": 500000.0,
  "total_print_time": 450000.0,
  "total_filament_used": 10000000.0,
  "longest_job": 36000.0,
  "longest_print": 35000.0
}
```

---

## Webcam Management

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/webcams/list` | GET | All configured webcams |
| `/server/webcams/item?uid=<uid>` | GET | Single webcam config |
| `/server/webcams/item` | POST | Add/update webcam |
| `/server/webcams/item?uid=<uid>` | DELETE | Remove webcam |
| `/server/webcams/test?uid=<uid>` | POST | Test connectivity |

**Webcam Object:**
```json
{
  "uid": "webcam-uuid",
  "name": "Main Camera",
  "enabled": true,
  "service": "mjpegstreamer",  // mjpegstreamer, ustreamer, janus, webrtc_camerastreamer, ipstream
  "stream_url": "http://localhost/webcam/?action=stream",
  "snapshot_url": "http://localhost/webcam/?action=snapshot",
  "target_fps": 15,
  "target_fps_idle": 5,
  "rotation": 0,  // 0, 90, 180, 270
  "flip_horizontal": false,
  "flip_vertical": false,
  "aspect_ratio": "16:9",
  "location": "printer"
}
```

---

## Power Device Control

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/machine/device_power/devices` | GET | List all devices |
| `/machine/device_power/device?device=<name>` | GET | Device status |
| `/machine/device_power/device` | POST | Set state (on/off/toggle) |
| `/machine/device_power/on?<devices>` | POST | Turn on multiple |
| `/machine/device_power/off?<devices>` | POST | Turn off multiple |

**Supported Device Types:**
- GPIO - Raspberry Pi pins
- Klipper Pins - Via `[output_pin]`
- TPLink Smartplug - Kasa devices
- Tasmota - Tasmota firmware
- Shelly - Shelly devices
- Home Assistant - HA integration
- HomeSeer - HomeSeer devices
- Loxone - Loxone system
- SmartThings - Samsung SmartThings
- Philips Hue - Hue lights
- MQTT - Generic MQTT
- HTTP - REST API devices
- RF - RF switches
- uhubctl - USB hub control

**Device Object:**
```json
{
  "device": "printer_power",
  "status": "on",  // on, off, error, init, unknown
  "locked_while_printing": true,
  "type": "gpio"
}
```

---

## Update Manager

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/machine/update/status?refresh=false` | GET | Available updates |
| `/machine/update/refresh` | POST | Check for updates (CPU intensive) |
| `/machine/update/full` | POST | Update all components |
| `/machine/update/<name>` | POST | Update specific component |
| `/machine/update/rollback?name=<name>` | POST | Revert version |
| `/machine/update/recover?name=<name>&hard=false` | POST | Fix corrupt repos |

**Update Status Response:**
```json
{
  "version_info": {
    "klipper": {
      "channel": "stable",
      "remote_version": "v0.12.0",
      "version": "v0.11.0",
      "is_dirty": false,
      "detached": false,
      "commits_behind": 50,
      "git_messages": []
    },
    "moonraker": { ... },
    "mainsail": { ... }
  },
  "busy": false,
  "github_rate_limit": 60,
  "github_requests_remaining": 55,
  "github_limit_reset_time": 1699000000
}
```

---

## System Machine Control

### System Operations
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/machine/system_info` | GET | CPU, memory, network, SD card |
| `/machine/shutdown` | POST | Power off host |
| `/machine/reboot` | POST | Restart host |
| `/machine/proc_stats` | GET | CPU/memory usage, throttling |

### Service Management
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/machine/services/restart?service=<name>` | POST | Restart service |
| `/machine/services/stop?service=<name>` | POST | Stop service |
| `/machine/services/start?service=<name>` | POST | Start service |

### Peripherals
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/machine/peripherals/usb` | GET | USB devices |
| `/machine/peripherals/serial` | GET | Serial ports |
| `/machine/peripherals/video` | GET | Video devices |
| `/machine/peripherals/canbus?interface=<iface>` | GET | CAN bus nodes |

---

## Data Stores

### Temperature Store
```
GET /server/temperature_store?include_monitors=false
```
Returns 20 minutes of temperature history for all heaters/sensors.

**Response:**
```json
{
  "extruder": {
    "temperatures": [200.0, 200.1, ...],
    "targets": [200.0, 200.0, ...],
    "powers": [0.5, 0.48, ...]
  },
  "heater_bed": { ... }
}
```

### GCode Store
```
GET /server/gcode_store?count=100
```
FIFO queue of recent G-code commands and responses.

**Response:**
```json
{
  "gcode_store": [
    {"message": "ok", "time": 1699000000.0, "type": "response"},
    {"message": "G28", "time": 1699000001.0, "type": "command"}
  ]
}
```

---

## Spoolman Integration

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/server/spoolman/status` | GET | Connection status |
| `/server/spoolman/spool_id` | POST | Set active spool |
| `/server/spoolman/proxy` | POST | Forward API requests |

**Status Response:**
```json
{
  "spoolman_connected": true,
  "pending_reports": [],
  "spool_id": 5
}
```

**Proxy Usage:**
```json
POST /server/spoolman/proxy
{
  "request_method": "GET",
  "path": "/api/v1/spool",
  "query": "filament_id=3"
}
```

---

## WebSocket Notifications

### Critical Notifications
| Event | Description |
|-------|-------------|
| `notify_klippy_ready` | Klipper entered ready state |
| `notify_klippy_shutdown` | Klipper shutdown |
| `notify_klippy_disconnected` | Connection lost |
| `notify_status_update` | Printer object changes |
| `notify_gcode_response` | G-code command output |

### Job Notifications
| Event | Description |
|-------|-------------|
| `notify_history_changed` | Job added/finished |
| `notify_job_queue_changed` | Queue modified |
| `notify_filelist_changed` | Files created/deleted/moved |
| `notify_metadata_update` | New file metadata parsed |

### System Notifications
| Event | Description |
|-------|-------------|
| `notify_proc_stat_update` | CPU/memory/network stats |
| `notify_cpu_throttled` | Thermal throttling |
| `notify_service_state_changed` | Service status |
| `notify_update_response` | Update progress |
| `notify_update_refreshed` | Update check complete |

### Integration Notifications
| Event | Description |
|-------|-------------|
| `notify_webcams_changed` | Webcam config changed |
| `notify_active_spool_set` | Spoolman spool changed |
| `notify_spoolman_status_changed` | Spoolman connection |
| `notify_sensor_update` | Sensor measurements |
| `notify_button_event` | GPIO button press |

---

## Key Printer Objects

### Motion
| Object | Key Fields |
|--------|------------|
| `toolhead` | `position`, `homed_axes`, `max_velocity`, `max_accel`, `print_time` |
| `gcode_move` | `speed_factor`, `extrude_factor`, `gcode_position`, `homing_origin` |
| `motion_report` | `live_position`, `live_velocity` (real-time) |

### Heaters
| Object | Key Fields |
|--------|------------|
| `extruder` | `temperature`, `target`, `power`, `can_extrude`, `pressure_advance` |
| `heater_bed` | `temperature`, `target`, `power` |
| `heater_generic` | `temperature`, `target`, `power` |
| `temperature_sensor` | `temperature`, `measured_min_temp`, `measured_max_temp` |
| `temperature_fan` | `speed`, `temperature`, `target` |

### Fans
| Object | Key Fields |
|--------|------------|
| `fan` | `speed`, `rpm` (if available) |
| `heater_fan` | `speed`, `rpm` |
| `controller_fan` | `speed`, `rpm` |

### Print Status
| Object | Key Fields |
|--------|------------|
| `print_stats` | `filename`, `state`, `print_duration`, `filament_used`, `info.current_layer`, `info.total_layer` |
| `virtual_sdcard` | `file_path`, `progress`, `is_active`, `file_position`, `file_size` |
| `display_status` | `message`, `progress` |
| `pause_resume` | `is_paused` |

### Bed Leveling
| Object | Key Fields |
|--------|------------|
| `bed_mesh` | `profile_name`, `mesh_min`, `mesh_max`, `probed_matrix`, `profiles` |
| `screws_tilt_adjust` | `error`, `max_deviation`, `results` |
| `z_tilt` | `applied` |
| `quad_gantry_level` | `applied` |

### Sensors & Safety
| Object | Key Fields |
|--------|------------|
| `filament_switch_sensor` | `filament_detected`, `enabled` |
| `filament_motion_sensor` | `filament_detected`, `enabled` |
| `idle_timeout` | `state`, `printing_time` |

### System
| Object | Key Fields |
|--------|------------|
| `webhooks` | `state` (ready/shutdown/startup/error), `state_message` |
| `configfile` | `config`, `settings`, `save_config_pending`, `warnings` |
| `mcu` | `mcu_version`, `mcu_build_versions` |
| `exclude_object` | `objects`, `excluded_objects`, `current_object` |

### Firmware Retraction
| Object | Key Fields |
|--------|------------|
| `firmware_retraction` | `retract_length`, `retract_speed`, `unretract_extra_length`, `unretract_speed` |

### Input Shaper
| Object | Key Fields |
|--------|------------|
| `input_shaper` | `shaper_type_x`, `shaper_freq_x`, `damping_ratio_x`, `shaper_type_y`, `shaper_freq_y`, `damping_ratio_y` |

---

# Feature Gap Analysis

## TIER 1: CRITICAL (Must Have for Parity)

### 1. Temperature Presets
**Gap:** Every competitor has material presets (PLA/PETG/ABS/etc.)
**Impact:** Users manually type temperatures every time
**Competitors:** KlipperScreen, Mainsail, Fluidd all have this

**Requirements:**
- Preset buttons on nozzle/bed temp panels
- Default presets: PLA (200/60), PETG (240/80), ABS (250/100), TPU (230/50), ASA (260/100)
- Custom preset creation/editing
- Quick-apply from home screen
- Store in config/database

**Implementation:**
```cpp
struct TempPreset {
    std::string name;
    std::string material;
    int nozzle_temp;
    int bed_temp;
    uint32_t color;  // For UI chip
};
```

**Files:**
- `ui_xml/temp_preset_modal.xml` - Create/edit presets
- `include/temperature_presets.h` - Data structures
- `src/temperature_presets.cpp` - Logic
- Modify `nozzle_temp_panel.xml`, `bed_temp_panel.xml` - Add buttons

---

### 2. Macro Panel
**Gap:** Cannot execute/manage macros from touchscreen
**Impact:** Users need laptop for calibration, maintenance, custom workflows
**Competitors:** KlipperScreen has `gcode_macros` panel

**Requirements:**
- List all Klipper macros from `printer.objects.list`
- Categorization (Calibration, User, System, HelixScreen, Hidden)
- Parameter input via on-screen keyboard (for macros with `params`)
- Favorites/quick access
- Hide system/internal macros option
- Search/filter

**Macro Detection:**
```python
# Klipper macro in printer.cfg
[gcode_macro PRINT_START]
description: Initialize print
gcode:
    {% set BED_TEMP = params.BED_TEMP|default(60)|float %}
    {% set EXTRUDER_TEMP = params.EXTRUDER_TEMP|default(200)|float %}
    M140 S{BED_TEMP}
    M104 S{EXTRUDER_TEMP}
    G28
```

**Implementation:**
```cpp
struct MacroInfo {
    std::string name;
    std::string description;
    std::string category;  // Detected or configured
    std::vector<std::string> parameters;
    bool is_favorite;
    bool is_hidden;
};
```

**Files:**
- `ui_xml/macro_panel.xml` - Main panel with list
- `ui_xml/macro_card.xml` - Individual macro card
- `ui_xml/macro_params_modal.xml` - Parameter input
- `include/ui_panel_macros.h`
- `src/ui_panel_macros.cpp`

---

### 3. Console Panel
**Gap:** No G-code terminal access from touchscreen
**Impact:** Cannot run custom commands, see errors, debug issues
**Competitors:** KlipperScreen, Mainsail, Fluidd all have consoles

**Requirements:**
- Scrollable command history (last 100-500 lines)
- On-screen keyboard for input
- Temperature message filtering (regex: `^(ok\s+)?(B|T\d*):`)
- Color-coded output (errors red, warnings yellow)
- Command history (up/down navigation)
- Copy output to clipboard (if possible)

**Moonraker APIs:**
- `GET /server/gcode_store?count=100` - Get history
- `POST /printer/gcode/script` - Execute command
- `notify_gcode_response` - Real-time output via WebSocket

**Implementation:**
```cpp
struct ConsoleEntry {
    std::string message;
    double timestamp;
    enum Type { COMMAND, RESPONSE, ERROR, WARNING } type;
};

class ConsolePanel : public PanelBase {
    std::deque<ConsoleEntry> history_;
    std::string pending_command_;
    bool filter_temps_ = true;
    // ...
};
```

**Files:**
- `ui_xml/console_panel.xml`
- `include/ui_panel_console.h`
- `src/ui_panel_console.cpp`

---

### 4. Screws Tilt Adjust Visual Panel
**Gap:** KlipperScreen's most praised feature - we don't have it
**Impact:** Users cannot do probe-assisted bed leveling from touchscreen
**Competitors:** Only KlipperScreen has this well

**Requirements:**
- Visual bed diagram with screw positions
- Support for 3, 4, 5, 6, 8, 9 screw configurations
- Rotation indicators ("turn CW 1/4", "turn CCW 1/8")
- Iterative workflow (probe → adjust → re-probe)
- Different bed shapes support
- Reference screw indication
- Acceptable range highlighting

**Klipper Configuration:**
```ini
[screws_tilt_adjust]
screw1: 30, 30
screw1_name: front left
screw2: 270, 30
screw2_name: front right
screw3: 270, 270
screw3_name: rear right
screw4: 30, 270
screw4_name: rear left
horizontal_move_z: 10
speed: 100
screw_thread: CW-M4
```

**Klipper Command & Response:**
```
SCREWS_TILT_CALCULATE
// Response:
// 01:20:00 : front left (base) : x=30.0, y=30.0, z=2.00000
// 01:20:05 : front right : x=270.0, y=30.0, z=2.05000 : adjust CW 00:03
// 01:20:10 : rear right : x=270.0, y=270.0, z=1.95000 : adjust CCW 00:02
// 01:20:15 : rear left : x=30.0, y=270.0, z=2.02000 : adjust CW 00:01
```

**Implementation:**
```cpp
struct ScrewResult {
    std::string name;
    float x, y, z;
    bool is_base;
    std::string adjustment;  // "CW 00:03" or empty
    float deviation;
};

class ScrewsTiltPanel : public PanelBase {
    std::vector<ScrewResult> results_;
    float max_deviation_;
    bool probing_in_progress_;
    // ...
};
```

**Files:**
- `ui_xml/screws_tilt_panel.xml` - Main panel with bed visual
- `ui_xml/screw_indicator.xml` - Reusable screw widget
- `include/ui_panel_screws_tilt.h`
- `src/ui_panel_screws_tilt.cpp`

---

### 5. Camera/Webcam Integration
**Gap:** No camera support at all
**Impact:** Cannot monitor prints on the touchscreen itself
**Competitors:** All have webcam support

**Requirements:**
- Phase 1: Single camera MJPEG view
- Phase 2: Multi-camera selector
- Phase 3: Picture-in-picture during prints
- Snapshot button
- Rotation/flip settings
- Support MJPEG and potentially WebRTC

**Moonraker APIs:**
- `GET /server/webcams/list` - Get configured cameras
- Webcam `stream_url` for MJPEG feed
- Webcam `snapshot_url` for still images

**MJPEG Display Options:**
1. **LVGL Image + HTTP fetch** - Periodically fetch snapshots
2. **Custom MJPEG decoder** - Parse MJPEG stream, display frames
3. **SDL2 integration** - Use SDL2 for video (development only)

**Implementation Challenges:**
- MJPEG parsing in C++
- Frame rate management
- Memory management for frames
- Network bandwidth consideration

**Files:**
- `ui_xml/camera_panel.xml` - Full camera view
- `ui_xml/camera_pip.xml` - Picture-in-picture overlay
- `include/webcam_client.h` - MJPEG client
- `src/webcam_client.cpp`
- `include/ui_panel_camera.h`
- `src/ui_panel_camera.cpp`

---

### 6. Print History Panel
**Gap:** No job history tracking
**Impact:** Cannot see past prints, track success rate, reprint
**Competitors:** Mainsail, Fluidd have excellent history

**Requirements:**
- List past jobs with pagination
- Success/failure/cancelled indicators
- Print duration, filament used
- Thumbnail from metadata
- Reprint from history
- Statistics dashboard (totals)
- Delete entries
- Filter by status

**Moonraker APIs:**
- `GET /server/history/list?limit=50&start=0`
- `GET /server/history/totals`
- `DELETE /server/history/job?uid=<uid>`

**Implementation:**
```cpp
struct HistoryJob {
    std::string uid;
    std::string filename;
    std::string status;  // completed, cancelled, error
    double start_time;
    double end_time;
    double print_duration;
    double filament_used;
    std::string thumbnail_path;
};

struct HistoryTotals {
    int total_jobs;
    int completed_jobs;
    int failed_jobs;
    double total_time;
    double total_filament;
};
```

**Files:**
- `ui_xml/history_panel.xml` - Job list with stats header
- `ui_xml/history_item.xml` - Job row/card
- `include/ui_panel_history.h`
- `src/ui_panel_history.cpp`

---

### 7. Power Device Control Panel
**Gap:** No Moonraker power device control
**Impact:** Cannot turn on printer, lights, enclosure from touchscreen
**Competitors:** KlipperScreen has power panel, Mainsail/Fluidd have power controls

**Requirements:**
- List all power devices
- On/Off/Toggle buttons
- Status indicators (on/off/error/init)
- Safety locks for critical devices (locked_while_printing)
- Quick access from home screen
- Confirmation for critical operations

**Moonraker APIs:**
- `GET /machine/device_power/devices`
- `POST /machine/device_power/device` with `{device: "name", action: "on|off|toggle"}`

**Implementation:**
```cpp
struct PowerDevice {
    std::string name;
    std::string status;  // on, off, error, init, unknown
    std::string type;    // gpio, tplink, tasmota, etc.
    bool locked_while_printing;
};
```

**Files:**
- `ui_xml/power_panel.xml` - Device list
- `ui_xml/power_device_row.xml` - Device row with toggle
- `include/ui_panel_power.h`
- `src/ui_panel_power.cpp`

---

## TIER 2: HIGH PRIORITY (Should Have)

### 8. Input Shaper Panel
**Gap:** KlipperScreen has it, essential for print quality tuning
**Implementation:** Run SHAPER_CALIBRATE, display results, show graphs

### 9. Firmware Retraction Panel
**Gap:** KlipperScreen has dedicated panel
**Implementation:** View/edit firmware_retraction settings

### 10. Spoolman Integration
**Gap:** Framework exists but not functional
**Implementation:** Full panel with QR scanner

### 11. Job Queue Panel
**Gap:** Batch printing not available
**Implementation:** Queue management UI

### 12. Update Manager Panel
**Gap:** Cannot update software from touchscreen
**Implementation:** Update status, one-click update

### 13. Timelapse Controls
**Gap:** Config-only, no UI
**Implementation:** Enable/disable, settings, video browser

### 14. Layer Information Display
**Gap:** Current/total layers not shown
**Implementation:** Add to print_status_panel.xml

---

## TIER 3: MEDIUM PRIORITY (Nice to Have)

- Limits Panel (velocity/acceleration)
- LED Effects Control
- Advanced Probe Calibration (Beacon/Cartographer)
- Temperature Graphs
- Filament Sensor Status
- System Information
- Adaptive Bed Mesh Toggle

---

## TIER 4: DIFFERENTIATORS (Beat Competitors)

### PID Tuning UI
**Unique Opportunity:** NO touchscreen has this

**Implementation:**
```xml
<lv_obj name="pid_tuning_panel">
  <!-- Heater selector: Hotend / Bed -->
  <!-- Target temperature input -->
  <!-- Start PID_CALIBRATE button -->
  <!-- Progress indicator -->
  <!-- Results display: Kp, Ki, Kd -->
  <!-- Save to config button -->
</lv_obj>
```

### Pressure Advance UI
**Unique Opportunity:** Console-only everywhere

### First-Layer Calibration Wizard
**Unique Opportunity:** No guided workflow exists

### Material Database
**Unique Opportunity:** No built-in presets

### Maintenance Tracker
**Unique Opportunity:** NO ONE tracks this
- Nozzle change reminders (by hours/prints)
- Belt tension schedule
- Filament path cleaning
- Total print hours

---

# Klipper Extensions Integration

## Critical Extensions

### Exclude Object (✅ DONE)
- Native Klipper feature
- Already implemented in HelixScreen

### Crowsnest (Camera Service)
- Standard camera solution for Klipper
- Supports up to 4 cameras
- MJPEG streaming via ustreamer/mjpg-streamer
- **Integration:** Use webcam URLs from Moonraker API

### Native Adaptive Mesh (Klipper 0.12+)
- Only meshes print area
- Faster, denser meshes
- **Integration:** Toggle in bed mesh settings

### Screws Tilt Adjust
- Native Klipper feature
- **Integration:** Visual panel (see above)

### Input Shaper
- Native Klipper feature
- **Integration:** Calibration panel

---

## High Priority Extensions

### Spoolman (1.4k+ GitHub stars)
**Repository:** https://github.com/Donkie/Spoolman

**Features:**
- Centralized filament inventory
- QR code scanning for spool selection
- Usage tracking
- Multi-printer support
- Native Moonraker integration

**UI Needs:**
- Spoolman panel with spool list
- Active spool display
- Spool selection modal at print start
- QR scanner via webcam (killer feature!)
- Remaining filament gauge
- Low filament warnings

### Moonraker Timelapse
**Repository:** https://github.com/mainsail-crew/moonraker-timelapse

**Modes:**
- `layermacro` - Capture at layer changes
- `hyperlapse` - Time-based capture

**UI Needs:**
- Enable/disable toggle
- Mode selector
- Frame rate setting
- Video library browser
- Preview/download/delete

### Shake&Tune
**Repository:** https://github.com/Frix-x/klippain-shaketune

**Features:**
- Advanced input shaper calibration
- Better visualizations than stock
- Belt comparison

**UI Needs:**
- Calibration triggers
- Graph viewer for results
- Settings display

### Happy Hare (MMU/AMS)
**Repository:** https://github.com/moggieuk/Happy-Hare

**Features:**
- Universal MMU driver
- Supports ERCF, TradRack, 3MS, Box Turtle, etc.

**UI Needs:**
- Full AMS panel (see AMS_IMPLEMENTATION_PLAN.md)
- Tool selector
- Filament map
- Load/unload
- Error recovery

---

## Medium Priority Extensions

### LED Effects
**Repository:** https://github.com/julianschill/klipper-led_effect

**UI Needs:**
- Preset selector
- Color picker
- Brightness slider
- Effect speed control

### TMC Autotune
**Repository:** https://github.com/andrewmcgr/klipper_tmc_autotune

**UI Needs:**
- Per-axis tuning goal
- Apply button
- Status display

### Beacon/Cartographer/BTT Eddy
Advanced eddy current probes

**UI Needs:**
- Calibration wizards
- Mode selection (contact/scan)
- Z offset adjustment
- Diagnostics

### Z Calibration Plugin
**Repository:** https://github.com/protoloft/klipper_z_calibration

**UI Needs:**
- Calibration wizard
- Offset display
- History

---

## Low Priority Extensions

- **Obico** - Status display only (remote monitoring)
- **Telegram Bot** - External notification
- **PrettyGCode** - Too resource-heavy for embedded
- **Print Queue** - Farm/belt printer use case

---

# Community Pain Points & Requests

## Research Sources
- Reddit: r/klippers, r/VORONDesign, r/3Dprinting
- GitHub: KlipperScreen issues, Mainsail issues
- Klipper Discourse forum
- Voron Discord
- YouTube reviews

## Most Requested Features

### 1. Feature Parity with Web UIs
**Frequency:** Very High
**Quote:** "I wish I didn't have to grab my laptop to..."

Users expect touchscreen to do everything web UI can:
- View graphs
- Run calibrations
- See history
- Control power devices

### 2. Bed Mesh Visualization
**Status:** ✅ We have this!

### 3. Input Shaper Graphs
**Frequency:** High
**Issue:** Graphs stored in /tmp, not accessible from touchscreen
**Solution:** Graph viewer panel

### 4. Layer Information During Print
**Frequency:** High
**Issue:** Can't see current layer / total layers
**Solution:** Add to print status panel (easy win!)

### 5. Power Controls on Home Screen
**Frequency:** High
**Issue:** Have to navigate to turn on printer
**Solution:** Quick access power button

### 6. Camera During Print
**Frequency:** High
**Issue:** Can't see print progress visually
**Solution:** Picture-in-picture camera

### 7. Temperature Presets
**Frequency:** Very High
**Issue:** Type temperatures every time
**Solution:** Material preset buttons

### 8. Print History
**Frequency:** High
**Issue:** Can't track printer usage
**Solution:** History panel with stats

---

## Common Complaints About KlipperScreen

### 1. UI Scaling Issues
**Quote:** "I have to squint to see..."
**Our Status:** ✅ We have responsive breakpoints

### 2. Touch Reliability
**Quote:** "The update broke touch..."
**Our Status:** ✅ Native app avoids Xorg issues

### 3. Performance/Lag
**Quote:** "It's so laggy I just use Mainsail instead..."
**Our Status:** ✅ Native C++ should be fast

### 4. Update Breakage
**Quote:** "The update broke everything..."
**Our Solution Needed:** Rollback mechanism

### 5. Features Buried in Menus
**Quote:** "Why is this buried in a menu?"
**Our Solution:** Better information architecture

### 6. Spoolman Bugs
**Quote:** "Spoolman menu causes display bugs..."
**Our Solution:** Careful implementation

---

## UX Opportunities

### E-Stop Visibility
**Request:** Always visible, large
**Implementation:** Persistent button in header/footer

### Power ON from Home
**Request:** Quick access
**Implementation:** Power card on home panel

### File Search
**Request:** Filter files by name
**Implementation:** Search bar in print select

### Multi-Language
**Request:** Full i18n support
**Status:** Need to verify/improve

### Font Size Options
**Request:** Accessibility
**Implementation:** Settings option for text scaling

---

## User Quotes to Remember

> "I wish I didn't have to grab my laptop to [do X]"

This is the core user need. The touchscreen should be **complete**.

> "Mainsail has this but KlipperScreen doesn't..."

Users compare touchscreen to web UI. We need feature parity.

> "The update broke everything and I can't..."

Reliability matters more than features. Don't break things.

> "Why is this buried in a menu when I use it every day?"

Information architecture matters. Frequent actions should be accessible.

> "I have to squint to see the text on my small screen"

Responsive design is essential. Test on real hardware.

---

# Implementation Specifications

## Strategy: Breadth First with "Coming Soon" Markers

**Approach:**
1. Create ALL critical panel stubs with navigation
2. Add "Coming Soon" overlays to incomplete features
3. Implement quick wins fully
4. Build out core features progressively
5. Track status in docs/FEATURE_STATUS.md

**Benefits:**
- Users see planned features
- Developers know what's incomplete
- No false promises
- Progress is visible

## Coming Soon Component

Add to `globals.xml`:
```xml
<component name="coming_soon_overlay">
    <lv_obj name="coming_soon_container"
            style_width="100%"
            style_height="100%"
            style_flex_flow="column"
            style_flex_main_place="center"
            style_flex_cross_place="center"
            style_bg_color="#overlay_bg"
            style_bg_opa="230">

        <lv_label style_text_font="mdi_icons_48"
                  style_text_color="#text_secondary">
            󰚌
        </lv_label>

        <text_heading style_text_align="center"
                      style_margin_top="#space_md">
            Coming Soon
        </text_heading>

        <text_body style_text_align="center"
                   style_text_color="#text_secondary"
                   style_margin_top="#space_sm">
            This feature is under development
        </text_body>
    </lv_obj>
</component>
```

## Phase 1: Scaffolding (~4 hours)

Create panel stubs:
| Panel | Nav Location | Notes |
|-------|--------------|-------|
| `macro_panel.xml` | Controls sub-panel | New card in controls |
| `console_panel.xml` | Nav bar icon | New nav item |
| `camera_panel.xml` | Nav bar icon | New nav item |
| `history_panel.xml` | Nav bar icon | New nav item |
| `power_panel.xml` | Settings section | Or home quick access |
| `screws_tilt_panel.xml` | Controls sub-panel | Under calibration |
| `input_shaper_panel.xml` | Controls sub-panel | Under calibration |

## Phase 2: Quick Wins (~3 hours)

Fully implement:
| Feature | Time | Complexity |
|---------|------|------------|
| Layer display in print_status | 30min | LOW |
| Temperature presets (basic) | 90min | LOW-MEDIUM |
| Power device control | 60min | LOW |

## Phase 3: Core Features (~5 hours)

Build functional versions:
| Feature | Target State |
|---------|--------------|
| Macro Panel | List, execute (no params) |
| Console Panel | Read-only history |
| Camera Panel | Single MJPEG stream |
| History Panel | List jobs (no stats) |

---

# UI/UX Considerations

## Screen Size Breakpoints

Existing in globals.xml:
- **Small:** ≤272px width
- **Medium:** ≤480px width
- **Large:** >480px width

## Touch Target Sizes

Minimum touch targets:
- **Primary actions:** 48x48px
- **Secondary actions:** 40x40px
- **List items:** 48px height minimum
- **Spacing between targets:** 8px minimum

## Navigation Patterns

### Current Structure
```
Home
├── Controls
│   ├── Motion
│   ├── Nozzle Temp
│   ├── Bed Temp
│   ├── Extrusion
│   ├── Filament
│   ├── Fan
│   └── Advanced (Bed Mesh)
├── Print Select
├── Print Status (during print)
└── Settings
```

### Proposed Additions
```
Home
├── Controls
│   ├── ... existing ...
│   ├── Macros          ← NEW
│   ├── Input Shaper    ← NEW
│   ├── Screws Tilt     ← NEW
│   └── Limits          ← NEW
├── Print Select
├── Print Status
├── Camera              ← NEW (nav bar)
├── Console             ← NEW (nav bar)
├── History             ← NEW (nav bar)
└── Settings
    └── Power Devices   ← NEW (settings section)
```

## Color Coding

### Status Colors (from globals.xml)
- **Success/Ready:** `#success_color`
- **Warning:** `#warning_color`
- **Error:** `#error_color`
- **Info:** `#info_color`

### Print Status Colors
- **Printing:** Primary blue
- **Paused:** Warning yellow
- **Complete:** Success green
- **Error:** Error red
- **Cancelled:** Muted gray

### Temperature Colors
- **Cold:** Blue
- **Heating:** Orange/animated
- **At target:** Green
- **Over target:** Red

## Icons (MDI)

### Existing Icons
- Home: `󰋜`
- Settings: `󰒓`
- Print: `󰐊`
- Temperature: `󰔏`
- Fan: `󰈐`

### New Icons Needed
| Feature | Icon | MDI Name |
|---------|------|----------|
| Macros | `󰘦` | script-text |
| Console | `󰆍` | console |
| Camera | `󰄀` | camera |
| History | `󰋚` | history |
| Power | `󰐥` | power |
| Screws | `󱁼` | screw-machine-flat-top |
| Input Shaper | `󰺅` | sine-wave |
| Timelapse | `󰔝` | timelapse |
| Update | `󰚰` | update |
| Spoolman | `󱊧` | roller-skate |

---

# Code Patterns & Templates

## New Panel Class Template

```cpp
// include/ui_panel_example.h
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UI_PANEL_EXAMPLE_H
#define UI_PANEL_EXAMPLE_H

#include "ui_panel_base.h"
#include "observer_guard.h"
#include <memory>

class ExamplePanel : public PanelBase {
public:
    ExamplePanel();
    ~ExamplePanel() override;

    void create(lv_obj_t* parent) override;
    void update() override;
    void cleanup() override;

private:
    void setup_observers();
    void setup_callbacks();
    void refresh_data();

    ObserverGuard observer_guard_;
    lv_obj_t* content_container_ = nullptr;
    // Panel-specific members
};

#endif // UI_PANEL_EXAMPLE_H
```

```cpp
// src/ui_panel_example.cpp
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_panel_example.h"
#include "ui_nav.h"
#include "moonraker_api.h"
#include <spdlog/spdlog.h>

ExamplePanel::ExamplePanel() = default;

ExamplePanel::~ExamplePanel() {
    cleanup();
}

void ExamplePanel::create(lv_obj_t* parent) {
    spdlog::debug("ExamplePanel::create");

    root_ = lv_xml_create(parent, "example_panel", nullptr);
    if (!root_) {
        spdlog::error("Failed to create example_panel from XML");
        return;
    }

    content_container_ = lv_obj_find_by_name(root_, "content_container");

    setup_observers();
    setup_callbacks();
    refresh_data();
}

void ExamplePanel::update() {
    refresh_data();
}

void ExamplePanel::cleanup() {
    spdlog::debug("ExamplePanel::cleanup");
    observer_guard_.clear();
}

void ExamplePanel::setup_observers() {
    // Subscribe to relevant subjects
    // observer_guard_.add(subject, callback);
}

void ExamplePanel::setup_callbacks() {
    // Register event callbacks
    lv_xml_register_event_cb(root_, "example_button_clicked", [](lv_event_t* e) {
        spdlog::info("Example button clicked");
        // Handle click
    });
}

void ExamplePanel::refresh_data() {
    // Fetch and display data
}
```

## XML Panel Template

```xml
<?xml version="1.0" encoding="UTF-8"?>
<component>
    <lv_obj name="example_panel"
            style_width="100%"
            style_height="100%"
            style_bg_color="#panel_bg_color"
            style_pad_all="#space_md"
            style_flex_flow="column">

        <!-- Header -->
        <header_bar title="Example Panel"/>

        <!-- Content Container -->
        <lv_obj name="content_container"
                style_width="100%"
                style_flex_grow="1"
                style_flex_flow="column"
                style_gap_row="#space_sm"
                style_pad_top="#space_sm">

            <!-- Panel content goes here -->

        </lv_obj>

        <!-- Coming Soon Overlay (remove when implemented) -->
        <coming_soon_overlay/>

    </lv_obj>
</component>
```

## Moonraker API Method Template

```cpp
// In moonraker_api.h
std::future<std::vector<PowerDevice>> get_power_devices();
std::future<bool> set_power_device_state(
    const std::string& device,
    const std::string& action  // "on", "off", "toggle"
);

// In moonraker_api.cpp
std::future<std::vector<PowerDevice>> MoonrakerApi::get_power_devices() {
    return std::async(std::launch::async, [this]() {
        std::vector<PowerDevice> devices;

        auto response = client_.get("/machine/device_power/devices");
        if (!response || !response->contains("result")) {
            spdlog::error("Failed to get power devices");
            return devices;
        }

        auto& result = (*response)["result"]["devices"];
        for (auto& [name, info] : result.items()) {
            devices.push_back({
                .name = name,
                .status = info.value("status", "unknown"),
                .type = info.value("type", "unknown"),
                .locked_while_printing = info.value("locked_while_printing", false)
            });
        }

        return devices;
    });
}

std::future<bool> MoonrakerApi::set_power_device_state(
    const std::string& device,
    const std::string& action
) {
    return std::async(std::launch::async, [this, device, action]() {
        nlohmann::json params = {
            {"device", device},
            {"action", action}
        };

        auto response = client_.post("/machine/device_power/device", params);
        if (!response) {
            spdlog::error("Failed to set power device state: {} -> {}", device, action);
            return false;
        }

        return true;
    });
}
```

## Panel Registration (main.cpp)

```cpp
// Include new panel headers
#include "ui_panel_macros.h"
#include "ui_panel_console.h"
#include "ui_panel_camera.h"
#include "ui_panel_history.h"
#include "ui_panel_power.h"

// In setup_panels() or panel registry initialization
void register_panels() {
    auto& registry = PanelRegistry::instance();

    // Existing panels...

    // New panels
    registry.register_panel("macros", []() {
        return std::make_unique<MacrosPanel>();
    });

    registry.register_panel("console", []() {
        return std::make_unique<ConsolePanel>();
    });

    registry.register_panel("camera", []() {
        return std::make_unique<CameraPanel>();
    });

    registry.register_panel("history", []() {
        return std::make_unique<HistoryPanel>();
    });

    registry.register_panel("power", []() {
        return std::make_unique<PowerPanel>();
    });
}

// Register navigation callbacks
void register_nav_callbacks() {
    lv_xml_register_event_cb(nullptr, "nav_macros_clicked", [](lv_event_t* e) {
        ui_nav_push_panel("macros");
    });

    lv_xml_register_event_cb(nullptr, "nav_console_clicked", [](lv_event_t* e) {
        ui_nav_push_panel("console");
    });

    // etc...
}
```

---

# Testing Strategy

## Unit Tests (Catch2)

### New Test Files Needed
- `tests/unit/test_temperature_presets.cpp`
- `tests/unit/test_macro_parser.cpp`
- `tests/unit/test_history_api.cpp`
- `tests/unit/test_power_api.cpp`
- `tests/unit/test_webcam_client.cpp`

### Test Categories
1. **API Response Parsing** - Mock Moonraker responses
2. **Data Structures** - Serialize/deserialize
3. **State Management** - Subject updates
4. **Edge Cases** - Empty responses, errors, timeouts

## Integration Tests

### Manual Test Checklist
- [ ] Temperature presets save/load correctly
- [ ] Macros execute with correct parameters
- [ ] Console displays real-time output
- [ ] Camera stream displays without memory leaks
- [ ] History pagination works
- [ ] Power devices toggle correctly
- [ ] Screws tilt displays correct adjustments

### Mock Mode Testing
```bash
./build/bin/helix-screen --test -p macros -vv
./build/bin/helix-screen --test -p console -vv
./build/bin/helix-screen --test -p history -vv
```

## Performance Testing

### Metrics to Track
- Panel load time
- Memory usage over time
- Frame rate during camera view
- Response latency for API calls

### Benchmarks
- Camera: Maintain 15fps MJPEG display
- Console: Handle 500+ line history smoothly
- History: Load 100 jobs in <1s

---

# Future Ideas & Possibilities

## Longer Term Features

### Voice Feedback
- Audio notifications for print events
- Text-to-speech for errors
- Configurable sounds

### Gesture Controls
- Swipe to change panels
- Pinch to zoom bed mesh
- Long-press context menus

### Remote Access
- Direct connection from mobile
- Cloud integration (Obico-style)
- Push notifications

### Multi-Printer Support
- Printer selector on home
- Per-printer configuration
- Status overview dashboard

### Print Farm Features
- Print queue scheduling
- Material assignment
- Production tracking

## Experimental Ideas

### AI Integration
- Failure detection from camera
- Print quality analysis
- Suggested settings

### Augmented Reality
- AR overlay for bed leveling
- Visual guides for maintenance
- Part placement preview

### Social Features
- Share prints to community
- Download popular profiles
- Leaderboards (print hours, success rate)

## Technical Improvements

### Plugin System
- User-installable extensions
- Custom panel development
- Theme marketplace

### Web Dashboard
- Remote access via browser
- Mobile-responsive companion
- Configuration sync

### OTA Updates
- Self-updating firmware
- Rollback support
- Update notifications

---

# Appendix: File Index

## New Files to Create

### UI XML (~20 files)
```
ui_xml/
├── coming_soon_overlay.xml      # Reusable overlay component
├── temp_preset_modal.xml        # Preset create/edit
├── macro_panel.xml              # Main macro panel
├── macro_card.xml               # Macro list item
├── macro_params_modal.xml       # Parameter input
├── console_panel.xml            # G-code terminal
├── screws_tilt_panel.xml        # Bed leveling visual
├── screw_indicator.xml          # Screw widget
├── camera_panel.xml             # Webcam view
├── camera_pip.xml               # Picture-in-picture
├── history_panel.xml            # Job history
├── history_item.xml             # History row
├── power_panel.xml              # Power devices
├── power_device_row.xml         # Device toggle
├── input_shaper_panel.xml       # Resonance calibration
├── retraction_panel.xml         # Firmware retraction
├── spoolman_panel.xml           # Filament tracking
├── spool_select_modal.xml       # Spool picker
├── job_queue_panel.xml          # Print queue
├── update_panel.xml             # Software updates
├── timelapse_panel.xml          # Timelapse controls
└── limits_panel.xml             # Velocity/accel
```

### Headers (~15 files)
```
include/
├── temperature_presets.h
├── ui_panel_macros.h
├── ui_panel_console.h
├── ui_panel_screws_tilt.h
├── ui_panel_camera.h
├── webcam_client.h
├── ui_panel_history.h
├── ui_panel_power.h
├── ui_panel_input_shaper.h
├── ui_panel_retraction.h
├── spoolman_client.h
├── ui_panel_spoolman.h
├── ui_panel_job_queue.h
├── ui_panel_updates.h
└── ui_panel_timelapse.h
```

### Sources (~15 files)
```
src/
├── temperature_presets.cpp
├── ui_panel_macros.cpp
├── ui_panel_console.cpp
├── ui_panel_screws_tilt.cpp
├── ui_panel_camera.cpp
├── webcam_client.cpp
├── ui_panel_history.cpp
├── ui_panel_power.cpp
├── ui_panel_input_shaper.cpp
├── ui_panel_retraction.cpp
├── spoolman_client.cpp
├── ui_panel_spoolman.cpp
├── ui_panel_job_queue.cpp
├── ui_panel_updates.cpp
└── ui_panel_timelapse.cpp
```

### Tests (~5 files)
```
tests/unit/
├── test_temperature_presets.cpp
├── test_macro_parser.cpp
├── test_history_api.cpp
├── test_power_api.cpp
└── test_webcam_client.cpp
```

## Files to Modify

### XML
- `ui_xml/globals.xml` - Add coming_soon, new tokens
- `ui_xml/navigation_bar.xml` - New nav icons
- `ui_xml/nozzle_temp_panel.xml` - Preset buttons
- `ui_xml/bed_temp_panel.xml` - Preset buttons
- `ui_xml/print_status_panel.xml` - Layer display
- `ui_xml/home_panel.xml` - Quick access buttons
- `ui_xml/controls_panel.xml` - New sub-panel cards
- `ui_xml/settings_panel.xml` - Power section

### C++
- `src/main.cpp` - Register panels, callbacks
- `include/moonraker_api.h` - New API methods
- `src/moonraker_api.cpp` - Implement APIs
- `config/helixconfig.json.template` - New settings

---

# Document Changelog

- **2025-12-08:** Initial comprehensive research document
  - Compiled from 5 research agents (Moonraker API, competitors, extensions, community, codebase audit)
  - Added complete Moonraker API reference
  - Added competitor deep dives
  - Added extension integration guide
  - Added community pain points
  - Added implementation specifications
  - Added code templates
  - Added file index
