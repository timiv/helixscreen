<p align="center">
  <img src="assets/images/helix-icon-64.png" alt="HelixScreen" width="128"/>
  <br>
  <h1 align="center">HelixScreen</h1>
  <p align="center"><em>A modern, lightweight touch interface for Klipper/Moonraker 3D printers</em></p>
</p>

<p align="center">
  <a href="https://github.com/prestonbrown/helixscreen/actions/workflows/build.yml"><img src="https://github.com/prestonbrown/helixscreen/actions/workflows/build.yml/badge.svg?branch=main" alt="Build"></a>
  <a href="https://github.com/prestonbrown/helixscreen/actions/workflows/quality.yml"><img src="https://github.com/prestonbrown/helixscreen/actions/workflows/quality.yml/badge.svg?branch=main" alt="Code Quality"></a>
  <a href="https://www.gnu.org/licenses/gpl-3.0"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPL v3"></a>
  <a href="https://lvgl.io/"><img src="https://img.shields.io/badge/LVGL-9.4.0-green.svg" alt="LVGL"></a>
  <img src="https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey.svg" alt="Platform">
  <a href="https://en.cppreference.com/w/cpp/17"><img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17"></a>
</p>

HelixScreen is a next-generation printer control interface built from the ground up using LVGL 9's declarative XML system. Designed for embedded hardware with limited resources, it brings advanced Klipper features to printers that ship with restrictive vendor UIs.

---

