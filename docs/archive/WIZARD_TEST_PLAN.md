# Wizard UI Test Plan

**Purpose:** Manual test procedures and automated test requirements for the first-run wizard.

## Test Coverage Areas

1. **WiFi Setup (Step 0/1)**
2. **Connection Setup (Step 2)**
3. **Printer Identification (Step 3)**
4. **Hardware Detection (Steps 4-7)**
5. **Summary & Completion (Step 8)**

---

## WiFi Setup Tests

### Test 1.1: WiFi Toggle ON - Placeholder Visibility

**Initial State:**
- WiFi toggle: OFF
- Placeholder visible: "Enable WiFi to scan for networks"
- Network list: empty
- Container: dimmed (50% opacity)

**Steps:**
1. Click WiFi toggle to enable

**Expected Results:**
- ✅ Placeholder immediately hidden
- ✅ Status label shows: "Scanning for networks..."
- ✅ Container un-dimmed (100% opacity)
- ✅ After 3 seconds: Network scan starts
- ✅ After scan completes: Networks populate list

**Verification:**
```bash
# Take before screenshot
./scripts/screenshot.sh helix-screen wifi-off --wizard-step wifi

# Manually toggle WiFi ON, wait 1 second
# Take after screenshot
./scripts/screenshot.sh helix-screen wifi-on-immediate --wizard-step wifi

# Verify: placeholder not visible, status shows "Scanning..."
```

---

### Test 1.2: WiFi Toggle OFF - Placeholder Restoration

**Initial State:**
- WiFi toggle: ON
- Networks displayed in list (or scan in progress)
- Placeholder: hidden

**Steps:**
1. Click WiFi toggle to disable

**Expected Results:**
- ✅ Placeholder immediately shown: "Enable WiFi to scan for networks"
- ✅ Network list cleared (all items removed)
- ✅ Container dimmed (50% opacity)
- ✅ Status label shows: "Enable WiFi to scan for networks"
- ✅ Scan timer canceled if running

**Verification:**
```bash
# Start with WiFi enabled
./build/bin/helix-screen --wizard-step wifi
# Manually toggle OFF
# Verify placeholder reappears immediately
```

---

### Test 1.3: Network Scan Results

**Initial State:**
- WiFi toggle: ON
- Scan in progress

**Steps:**
1. Wait for scan to complete

**Expected Results:**
- ✅ Placeholder hidden (already hidden from toggle)
- ✅ Network items created for each network
- ✅ Each item shows: SSID, signal strength, lock icon (if secured)
- ✅ Items are clickable

**Test Data (macOS mock mode):**
- Should display 10 mock networks
- Signal strengths: 95%, 85%, 75%, 65%, 55%, 45%, 35%, 25%, 15%, 5%
- Mix of secured/unsecured networks

---

### Test 1.4: Empty Scan Results

**Initial State:**
- WiFi toggle: ON
- No networks available

**Steps:**
1. Trigger scan with no networks available

**Expected Results:**
- ✅ Placeholder remains hidden (WiFi is still ON)
- ✅ Status shows: "No networks found" (TODO: implement this)
- ✅ Container not dimmed

**Note:** Currently not fully implemented - empty scans may show placeholder incorrectly.

---

### Test 1.5: Rapid Toggle ON/OFF/ON

**Initial State:**
- WiFi toggle: OFF

**Steps:**
1. Toggle WiFi ON
2. Immediately (< 3s) toggle WiFi OFF
3. Toggle WiFi ON again

**Expected Results:**
- ✅ First ON: Placeholder hidden, scan timer started
- ✅ OFF: Placeholder shown, scan timer canceled
- ✅ Second ON: Placeholder hidden, new scan timer started
- ✅ No duplicate network items
- ✅ No crashes or visual glitches

---

## Connection Setup Tests

### Test 2.1: Initial State

**Expected:**
- ✅ IP address field pre-populated from config
- ✅ Port field shows 7125
- ✅ Test Connection button visible
- ✅ Status label empty

### Test 2.2: Test Connection Success

**Steps:**
1. Enter valid IP and port
2. Click "Test Connection"

**Expected:**
- ✅ Status shows: "Testing..."
- ✅ On success: "Connected ✓"
- ✅ Button remains enabled for re-testing

### Test 2.3: Test Connection Failure

**Steps:**
1. Enter invalid IP (e.g., 192.168.1.999)
2. Click "Test Connection"

**Expected:**
- ✅ Status shows: "Connection failed"
- ✅ Error message descriptive
- ✅ Button remains enabled for retry

