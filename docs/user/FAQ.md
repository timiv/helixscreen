# Frequently Asked Questions

Quick answers to common questions about HelixScreen.

---

## General

### What is HelixScreen?

HelixScreen is a touchscreen interface for Klipper 3D printers. It connects to your Moonraker instance and provides a modern, touch-friendly UI for controlling your printer.

**Key features:**
- 20+ panels covering all printer operations
- 3D G-code preview and bed mesh visualization
- First-run wizard for easy setup
- Dark and light themes
- Designed for embedded displays (low memory, no desktop required)

### Which printers are supported?

**Any printer running Klipper + Moonraker** is supported. HelixScreen connects to Moonraker's API, not directly to Klipper, so it works with:

- Voron (all models)
- Prusa (with Klipper mod)
- Creality (K1, Ender with Klipper)
- Bambu (with Klipper mod)
- FlashForge Adventurer 5M (with [Forge-X](https://github.com/DrA1ex/ff5m))
- Ratrig
- Any custom Klipper build

### Which displays are supported?

- **HDMI displays** (most common, plug and play)
- **Official Raspberry Pi touchscreen** (7" DSI)
- **SPI displays** (with proper configuration)
- **BTT Pad 7** and similar Klipper pads
- **Built-in displays** (like AD5M 4.3" screen)

Minimum resolution: 480x320
Recommended: 800x480 or higher

### How is this different from KlipperScreen and GuppyScreen?

| Feature | HelixScreen | KlipperScreen | GuppyScreen |
|---------|-------------|---------------|-------------|
| **UI Technology** | LVGL 9 XML | GTK (Python) | LVGL 8 (C++) |
| **Memory Usage** | ~50-80MB | ~150-200MB | ~80-100MB |
| **Change UI without rebuild** | Yes (XML) | Yes (Python) | No |
| **Reactive data binding** | Built-in | Manual | Manual |
| **3D G-code preview** | Yes | 2D layers | No |
| **3D bed mesh** | Yes | 2D heatmap | 2D heatmap |
| **Status** | Beta | Stable | Stable |

**HelixScreen advantages:**
- Lower memory footprint
- Declarative XML layouts
- Modern reactive architecture
- 3D visualizations

### Is HelixScreen production-ready?

HelixScreen is currently in **beta**. This means:

- ✅ Core features are complete and tested
- ✅ Daily use on real printers works well
- ⚠️ Some edge cases may have bugs
- ⚠️ Some advanced features still in development

We recommend it for enthusiasts comfortable with:
- SSH access to their printer
- Reading logs for troubleshooting
- Reporting bugs on GitHub

---

## Installation

### What Raspberry Pi do I need?

| Pi Model | Supported | Notes |
|----------|-----------|-------|
| Pi 5 | ✅ Recommended | Best performance |
| Pi 4 | ✅ Recommended | Great performance |
| Pi 3B+ | ✅ Minimum | Works well |
| Pi 3B | ⚠️ Usable | May be slow |
| Pi Zero 2 W | ✅ OK | Good for space-constrained setups |
| Pi Zero (original) | ❌ No | Too slow |

**Memory:** 1GB minimum, 2GB+ recommended.

### Can I run HelixScreen alongside KlipperScreen?

**Not on the same display.** Both compete for the framebuffer. Choose one:

```bash
# Disable KlipperScreen
sudo systemctl stop KlipperScreen
sudo systemctl disable KlipperScreen

# Enable HelixScreen
sudo systemctl enable helixscreen
sudo systemctl start helixscreen
```

If you have two displays, you could run both (advanced configuration).

### Do I need to install X11 or a desktop environment?

**No.** HelixScreen renders directly to the framebuffer (fbdev) or DRM. It doesn't need:
- X11 / Xorg
- Wayland
- Desktop environment (GNOME, KDE, etc.)
- Display manager (LightDM, GDM, etc.)

This is why it uses less memory than alternatives.

### Does this work with MainsailOS, FluiddPi, or other Klipper distros?

Yes! HelixScreen works with any Klipper distribution that includes Moonraker:
- MainsailOS ✅
- FluiddPi ✅
- Custom Klipper installs ✅
- KIAUH installs ✅

The web frontend you use (Mainsail, Fluidd, etc.) doesn't matter - HelixScreen talks to Moonraker.

---

## Features

### Does it support multiple extruders?

**Partially.** Current support:
- ✅ Primary extruder temperature control
- ⚠️ Multi-extruder display (in development via AMS feature)
- ⚠️ Toolchanger support (planned)

### Can I use my webcam?

**Coming soon.** Camera/webcam support is implemented as a "Coming Soon" stub. MJPEG streaming is on the roadmap.

### Does it work with Spoolman?

**Coming soon.** Spoolman integration for filament tracking is planned for a future update.

### Does it support Happy Hare or AFC-Klipper?

**In development.** AMS/multi-material support is being actively developed with:
- ✅ Slot status display on home panel
- 🚧 Full load/unload controls
- 🚧 Filament path visualization

### Can I customize the colors or layout?

**Yes, but it requires editing files:**
- Colors: Edit `ui_xml/globals.xml` (design tokens)
- Layout: Edit XML files in `ui_xml/`
- No recompilation needed for XML changes

A graphical theme editor is not currently available.

### Does it support multiple printers?

**Not currently.** HelixScreen connects to one Moonraker instance. Multi-printer support is on the long-term roadmap.

---

## Usage

### Why are some buttons grayed out?

Buttons are disabled when:
- **Printer disconnected:** Controls/Filament panels disabled for safety
- **Not homed:** Motion controls require homing first
- **Printing:** Some operations locked during active print
- **Temperature too low:** Extrusion requires hot nozzle

### How do I take a screenshot?

**On the device:** Currently requires SSH access and a script.

**Development mode:** Press `S` key when running via SDL2 simulator.

### Why does the setup wizard keep appearing?

The wizard runs when no valid configuration exists. Causes:
1. Config file missing or deleted
2. Config file has invalid JSON
3. Permissions prevent reading config

**Fix:** Check `/opt/helixscreen/helixconfig.json` exists and is valid JSON.

### How do I change the Moonraker address?

**Method 1: Re-run wizard**
```bash
sudo rm /opt/helixscreen/helixconfig.json
sudo systemctl restart helixscreen
```

**Method 2: Edit config**
```bash
sudo nano /opt/helixscreen/helixconfig.json
# Edit moonraker_host and moonraker_port
sudo systemctl restart helixscreen
```

### Why won't my print start?

Common causes:
1. **Klipper not ready:** Check `sudo systemctl status klipper`
2. **Not homed:** Some printers require homing before print
3. **Heater issue:** Klipper may be waiting for temperatures
4. **File error:** G-code may have issues

Check the console panel for error messages.

---

## Troubleshooting

### The UI is very slow

1. **Check CPU usage:** `top` - is something else using CPU?
2. **Reduce logging:** Remove `-vv` from service if present
3. **Disable animations:** Set `animations_enabled: false` in config
4. **Check memory:** `free -h` - swapping causes slowness

### Touch doesn't work

1. **Check input device:** `ls /dev/input/event*`
2. **Test manually:** `sudo evtest /dev/input/event0`
3. **Specify device:** Add `"touch_device": "/dev/input/event1"` to config

### Screen is black

1. **Check service:** `sudo systemctl status helixscreen`
2. **Check logs:** `sudo journalctl -u helixscreen -n 50`
3. **Check framebuffer:** `ls /dev/fb*` or `ls /dev/dri/*`
4. **Try specifying device:** Add `"drm_device": "/dev/dri/card1"` to config

### Can't connect to Moonraker

1. **Check Moonraker:** `sudo systemctl status moonraker`
2. **Test manually:** `curl http://localhost:7125/printer/info`
3. **Check firewall:** `sudo ufw status`
4. **Verify IP:** `hostname -I`

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more solutions.

---

## Development & Contributing

### Is HelixScreen open source?

Yes! HelixScreen is licensed under **GPL v3**. Source code is on GitHub.

### How can I contribute?

See the [Contributing Guide](../CONTRIBUTING.md) for:
- Code standards
- Development setup
- Pull request process

We welcome:
- Bug reports
- Feature suggestions
- Code contributions
- Documentation improvements

### How do I build from source?

```bash
# Clone repository
git clone https://github.com/prestonbrown/helixscreen.git
cd helixscreen

# Install dependencies
make check-deps
make install-deps

# Build
make -j

# Run in test mode
./build/bin/helix-screen --test
```

See [DEVELOPMENT.md](../DEVELOPMENT.md) for complete instructions.

### What programming language is HelixScreen?

- **C++17** for application logic
- **XML** for UI layouts (LVGL 9 declarative system)
- **Makefile** for build system
- **Bash** for scripts

---

## Getting Help

### Where can I report bugs?

Open an issue on [GitHub Issues](https://github.com/prestonbrown/helixscreen/issues) with:
- HelixScreen version
- Hardware info (Pi model, display)
- Steps to reproduce
- Relevant log output

### Where can I request features?

Open a GitHub issue with the "enhancement" label. Describe:
- What you want
- Why it's useful
- How you imagine it working

### Is there a Discord or forum?

Currently, GitHub Issues and Discussions are the primary communication channels.

### Where are the logs?

```bash
# systemd journal (MainsailOS)
sudo journalctl -u helixscreen -f

# If using file logging
cat /var/log/helix-screen.log
# or
cat ~/.local/share/helix-screen/helix.log
```

---

*For more details, see:*
- *[Installation Guide](INSTALL.md)*
- *[User Guide](USER_GUIDE.md)*
- *[Configuration Reference](CONFIGURATION.md)*
- *[Troubleshooting](TROUBLESHOOTING.md)*
