# Configuration Reference

Complete reference for HelixScreen configuration options.

---

## Table of Contents

- [Configuration File Location](#configuration-file-location)
- [Configuration Structure](#configuration-structure)
- [General Settings](#general-settings)
- [Display Settings](#display-settings)
- [Input Settings](#input-settings)
- [Network Settings](#network-settings)
- [Printer Settings](#printer-settings)
- [Moonraker Settings](#moonraker-settings)
- [G-code Viewer Settings](#g-code-viewer-settings)
- [Cache Settings](#cache-settings)
- [AMS Settings](#ams-settings)
- [Safety Limits](#safety-limits)
- [Capability Overrides](#capability-overrides)
- [Resetting Configuration](#resetting-configuration)
- [Command-Line Options](#command-line-options)
- [Environment Variables](#environment-variables)

---

## Configuration File Location

| Platform | Location |
|----------|----------|
| MainsailOS (Pi) | `/opt/helixscreen/config/helixconfig.json` |
| AD5M Forge-X | `/opt/helixscreen/config/helixconfig.json` |
| AD5M Klipper Mod | `/root/printer_software/helixscreen/config/helixconfig.json` |
| K1 Simple AF | `/usr/data/helixscreen/config/helixconfig.json` |
| Development | `./config/helixconfig.json` (in config/ directory) |

The configuration file is created automatically by the first-run wizard. You can also copy from the template:

```bash
cp config/helixconfig.json.template config/helixconfig.json
```

**Note:** Legacy config locations (`helixconfig.json` in app root or `/opt/helixscreen/helixconfig.json`) are automatically migrated to the new location on startup.

---

## Configuration Structure

The configuration file is JSON format with several top-level sections:

```json
{
  "config_path": "config/helixconfig.json",
  "animations_enabled": true,
  "dark_mode": true,
  "display_rotate": 0,
  "display_sleep_sec": 600,
  "log_dest": "auto",
  "log_path": "",

  "display": { ... },
  "input": { ... },
  "network": { ... },
  "printer": { ... },
  "gcode_viewer": { ... },
  "ams": { ... }
}
```

---

## General Settings

### `config_path`
**Type:** string
**Default:** `"helixconfig.json"`
**Description:** Path to this config file (for self-reference).

### `animations_enabled`
**Type:** boolean
**Default:** `true`
**Description:** Enable UI animations and transitions. Disable for lower-end hardware.

### `dark_mode`
**Type:** boolean
**Default:** `false`
**Description:** Use dark theme (`true`) or light theme (`false`). Can also be set via Settings panel.

### `display_rotate`
**Type:** integer
**Default:** `0`
**Values:** `0`, `90`, `180`, `270`
**Description:** Rotate the entire display by specified degrees.

### `display_sleep_sec`
**Type:** integer
**Default:** `600`
**Description:** Seconds of inactivity before screen turns off. Set to `0` to disable.

### `display_dim_sec`
**Type:** integer
**Default:** `300`
**Description:** Seconds of inactivity before screen dims (must be less than `display_sleep_sec`). Set to `0` to disable dimming.

### `display_dim_brightness`
**Type:** integer
**Default:** `30`
**Range:** `1` - `100`
**Description:** Brightness percentage when screen is dimmed.

### `time_format`
**Type:** integer
**Default:** `0`
**Values:** `0` (12-hour), `1` (24-hour)
**Description:** Time display format. `0` shows "2:30 PM", `1` shows "14:30".

### `log_dest`
**Type:** string
**Default:** `"auto"`
**Values:** `"auto"`, `"journal"`, `"syslog"`, `"file"`, `"console"`
**Description:** Log destination:
- `auto` - Detect best option (journal on systemd, console otherwise)
- `journal` - systemd journal (view with `journalctl -u helixscreen`)
- `syslog` - Traditional syslog
- `file` - Write to log file
- `console` - Print to stdout/stderr

### `log_path`
**Type:** string
**Default:** `""`
**Description:** Path for log file when `log_dest` is `"file"`. Empty uses default location:
- `/var/log/helix-screen.log` (if writable)
- `~/.local/share/helix-screen/helix.log` (fallback)

---

## Display Settings

Located in the `display` section:

```json
{
  "display": {
    "drm_device": "",
    "touch_device": ""
  }
}
```

### `drm_device`
**Type:** string
**Default:** `""` (auto-detect)
**Example:** `"/dev/dri/card1"`
**Description:** Override DRM device for display output. Leave empty for auto-detection.

**Pi 5 DRM devices:**
- `/dev/dri/card0` - v3d (3D only, no display output)
- `/dev/dri/card1` - DSI touchscreen
- `/dev/dri/card2` - HDMI (vc4)

### `touch_device`
**Type:** string
**Default:** `""` (auto-detect)
**Example:** `"/dev/input/event1"`
**Description:** Override touch/pointer input device. Leave empty for auto-detection via libinput.

---

## Input Settings

Located in the `input` section:

```json
{
  "input": {
    "scroll_throw": 25,
    "scroll_limit": 5
  }
}
```

### `scroll_throw`
**Type:** integer
**Default:** `25`
**Range:** `1` - `99`
**Description:** Scroll momentum decay rate. Higher values = faster decay (less "throw"). Default LVGL is 10; we use 25 for better touchscreen feel.

### `scroll_limit`
**Type:** integer
**Default:** `5`
**Range:** `1` - `50`
**Description:** Pixels of movement required before scrolling starts. Lower = more responsive. Default LVGL is 10; we use 5.

---

## Network Settings

Located in the `network` section:

```json
{
  "network": {
    "connection_type": "wifi",
    "wifi_ssid": "MyNetwork",
    "eth_ip": ""
  }
}
```

### `connection_type`
**Type:** string
**Default:** `"None"`
**Values:** `"None"`, `"wifi"`, `"ethernet"`
**Description:** Current network connection type.

### `wifi_ssid`
**Type:** string
**Default:** `""`
**Description:** Connected WiFi network SSID.

### `eth_ip`
**Type:** string
**Default:** `""`
**Description:** Ethernet IP address (when connected).

---

## Printer Settings

Located in the `printer` section:

```json
{
  "printer": {
    "name": "My Printer",
    "type": "Voron 2.4",
    "heaters": {
      "hotend": "extruder",
      "bed": "heater_bed"
    },
    "temp_sensors": {
      "hotend": "extruder",
      "bed": "heater_bed"
    },
    "fans": {
      "part": "fan",
      "hotend": "heater_fan hotend_fan"
    },
    "leds": {
      "strip": "neopixel chamber_light"
    },
    "extra_sensors": {},
    "hardware": {
      "optional": [],
      "expected": [],
      "last_snapshot": {}
    }
  }
}
```

> **Breaking Change (Jan 2026):** The config schema has changed from singular keys (`heater`, `sensor`, `fan`, `led`) to plural keys (`heaters`, `temp_sensors`, `fans`, `leds`). If upgrading from an older version, delete your config file and re-run the first-run wizard.

### `name`
**Type:** string
**Description:** Display name for your printer.

### `type`
**Type:** string
**Description:** Printer model/type for feature detection.

### `heaters.hotend`
**Type:** string
**Default:** `"extruder"`
**Description:** Klipper heater name for hotend.

### `heaters.bed`
**Type:** string
**Default:** `"heater_bed"`
**Description:** Klipper heater name for heated bed.

### `temp_sensors.hotend`
**Type:** string
**Description:** Temperature sensor for hotend (typically same as heater name).

### `temp_sensors.bed`
**Type:** string
**Description:** Temperature sensor for bed (typically same as heater name).

### `fans.part`
**Type:** string
**Default:** `"fan"`
**Description:** Klipper fan name for part cooling.

### `fans.hotend`
**Type:** string
**Description:** Klipper fan name for hotend cooling.

### `leds.strip`
**Type:** string
**Default:** `"None"`
**Description:** Klipper LED strip name, or `"None"` if no controllable LEDs.

### `extra_sensors`
**Type:** object
**Default:** `{}`
**Description:** Additional temperature sensors to monitor (beyond hotend/bed). Keys are display names, values are Klipper sensor names.

### `hardware`
**Type:** object
**Description:** Hardware tracking information (managed automatically by the wizard):
- `optional` - List of optional hardware detected
- `expected` - List of expected hardware based on printer type
- `last_snapshot` - Last hardware state snapshot for change detection

---

## Moonraker Settings

Connection settings are in the `printer` section:

```json
{
  "printer": {
    "moonraker_host": "192.168.1.100",
    "moonraker_port": 7125,
    "moonraker_api_key": false,
    "moonraker_connection_timeout_ms": 10000,
    "moonraker_request_timeout_ms": 30000,
    "moonraker_keepalive_interval_ms": 10000,
    "moonraker_reconnect_min_delay_ms": 200,
    "moonraker_reconnect_max_delay_ms": 2000,
    "moonraker_timeout_check_interval_ms": 2000
  }
}
```

### `moonraker_host`
**Type:** string
**Default:** `"localhost"`
**Description:** Moonraker hostname or IP address.

### `moonraker_port`
**Type:** integer
**Default:** `7125`
**Description:** Moonraker port number.

### `moonraker_api_key`
**Type:** string or false
**Default:** `false`
**Description:** API key if Moonraker authentication is enabled. Set to `false` if no authentication.

### `moonraker_connection_timeout_ms`
**Type:** integer
**Default:** `10000`
**Description:** Connection timeout in milliseconds.

### `moonraker_request_timeout_ms`
**Type:** integer
**Default:** `30000`
**Description:** Request timeout for Moonraker API calls.

### `moonraker_keepalive_interval_ms`
**Type:** integer
**Default:** `10000`
**Description:** Interval for WebSocket keepalive pings.

### `moonraker_reconnect_min_delay_ms`
**Type:** integer
**Default:** `200`
**Description:** Minimum delay before reconnection attempt.

### `moonraker_reconnect_max_delay_ms`
**Type:** integer
**Default:** `2000`
**Description:** Maximum delay before reconnection attempt (exponential backoff cap).

---

## G-code Viewer Settings

Located in the `gcode_viewer` section:

```json
{
  "gcode_viewer": {
    "shading_model": "smooth",
    "tube_sides": 4,
    "streaming_mode": "auto",
    "streaming_threshold_percent": 40
  }
}
```

### `shading_model`
**Type:** string
**Default:** `"smooth"`
**Values:** `"flat"`, `"smooth"`, `"phong"`
**Description:** 3D rendering quality:
- `flat` - Faceted look, lowest GPU cost
- `smooth` - Gouraud shading, good balance (default)
- `phong` - Per-pixel lighting, highest quality

### `tube_sides`
**Type:** integer
**Default:** `4`
**Values:** `4`, `8`, `16`
**Description:** Cross-section detail for filament paths:
- `4` - Diamond shape, fastest rendering
- `8` - Octagonal, balanced quality
- `16` - Circular, matches OrcaSlicer quality

### `streaming_mode`
**Type:** string
**Default:** `"auto"`
**Values:** `"auto"`, `"on"`, `"off"`
**Description:** Large G-code file handling:
- `auto` - Stream files that would use too much RAM
- `on` - Always stream (lowest memory)
- `off` - Always load full file (fastest viewing)

### `streaming_threshold_percent`
**Type:** integer
**Default:** `40`
**Range:** `1` - `90`
**Description:** Percent of available RAM that triggers streaming mode. Lower values stream smaller files. Only used when `streaming_mode` is `"auto"`.

---

## Cache Settings

Located in the `cache` section:

```json
{
  "cache": {
    "thumbnail_max_mb": 20,
    "disk_critical_mb": 5,
    "disk_low_mb": 20
  }
}
```

### `thumbnail_max_mb`
**Type:** integer
**Default:** `20`
**Description:** Maximum thumbnail cache size in MB. Cache auto-sizes to 5% of available disk, capped at this limit.

### `disk_critical_mb`
**Type:** integer
**Default:** `5`
**Description:** Stop caching when available disk falls below this threshold (MB). Prevents filling filesystem.

### `disk_low_mb`
**Type:** integer
**Default:** `20`
**Description:** Evict cache aggressively when available disk falls below this threshold (MB).

---

## AMS Settings

Located in the `ams` section:

```json
{
  "ams": {
    "spool_style": "3d"
  }
}
```

### `spool_style`
**Type:** string
**Default:** `"3d"`
**Values:** `"3d"`, `"flat"`
**Description:** Filament spool visualization style:
- `3d` - Bambu-style pseudo-3D canvas with gradients
- `flat` - Simple concentric rings

---

## Safety Limits

Located in `printer.safety_limits`:

```json
{
  "printer": {
    "safety_limits": {
    "max_temperature_celsius": 400.0,
    "min_temperature_celsius": 0.0,
    "max_fan_speed_percent": 100.0,
    "min_fan_speed_percent": 0.0,
    "max_feedrate_mm_min": 50000.0,
    "min_feedrate_mm_min": 0.0,
    "max_relative_distance_mm": 1000.0,
    "min_relative_distance_mm": -1000.0,
    "max_absolute_position_mm": 1000.0,
      "min_absolute_position_mm": 0.0
    }
  }
}
```

These override auto-detected limits. Useful for:
- High-temp printers (increase `max_temperature_celsius`)
- Very large printers (increase position limits)
- Safety restrictions (decrease maximums)

Leave unset (or remove the section) to use Moonraker auto-detection from printer.cfg.

---

## Capability Overrides

Located in `printer.capability_overrides`:

```json
{
  "printer": {
    "capability_overrides": {
    "bed_leveling": "auto",
    "qgl": "auto",
    "z_tilt": "auto",
    "nozzle_clean": "auto",
    "heat_soak": "auto",
      "chamber": "auto"
    }
  }
}
```

**Values for each setting:**
- `"auto"` - Use Moonraker detection
- `"enable"` - Force feature on
- `"disable"` - Force feature off

**Use cases:**
- Enable `heat_soak` when you have a chamber but no chamber heater (soak macro works without)
- Disable `qgl` on a printer where it's defined but not used
- Enable `bed_leveling` if detection failed

---

## Resetting Configuration

### Full Reset
Delete the config file and restart:
```bash
sudo rm /opt/helixscreen/helixconfig.json
sudo systemctl restart helixscreen
```

This triggers the first-run wizard.

### Partial Reset
Edit the config file directly:
```bash
sudo nano /opt/helixscreen/helixconfig.json
```

Or copy fresh from template:
```bash
sudo cp /opt/helixscreen/config/helixconfig.json.template /opt/helixscreen/helixconfig.json
```

---

## Command-Line Options

HelixScreen accepts command-line options for overriding configuration and debugging.

### Display Options

| Option | Description |
|--------|-------------|
| `-s, --size <size>` | Screen size: `tiny` (480×320), `small` (800×480), `medium` (1024×600), `large` (1280×720) |
| `--dpi <n>` | Display DPI (50-500, default: 160) |
| `--dark` | Use dark theme |
| `--light` | Use light theme |
| `--skip-splash` | Skip splash screen on startup |

### Navigation Options

| Option | Description |
|--------|-------------|
| `-p, --panel <panel>` | Start on specific panel (home, controls, filament, settings, advanced, print-select) |
| `-w, --wizard` | Force first-run configuration wizard |

### Connection Options

| Option | Description |
|--------|-------------|
| `--moonraker <url>` | Override Moonraker URL (e.g., `ws://192.168.1.112:7125`) |

### Logging Options

| Option | Description |
|--------|-------------|
| `-v, --verbose` | Increase verbosity (`-v`=info, `-vv`=debug, `-vvv`=trace) |
| `--log-dest <dest>` | Log destination: `auto`, `journal`, `syslog`, `file`, `console` |
| `--log-file <path>` | Log file path (when `--log-dest=file`) |

### Utility Options

| Option | Description |
|--------|-------------|
| `--screenshot [sec]` | Take screenshot after delay (default: 2 seconds) |
| `-t, --timeout <sec>` | Auto-quit after specified seconds (1-3600) |
| `-h, --help` | Show help message |
| `-V, --version` | Show version information |

### Examples

```bash
# Start in dark mode on the settings panel
helix-screen --dark --panel settings

# Override Moonraker connection
helix-screen --moonraker ws://192.168.1.50:7125

# Enable debug logging
helix-screen -vv

# Take screenshot after 5 seconds
helix-screen --screenshot 5
```

> **Note:** Test mode options (`--test`, `--real-*`) are for development only and not documented here.

---

## Environment Variables

These can be set in the systemd service file or before running the binary:

| Variable | Description |
|----------|-------------|
| `HELIX_DRM_DEVICE` | Override DRM device path (e.g., `/dev/dri/card1`) |
| `HELIX_TOUCH_DEVICE` | Override touch input device (e.g., `/dev/input/event1`) |
| `HELIX_DISPLAY_BACKEND` | Override display backend (`drm`, `fbdev`, `sdl`) |

**Example in service file:**
```ini
[Service]
Environment="HELIX_DRM_DEVICE=/dev/dri/card1"
Environment="HELIX_TOUCH_DEVICE=/dev/input/event0"
```

> **Note:** Most users won't need environment variables. The config file options for `display.drm_device` and `display.touch_device` are preferred. Environment variables are mainly for debugging when the config file isn't accessible.

---

## Example Complete Configuration

```json
{
  "config_path": "config/helixconfig.json",
  "animations_enabled": true,
  "dark_mode": true,
  "display_rotate": 0,
  "display_sleep_sec": 300,
  "log_dest": "journal",
  "log_path": "",

  "display": {
    "drm_device": "",
    "touch_device": ""
  },

  "input": {
    "scroll_throw": 25,
    "scroll_limit": 5
  },

  "network": {
    "connection_type": "wifi",
    "wifi_ssid": "PrinterNetwork",
    "eth_ip": ""
  },

  "printer": {
    "name": "Voron 2.4 350",
    "type": "Voron 2.4",
    "moonraker_host": "localhost",
    "moonraker_port": 7125,
    "moonraker_api_key": false,
    "moonraker_connection_timeout_ms": 10000,
    "moonraker_request_timeout_ms": 30000,
    "heaters": {
      "hotend": "extruder",
      "bed": "heater_bed"
    },
    "temp_sensors": {
      "hotend": "extruder",
      "bed": "heater_bed"
    },
    "fans": {
      "part": "fan",
      "hotend": "heater_fan hotend_fan"
    },
    "leds": {
      "strip": "caselight"
    },
    "extra_sensors": {},
    "hardware": {
      "optional": [],
      "expected": [],
      "last_snapshot": {}
    }
  },

  "gcode_viewer": {
    "shading_model": "smooth",
    "tube_sides": 8
  },

  "ams": {
    "spool_style": "3d"
  }
}
```

---

*Back to: [User Guide](USER_GUIDE.md) | [Troubleshooting](TROUBLESHOOTING.md)*
