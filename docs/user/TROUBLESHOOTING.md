# Troubleshooting Guide

Solutions to common problems with HelixScreen.

---

## Table of Contents

- [Connection Issues](#connection-issues)
- [Display Issues](#display-issues)
- [Touch Input Issues](#touch-input-issues)
- [Print Issues](#print-issues)
- [Performance Issues](#performance-issues)
- [Configuration Issues](#configuration-issues)
- [Gathering Diagnostic Information](#gathering-diagnostic-information)
- [Getting Help](#getting-help)

---

## Connection Issues

### "Cannot connect to Moonraker"

**Symptoms:**
- Red connection error on home screen
- "Disconnected" status
- Cannot control printer

**Causes:**
1. Wrong IP address or port
2. Moonraker not running
3. Firewall blocking connection
4. Network issues

**Solutions:**

**Check Moonraker is running:**
```bash
sudo systemctl status moonraker
```
If stopped: `sudo systemctl start moonraker`

**Verify the IP address:**
```bash
# On the Pi
hostname -I
```
Update helixconfig.json with correct IP.

**Test connection manually:**
```bash
curl http://localhost:7125/printer/info
```
Should return JSON with printer info.

**Check firewall:**
```bash
sudo ufw status
# If active, allow Moonraker:
sudo ufw allow 7125/tcp
```

---

### "Connection lost" during print

**Symptoms:**
- Disconnect toast appears
- UI shows "Disconnected"
- Print continues (Klipper handles it)

**Causes:**
1. Network instability (WiFi)
2. Moonraker timeout
3. Power management issues

**Solutions:**

**Use Ethernet if possible:**
Wired connections are more reliable than WiFi.

**Check WiFi signal strength:**
```bash
iwconfig wlan0 | grep -i signal
```

**Disable WiFi power management:**
```bash
sudo iw wlan0 set power_save off
```

To make permanent, add to `/etc/rc.local`:
```bash
iw wlan0 set power_save off
```

**Increase Moonraker timeouts** in helixconfig.json:
```json
{
  "printers": {
    "default_printer": {
      "moonraker_connection_timeout_ms": 15000,
      "moonraker_request_timeout_ms": 60000
    }
  }
}
```

---

### WiFi won't connect

**Symptoms:**
- WiFi setup in wizard fails
- Network not found
- Authentication failures

**Solutions:**

**Verify WiFi is working at OS level:**
```bash
nmcli device wifi list
```

**Check WPA supplicant:**
```bash
sudo systemctl status wpa_supplicant
```

**Manual WiFi connection:**
```bash
sudo nmcli device wifi connect "YourSSID" password "YourPassword"
```

**For hidden networks:**
```bash
sudo nmcli device wifi connect "HiddenSSID" password "Password" hidden yes
```

---

## Display Issues

### Black screen on startup

**Symptoms:**
- Display stays black
- Service shows running but no output

**Causes:**
1. Wrong display driver
2. Permission issues
3. Display not detected

**Solutions:**

**Check service is running:**
```bash
sudo systemctl status helixscreen
sudo journalctl -u helixscreen -n 50
```

**Verify framebuffer exists:**
```bash
ls -la /dev/fb*
ls -la /dev/dri/*
```

**Check display permissions:**
```bash
# User needs video group access
groups
# Should include 'video'
sudo usermod -aG video $USER
```

**For DRM displays, specify device:**
```json
// helixconfig.json
{
  "display": {
    "drm_device": "/dev/dri/card1"
  }
}
```

---

### Wrong screen size or resolution

**Symptoms:**
- UI too small or too large
- Partial screen visible
- Stretched or squished display

**Solutions:**

**Set correct dimensions:**
Edit `/etc/systemd/system/helixscreen.service`:
```ini
Environment="HELIX_SCREEN_WIDTH=800"
Environment="HELIX_SCREEN_HEIGHT=480"
```

Then reload:
```bash
sudo systemctl daemon-reload
sudo systemctl restart helixscreen
```

---

### Display upside down or rotated

**Symptoms:**
- Content displayed at wrong angle
- Touch offset from visual

**Solutions:**

**Rotate in helixconfig.json:**
```json
{
  "display_rotate": 180
}
```

Valid values: 0, 90, 180, 270

**For DSI displays, use /boot/config.txt:**
```ini
lcd_rotate=2
```

---

## Touch Input Issues

### Touch not responding

**Symptoms:**
- Display works but touch doesn't
- No response to taps

**Causes:**
1. Touch device not detected
2. Wrong input device
3. Permission issues

**Solutions:**

**Check touch device exists:**
```bash
ls /dev/input/event*
cat /proc/bus/input/devices | grep -A5 -i touch
```

**Test touch input:**
```bash
sudo evtest /dev/input/event0
# Tap screen and watch for events
```

**Specify touch device:**
```json
// helixconfig.json
{
  "display": {
    "touch_device": "/dev/input/event1"
  }
}
```

**Check permissions:**
```bash
ls -la /dev/input/event*
# User needs input group
sudo usermod -aG input $USER
```

---

### Touch offset from visuals

**Symptoms:**
- Touch registers in wrong location
- Have to tap above/below intended target

**Causes:**
- Rotation mismatch between display and touch
- Uncalibrated touch screen

**Solutions:**

**Ensure rotation matches for display and touch.**

**For libinput calibration (X11 systems only):**

> **Note:** The `xinput` command requires X11. Since HelixScreen runs directly on framebuffer without X11, this approach won't work in production. Use the config file `display_rotate` setting instead.

```bash
# This only works if you have X11 installed (development/testing)
xinput list-props "Your Touch Device"
xinput set-prop "Your Touch Device" "Coordinate Transformation Matrix" 0 1 0 -1 0 1 0 0 1
```

**For production (no X11):**

Use the `display_rotate` config option in helixconfig.json:
```json
{
  "display_rotate": 90
}
```

---

## Print Issues

### Files not appearing

**Symptoms:**
- Print Select shows empty
- "No files found"
- Known files missing

**Causes:**
1. Wrong file path
2. Moonraker file access issue
3. USB not mounted

**Solutions:**

**Check Moonraker file access:**
```bash
curl http://localhost:7125/server/files/list
```

**Verify gcodes directory:**
```bash
ls -la ~/printer_data/gcodes/
```

**For USB drives, check mount:**
```bash
mount | grep media
ls /media/usb/
```

---

### Print won't start

**Symptoms:**
- Tap Start but nothing happens
- Error message about prerequisites

**Causes:**
1. Klipper not ready
2. Temperature safety checks
3. Homing required

**Solutions:**

**Check Klipper state:**
```bash
curl http://localhost:7125/printer/info | jq '.result.state'
# Should be "ready"
```

**If "error" state, check Klipper logs:**
```bash
cat ~/printer_data/logs/klippy.log | tail -50
```

**Restart Klipper:**
```bash
sudo systemctl restart klipper
```

---

### Can't pause or cancel print

**Symptoms:**
- Buttons don't respond
- Print continues despite tapping Cancel

**Causes:**
1. Connection issue during print
2. Klipper busy processing

**Solutions:**

**For emergency, use E-Stop button on home panel.**

**Check connection status** - if disconnected, wait for reconnection.

**Via terminal:**
```bash
curl -X POST http://localhost:7125/printer/print/cancel
```

---

## Performance Issues

### UI feels slow or laggy

**Symptoms:**
- Delayed response to touches
- Choppy scrolling
- Slow panel transitions

**Causes:**
1. High CPU usage
2. Memory pressure
3. Excessive logging

**Solutions:**

**Check system resources:**
```bash
top
free -h
```

**Reduce log level:**
Remove `-vv` or `-vvv` flags from service if present.

**Disable debug features:**
Ensure not running with `--test` mode in production.

**Check for runaway processes:**
```bash
ps aux | head -20
```

---

### High memory usage

**Symptoms:**
- Out of memory errors
- System becomes unresponsive
- Crashes during complex operations

**Solutions:**

**Check memory usage:**
```bash
free -h
cat /proc/meminfo | head -10
```

**Reduce Moonraker cache** in moonraker.conf:
```ini
[file_manager]
queue_gcode_uploads: False
```

**Limit print history:**
```ini
[history]
max_job_count: 100
```

---

## Configuration Issues

### First-run wizard keeps appearing

**Symptoms:**
- Wizard shows on every boot
- Settings not saved

**Causes:**
- helixconfig.json missing or invalid
- Permission issues

**Solutions:**

**Check config exists:**
```bash
ls -la /opt/helixscreen/helixconfig.json
```

**Check permissions:**
```bash
# Should be readable by helixscreen user
sudo chown root:root /opt/helixscreen/helixconfig.json
sudo chmod 644 /opt/helixscreen/helixconfig.json
```

**Create default config:**
```bash
sudo cp /opt/helixscreen/helixconfig.json.template /opt/helixscreen/helixconfig.json
```

---

### Settings not saving

**Symptoms:**
- Changes revert after restart
- Config file unchanged

**Solutions:**

**Check file is writable:**
```bash
ls -la /opt/helixscreen/helixconfig.json
# Should have write permission
```

**Check disk space:**
```bash
df -h
```

**Try manual edit:**
```bash
sudo nano /opt/helixscreen/helixconfig.json
# Make change, save, restart
sudo systemctl restart helixscreen
```

---

### Wrong printer detected

**Symptoms:**
- Wizard shows wrong printer model
- Features missing or wrong

**Solutions:**

**Re-run wizard:**
1. Delete config: `sudo rm /opt/helixscreen/helixconfig.json`
2. Restart: `sudo systemctl restart helixscreen`
3. Manually select correct printer in wizard

**Manual configuration:**
Edit helixconfig.json to set correct printer type and features.

---

## Gathering Diagnostic Information

When reporting issues, gather this information:

### System Information

```bash
# HelixScreen version (shown in first line of --help output)
/opt/helixscreen/bin/helix-screen --help | head -1

# OS version
cat /etc/os-release

# Hardware
cat /proc/cpuinfo | grep Model
free -h
```

### Recent Logs

```bash
# HelixScreen logs (last 100 lines)
sudo journalctl -u helixscreen -n 100 --no-pager

# With timestamps
sudo journalctl -u helixscreen -n 100 --no-pager -o short-iso

# Errors only
sudo journalctl -u helixscreen -p err --no-pager
```

### Configuration

```bash
# Current config (sanitize API keys!)
cat /opt/helixscreen/helixconfig.json
```

### Display Information

```bash
# Framebuffer
ls -la /dev/fb*

# DRM devices
ls -la /dev/dri/

# Input devices
ls -la /dev/input/
cat /proc/bus/input/devices
```

---

## Getting Help

### Check Existing Resources

1. **This troubleshooting guide** - search for your symptoms
2. **[FAQ](FAQ.md)** - common questions
3. **[GitHub Issues](https://github.com/prestonbrown/helixscreen/issues)** - known problems

### Opening a New Issue

If you can't find a solution, open a GitHub issue with:

**Required Information:**
- HelixScreen version
- Hardware (Pi model, display type)
- What you expected to happen
- What actually happened
- Steps to reproduce

**Helpful Additions:**
- Relevant log output (use code blocks)
- Screenshots if visual issue
- helixconfig.json (remove sensitive data)

**Example Issue Format:**

```markdown
## Environment
- HelixScreen version: 1.0.0
- Hardware: Raspberry Pi 4 4GB
- Display: Official 7" touchscreen
- OS: MainsailOS 1.2.0

## Problem
Cannot connect to printer after WiFi change.

## Expected
Should connect to Moonraker on 192.168.1.100

## Actual
Shows "Connection failed" error

## Steps to Reproduce
1. Change WiFi network
2. Update helixconfig.json with new IP
3. Restart helixscreen service
4. See error

## Logs
```
[error] [Moonraker] Connection refused: 192.168.1.100:7125
```

## Configuration
```json
{
  "moonraker_host": "192.168.1.100",
  "moonraker_port": 7125
}
```
```

---

*Back to: [User Guide](USER_GUIDE.md) | [Installation](INSTALL.md)*