### Test 2.4: Card Layout Responsive

**Steps:**
1. Test on different screen sizes: tiny (480x320), small (800x480), medium (1024x600), large (1280x720)

**Expected:**
- ✅ Card fills available space with `height="100%"`
- ✅ Card respects `style_min_height="140"` on small screens
- ✅ Inputs remain on one row (2:1 ratio maintained)
- ✅ Button and status label on second row

---

## Printer Identification Tests

### Test 3.1: Roller Widget Display

**Expected:**
- ✅ Roller shows vertical scrollable list
- ✅ 5 rows visible at a time
- ✅ 32 printer types available
- ✅ Blue selection bar highlights current option
- ✅ Text readable (montserrat_14)

### Test 3.2: Roller Scrolling

**Steps:**
1. Swipe/scroll through roller options

**Expected:**
- ✅ Smooth scrolling animation
- ✅ Snaps to rows
- ✅ Can scroll to all 32 options
- ✅ Wraps at ends (normal mode, not infinite)

### Test 3.3: Printer Name Input

**Steps:**
1. Click printer name field
2. Enter custom name

**Expected:**
- ✅ Global keyboard appears
- ✅ Text updates in real-time
- ✅ Pre-filled with "My Printer"
- ✅ Can clear and re-enter

### Test 3.4: Printer Auto-Detection

**Steps:**
1. Connect to Moonraker
2. Navigate to printer identification

**Expected:**
- ✅ Status shows: "Detecting printer..."
- ✅ Printer type auto-selected if detected
- ✅ Hostname used for default printer name
- ✅ User can override auto-detected values

---

## Automated Test Requirements

### Unit Tests Needed

1. **WiFi Manager Mock Tests**
   - Toggle enable/disable
   - Scan results callback
   - Empty scan handling
   - Concurrent scan requests

2. **Network List Management**
   - `clear_network_list()` removes all items
   - `populate_network_list()` creates correct item count
   - Placeholder visibility logic
   - Item click handlers registered

3. **Widget State Management**
   - Opacity changes (dimmed/undimmed)
   - Flag changes (hidden/visible)
   - State changes (disabled/enabled)

### Integration Tests Needed

1. **Wizard Flow End-to-End**
   - Start wizard → complete all steps → configuration saved
   - Skip wizard → direct to home panel
   - Back button navigation
   - Next button validation

2. **Subject Bindings**
   - WiFi status updates propagate to UI
   - Connection status updates
   - Printer name/type updates

3. **Responsive Layout**
   - All screen sizes render correctly
   - Scroll containers work on small screens
   - Touch targets meet minimum size (48px)

### Visual Regression Tests

1. **Screenshot Comparison**
   - WiFi OFF state
   - WiFi ON + empty results
   - WiFi ON + populated results
   - Connection card layout
   - Printer identification layout

**Tool suggestions:**
- Percy.io (screenshot diffs)
- Playwright (browser automation, not applicable)
- Custom BMP comparison script

---

## Test Execution

### Manual Testing Procedure

```bash
# Build latest
make -j

# Test WiFi toggle behavior
./build/bin/helix-screen --wizard-step wifi -s small

# Manual steps:
# 1. Verify placeholder visible with WiFi OFF
# 2. Toggle WiFi ON → placeholder should hide immediately
# 3. Wait 3s → networks should populate
# 4. Toggle WiFi OFF → placeholder should show immediately
# 5. Verify network list cleared and dimmed

# Test connection screen
./build/bin/helix-screen --wizard-step connection -s medium

# Test printer identification
./build/bin/helix-screen --wizard-step printer-identify -s small

# Full wizard flow
./build/bin/helix-screen --wizard
```

### Automated Testing (Future)

```bash
# Run unit tests
make test

# Run integration tests
make test-integration

# Visual regression tests
make test-visual

# Coverage report
make coverage
```

---

## Known Issues / TODO

1. **Empty scan handling**: When scan returns 0 networks, placeholder should show different message
2. **Connection timeout**: No visual feedback during 10-second timeout
3. **Roller XML bug**: Options must be set programmatically due to parser limitation
4. **No automated tests**: All testing currently manual

---

## Test Maintenance

**Update this document when:**
- New wizard steps added
- UI components changed
- Behavior modified
- Bugs discovered and fixed

**Review frequency:** Before each release

**Owner:** Development team

---

**Last Updated:** 2025-10-27
**Version:** 1.0.0