> ## 🎉 We're Getting Close!
>
> **Status: Beta — Seeking Testers**
>
> Core features are complete and tested. We're looking for early adopters to help find edge cases before wider release.
>
> **Tested on:**
> - Raspberry Pi 5 with 5" touchscreen
> - FlashForge Adventurer 5M Pro ([Forge-X](https://github.com/DrA1ex/ff5m) firmware)
>
> **Ready to help?** See [Getting Started](#quick-start). Issues and feedback welcome!

---

**Quick Links:** [Features](#key-features) · [Screenshots](#screenshots) · [Getting Started](#quick-start) · [Requirements](#requirements) · [Documentation](#documentation) · [FAQ](#faq) · [Contributing](docs/CONTRIBUTING.md) · [Roadmap](docs/ROADMAP.md)

---

**Built on proven foundations:**
- Architecture inspired by [GuppyScreen](https://github.com/ballaswag/guppyscreen) with modern C++17 rewrite
- Modern XML-based UI with reactive data binding (LVGL 9.4)

## Why HelixScreen?

- 🎯 **Declarative XML UI** - Complete UI in XML files, change layouts without recompiling
- 🔄 **Reactive Data Binding** - Subject-Observer pattern for automatic UI updates
- 💾 **Resource Efficient** - ~50-80MB footprint, runs on constrained hardware
- 🏗️ **Modern C++17** - Type-safe architecture with RAII memory management

### Feature Comparison

| Feature | HelixScreen | GuppyScreen | KlipperScreen |
|---------|-------------|-------------|---------------|
| **UI Framework** | LVGL 9 XML | LVGL 8 C | GTK 3 (Python) |
| **Declarative UI** | ✅ Full XML | ❌ C only | ❌ Python only |
| **No Recompile Changes** | ✅ XML edits | ❌ Need rebuild | ✅ Python edits |
| **Memory Footprint** | ~50-80MB | ~60-80MB | ~150-200MB |
| **Reactive Data Binding** | ✅ Built-in | ⚠️ Manual | ⚠️ Manual |
| **Theme System** | ✅ XML globals | ✅ Built-in themes | ✅ CSS-like |
| **Embedded Target** | ✅ Optimized | ✅ Optimized | ⚠️ Heavy |
| **Touch Optimization** | ✅ Native | ✅ Native | ⚠️ Desktop-first |
| **Responsive Design** | ✅ Breakpoints | ✅ Multi-resolution | ⚠️ Fixed layouts |
| **Development Status** | 🚧 Beta | ✅ Stable | ✅ Mature |
| **Backend** | libhv WebSocket | libhv WebSocket | Python WebSocket |
| **Language** | C++17 | C | Python 3 |
| **Build Time** | ~30s clean | ~25s clean | N/A (interpreted) |
| **First-Run Setup** | ✅ Auto-wizard | ⚠️ Manual config | ⚠️ Manual config |
| **G-code Preview** | ✅ Layer view | ⚠️ Thumbnails | ⚠️ Thumbnails |
| **Bed Mesh Visual** | ✅ 3D gradient | ✅ Color-coded | ✅ 2D heatmap |
| **Multi-Fan Control** | ✅ All fans | ✅ Configurable | ✅ All fans |

**Legend:** ✅ Full support | ⚠️ Partial/Limited | ❌ Not available | 🚧 In development

## Screenshots

### Home Panel
<img src="docs/images/screenshot-home-panel.png" alt="Home Panel" width="800"/>

*Main dashboard showing printer status, AMS filament status, temperatures, and quick actions*

### Controls Panel
<img src="docs/images/screenshot-controls-panel.png" alt="Controls Panel" width="800"/>

*Card-based control center with Quick Actions, Temperatures, Multi-Fan Cooling, Filament presets, and Calibration tools*

### Print File Browser
<img src="docs/images/screenshot-print-select-card.png" alt="Print Select Panel" width="800"/>

*File selection with 3D thumbnail preview, print time estimates, filament requirements, and pre-print options*

### Motion Controls
<img src="docs/images/screenshot-motion-panel.png" alt="Motion Control Panel" width="800"/>

*Manual printer control with jog pad, distance selector, and live position display*

### Bed Mesh Visualization
<img src="docs/images/screenshot-bed-mesh-panel.png" alt="Bed Mesh Panel" width="800"/>

*3D bed mesh visualization with gradient coloring, mesh profiles, and calibration controls*

### Settings Panel
<img src="docs/images/screenshot-settings-panel.png" alt="Settings Panel" width="800"/>

*Comprehensive settings for appearance, input, printer configuration, and more*

### First-Run Wizard
<img src="docs/images/screenshot-wizard-wifi.png" alt="Setup Wizard" width="800"/>

*Guided 7-step setup with WiFi configuration and auto-discovery of printer components*

### G-Code Viewer
<img src="docs/images/screenshot-gcode-viewer.png" alt="G-Code Viewer" width="800"/>

*G-code layer preview with slider navigation and print progress visualization*

> 📸 **Note:** Screenshots auto-generated with `./scripts/screenshot.sh` - regenerate after UI changes

## Key Features

### Printer Control
- **Print Management** - Start, pause, resume, cancel with live progress tracking
- **Motion Controls** - XYZ jog pad, homing, emergency stop with safety confirmation
- **Temperature Control** - Nozzle/bed presets, custom targets, live graphs, heating animations
- **Fan Control** - Part fan, hotend fan, controller fan, and auxiliary fans
- **Z-Offset** - Baby stepping during prints with real-time adjustment

### Multi-Material Support (5 Backends)
- **AFC (Box Turtle)** - Full Aero Filament Changer integration
- **Happy Hare** - ERCF, 3MS, Tradrack, Night Owl support
- **Tool Changer** - Prusa/Bambu-style multi-nozzle systems
- **ValgACE** - REST API integration
- **Slot Management** - Color picker, load/unload, Spoolman linking

### Visualization
- **G-code Layer View** - 2D isometric preview with layer slider
- **3D Bed Mesh** - Gradient-colored surface visualization with profiles
- **Print Thumbnails** - Cached previews from slicer metadata

### Calibration & Tuning
- **Input Shaper** - Resonance testing with ADXL375 support
- **Bed Mesh** - Profile management, calibration, visualization
- **Screws Tilt Adjust** - Guided bed leveling assistance
- **PID Tuning** - Heater calibration interface
- **Firmware Retraction** - Retraction/Z-hop configuration

### Integrations
- **Spoolman** - Spool tracking and management
- **Power Devices** - Smart relay/PSU control
- **Print History** - Statistics, filtering, job logs
- **Timelapse** - Recording configuration (plugin required)
- **Exclude Objects** - Skip objects mid-print

### System
- **First-Run Wizard** - 8-step guided setup with auto-discovery
- **20+ Panels** - Home, Controls, Motion, Temps, Filament, Settings, Advanced, and more
- **Light/Dark Themes** - Runtime switching with design token system
- **Responsive Design** - 480×320 to 1024×600+ screen sizes
- **Connection-Aware** - Graceful disconnect handling with auto-reconnect

## Target Hardware

- **Raspberry Pi** (Pi 3/4/5, Zero 2 W)
- **BTT Pad 7** / similar touch displays
- **Vendor printer displays** (Creality K1/K1 Max, FlashForge AD5M, etc.)
- **Generic Linux ARM/x64** with framebuffer support
- **Development simulator:** macOS/Linux desktop with SDL2

## Requirements

### Software Requirements
- **Klipper** - Any recent version (tested with 0.11.0+)
- **Moonraker** - Any recent version (tested with 0.8.0+)
- **Operating System:** Linux (Debian/Ubuntu/Arch) or macOS (for development only)
- **Build Tools:** CMake 3.15+, C++17 compiler (GCC 8+ / Clang 10+), Make, Python 3, npm

### Hardware Requirements (Target Embedded Devices)
- **CPU:** ARM Cortex-A7+ or x86_64 (500MHz+)
- **RAM:** 128MB minimum, 256MB+ recommended
- **Storage:** 50MB for application + 20MB for assets
- **Display:** 480×320 minimum, 800×480+ recommended
- **Touch:** Capacitive or resistive touchscreen
- **Network:** WiFi or Ethernet for Moonraker connection

### Development Requirements (SDL2 Simulator)
- **macOS:** 10.15+ (Catalina) or **Linux:** Any modern distribution
- **RAM:** 512MB+ available
- **Display:** Any resolution (will simulate target screen size)

## Quick Start

### Install Dependencies

**Automated setup:**
```bash
make check-deps     # Check what's missing
make install-deps   # Auto-install missing dependencies (interactive)
```

**Manual setup (macOS):**
```bash
brew install cmake python3 node
npm install  # Install lv_font_conv and lv_img_conv
# Optional (auto-built if missing): brew install sdl2 spdlog libhv
```

**Manual setup (Debian/Ubuntu):**
```bash
sudo apt install cmake python3 python3-venv clang make npm
npm install  # Install lv_font_conv and lv_img_conv
# Optional (auto-built if missing): sudo apt install libsdl2-dev spdlog libhv-dev
```

### Build & Run

```bash
# Build (parallel, auto-detects CPU cores)
make -j

# Run simulator (production mode - requires real hardware/printer)
./build/bin/helix-screen

# Run in test mode (all components mocked - no hardware needed)
./build/bin/helix-screen --test

# Test mode with selective real components
./build/bin/helix-screen --test --real-moonraker      # Real printer, mock network
./build/bin/helix-screen --test --real-wifi --real-files  # Real WiFi/files, mock rest

# Controls: Click navigation icons, press 'S' for screenshot
```

### Cross-Compilation (Embedded Targets)

Build for embedded ARM targets using Docker (no toolchain installation required):

```bash
# Build for Raspberry Pi (aarch64)
make pi-docker

# Build for Flashforge Adventurer 5M (armv7-a)
make ad5m-docker

# View all cross-compilation options
make cross-info
```

Docker images are **automatically built** on first use. See [BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md#cross-compilation-embedded-targets) for details.

### Test Mode

Development without hardware:

```bash
./build/bin/helix-screen --test                    # All components mocked
./build/bin/helix-screen --test --real-moonraker   # Real printer, mock WiFi
```

Flags: `--real-wifi`, `--real-ethernet`, `--real-moonraker`, `--real-files`

## Project Status

Beta — core features complete, ~50-80MB memory footprint. See [ROADMAP.md](docs/ROADMAP.md) for feature timeline.

## FAQ

**Q: Is HelixScreen production-ready?**
A: Beta status. Core features work, but we're seeking testers to find edge cases. Suitable for enthusiasts willing to provide feedback.

**Q: How is this different from GuppyScreen/KlipperScreen?**
A: HelixScreen uses LVGL 9's declarative XML for UI definition—change layouts without recompiling. Lower memory footprint than KlipperScreen (~50-80MB vs ~150-200MB). See [comparison table](#feature-comparison).

**Q: Which printers are supported?**
A: Any printer running Klipper + Moonraker. The wizard auto-discovers your printer's capabilities.

**Q: What multi-material systems work?**
A: AFC (Box Turtle), Happy Hare (ERCF, 3MS, Tradrack), tool changers, and ValgACE. See [Key Features](#multi-material-support-5-backends).

**Q: How do I contribute?**
A: See [CONTRIBUTING.md](docs/CONTRIBUTING.md). Issues and PRs welcome!

## Troubleshooting

| Issue | Solution |
|-------|----------|
| CMake/SDL2 not found | Run `make install-deps` or see [Quick Start](#install-dependencies) |
| Submodule empty | `git submodule update --init --recursive` |
| Can't connect to Moonraker | Check IP/port in helixconfig.json, verify Moonraker is running |
| Wizard not showing | Delete helixconfig.json to trigger it |

For more, see [DEVELOPMENT.md](docs/DEVELOPMENT.md) or open a [GitHub issue](https://github.com/prestonbrown/helixscreen/issues).

## Architecture

```
XML Layout (ui_xml/*.xml)
    ↓ bind_text / bind_value / bind_flag
Reactive Subjects (lv_subject_t)
    ↓ lv_subject_set_* / copy_*
C++ Application Logic (src/*.cpp)
```

**Key Innovation:** The entire UI is defined in XML files. C++ code only handles initialization and reactive data updates—zero layout or styling logic.

## Documentation

### Getting Started
- **[Quick Start](#quick-start)** - Build and run in 5 minutes
- **[Requirements](#requirements)** - Software and hardware requirements
- **[Test Mode](#test-mode)** - Development without hardware

### User Guides
- **[FAQ](#faq)** - Frequently asked questions
- **[Project Status](#project-status)** - Current development phase and roadmap
- **[ROADMAP.md](docs/ROADMAP.md)** - Detailed feature timeline and milestones

### Development
- **[DEVELOPMENT.md](docs/DEVELOPMENT.md)** - Build system and daily workflow
- **[CONTRIBUTING.md](docs/CONTRIBUTING.md)** - Code standards and PR process
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - System design and patterns

### Technical Reference
- **[LVGL 9 XML Guide](docs/LVGL9_XML_GUIDE.md)** - Complete XML syntax reference
- **[Quick Reference](docs/QUICK_REFERENCE.md)** - Common patterns and code snippets
- **[BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md)** - Build configuration and patches
- **[Testing Guide](docs/TESTING.md)** - Test infrastructure and Catch2 usage

## License

GPL v3 - See individual source files for copyright headers.

## Acknowledgments

**HelixScreen builds upon:**
- **[GuppyScreen](https://github.com/ballaswag/guppyscreen)** - Core architecture patterns, Moonraker WebSocket integration, and printer state management
- **[KlipperScreen](https://github.com/KlipperScreen/KlipperScreen)** - Feature inspiration, UI design concepts, and workflow patterns

**Technology Stack:**
- **[LVGL 9.4](https://lvgl.io/)** - Light and Versatile Graphics Library with XML support
- **[Klipper](https://www.klipper3d.org/)** - Advanced 3D printer firmware
- **[Moonraker](https://github.com/Arksine/moonraker)** - Klipper API server and WebSocket interface
- **[libhv](https://github.com/ithewei/libhv)** - High-performance network library for WebSocket client
- **[spdlog](https://github.com/gabime/spdlog)** - Fast C++ logging library
- **[SDL2](https://www.libsdl.org/)** - Cross-platform development and simulation
