# UI Changelog - Home Panel

> **Purpose**: Document recent changes for screenshot review
> **Version**: 2025-01-11
> **Panel**: home_panel.xml

## Summary

**Total Changes**: 3
- **Fixed Issues**: 0 (initial version)
- **New Additions**: 3
- **Improvements**: 0

---

## New Additions

### 1. Complete Home Panel Layout Created

**Purpose**:
Establish the foundational home screen with printer visualization and quick-access status information.

**Implementation**:
```xml
<!-- NEW: Two-section vertical layout (home_panel.xml:3) -->
<view extends="lv_obj" width="100%" height="100%"
      style_bg_color="#panel_bg"
      style_pad_all="0"
      style_border_width="0"
      flex_flow="column">
    <!-- Top: Printer visualization (flex_grow="2") -->
    <!-- Bottom: Info cards (flex_grow="1") -->
</view>
```

**Expected Visual Result**:
- Full-screen panel with dark background (#111410)
- Top 2/3 shows printer image and status text
- Bottom 1/3 shows two cards side-by-side
- All content properly centered and spaced

**Verification Criteria**:
- [x] Panel fills entire content area
- [x] Background is dark green-tinted black
- [x] No borders or padding on outer panel
- [x] Vertical sections divide space 2:1

---

### 2. Printer Visualization Section

**Purpose**:
Display printer image and dynamic status message as primary visual focus.

**Implementation**:
```xml
<!-- NEW: Printer section (home_panel.xml:5-11) -->
<lv_obj width="100%" flex_grow="2"
        style_bg_opa="0%"
        style_border_width="0"
        style_pad_all="#padding_normal"
        flex_flow="column"
        style_flex_main_place="center"
        style_flex_cross_place="center">
    <lv_image src="A:/path/printer_400.png"
              width="#printer_img_size"
              height="#printer_img_size"/>
    <lv_label bind_text="status_text"
              style_text_color="#text_primary"
              style_text_font="montserrat_20"
              style_margin_top="#padding_normal"/>
</lv_obj>
```

**Expected Visual Result**:
- 400x400px printer image centered in top section
- Status text below image with 20px gap
- White text on dark background
- Entire section centered both horizontally and vertically

**Verification Criteria**:
- [x] Printer image is visible and properly sized
- [x] Image is perfectly centered horizontally
- [x] Status text appears below image
- [x] Text is white (high contrast)
- [x] 20px margin between image and text

---

### 3. Status Cards with Three Information Sections

**Purpose**:
Provide quick-glance access to printer status (temperature, network, lights) and file management.

**Implementation**:
```xml
<!-- NEW: Bottom card section (home_panel.xml:14-46) -->
<lv_obj width="100%" flex_grow="1"
        style_bg_opa="0%"
        style_border_width="0"
        style_pad_all="#padding_normal"
        flex_flow="row"
        flex_align="space_evenly center center">

    <!-- Print Files Card -->
    <lv_obj width="#card_width" height="#card_height"
            style_bg_color="#card_bg"
            style_radius="#card_radius" ...>
        <lv_label text="Print Files" .../>
    </lv_obj>

    <!-- Status Card with 3 sections + dividers -->
    <lv_obj width="#card_width" height="#card_height"
            style_bg_color="#card_bg"
            style_radius="#card_radius"
            flex_flow="row" ...>

        <!-- Temperature: Icon + Text -->
        <lv_obj flex_flow="column" ...>
            <lv_label text="#icon_temperature" style_text_font="fa_icons_48"/>
            <lv_label bind_text="temp_text" .../>
        </lv_obj>

        <!-- Divider (1px vertical line) -->
        <lv_obj width="1" style_align_self="stretch"
                style_bg_color="#text_secondary"
                style_bg_opa="50%"/>

        <!-- Network: Icon + Label -->
        <lv_obj flex_flow="column" ...>
            <lv_label name="network_icon" text="#icon_wifi"
                      style_text_color="#primary_color"/>
            <lv_label name="network_label" text="Wi-Fi"/>
        </lv_obj>

        <!-- Divider -->
        <lv_obj width="1" .../>

        <!-- Light: Icon only (clickable) -->
        <lv_obj flag_clickable="true" flex_flow="column" ...>
            <lv_label name="light_icon" text="#icon_lightbulb"/>
            <lv_event-call_function trigger="clicked" callback="light_toggle_cb"/>
        </lv_obj>
    </lv_obj>
</lv_obj>
```

**Expected Visual Result**:
- Two cards side-by-side in bottom section
- Each card 45% width, 80% height, with 8px rounded corners
- Cards have dark gray background (#202020) distinct from panel bg
- Print Files card: Simple centered label
- Status card: Three sections (temp/network/light) evenly distributed
- Vertical dividers (1px gray lines) between sections
- Icons 48px size, labels 16px below icons with 8px gap
- Network icon red (active), other icons gray
- Temperature displays reactive text binding

**Verification Criteria**:
- [x] Two cards visible and evenly spaced
- [x] Cards are same size
- [x] Cards have rounded corners (8px radius)
- [x] Card backgrounds clearly distinct from panel background
- [x] Print Files text centered in left card
- [x] Status card has three distinct sections
- [x] Vertical dividers visible between sections
- [x] Dividers are thin (1px) and gray
- [x] Dividers stretch full card height
- [x] Temperature icon visible (thermometer)
- [x] Network icon visible (wifi) and red colored
- [x] Light icon visible (lightbulb) and gray
- [x] All icons properly sized (48px, not too large/small)
- [x] Labels appear below icons
- [x] Labels have 8px spacing from icons
- [x] Icon-label stacks are centered in their sections

---

## Configuration Used

### Global Constants (globals.xml)
All constants referenced are defined:

```xml
<!-- Colors -->
<color name="panel_bg" value="0x111410"/>
<color name="card_bg" value="0x202020"/>
<color name="text_primary" value="0xffffff"/>
<color name="text_secondary" value="0x909090"/>
<color name="primary_color" value="0xff4444"/>

<!-- Dimensions -->
<px name="padding_normal" value="20"/>
<px name="card_radius" value="8"/>
<px name="printer_img_size" value="400"/>
<percent name="card_width" value="45%"/>
<percent name="card_height" value="80%"/>

<!-- Icons (auto-generated UTF-8) -->
<str name="icon_temperature" value=""/>  <!-- U+F2C7 thermometer-half -->
<str name="icon_wifi" value=""/>         <!-- U+F1EB wifi -->
<str name="icon_lightbulb" value=""/>    <!-- U+F0EB lightbulb -->
```

### Subjects Initialized (C++)
```cpp
// ui_panel_home.cpp
lv_subject_t status_text_subject;  // Bound to status label
lv_subject_t temp_text_subject;    // Bound to temperature display
```

### Named Widgets for C++ Access
- `network_icon` - For changing wifi icon state
- `network_label` - For updating network name/status
- `light_icon` - For changing light on/off state

---

## Testing Notes

### Build & Run
```bash
make
./build/bin/helix-ui-proto
# Press 'S' to capture screenshot
```

### Screenshot Location
```bash
/tmp/ui-screenshot-YYYYMMDD-HHMMSS.bmp
```

### Conversion for Review
```bash
magick /tmp/ui-screenshot-*.bmp /tmp/home-panel-review.png
```

---

## Notes for Reviewer Agent

### Critical Elements to Verify
1. **Layout proportions**: Top section should be twice the height of bottom section
2. **Card sizing**: Both cards should be identical width (45%) and height (80%)
3. **Icon sizing**: All status card icons should be same size (48px) and properly proportioned
4. **Divider visibility**: Thin vertical lines should be clearly visible but subtle
5. **Color consistency**: All colors should match the dark theme palette
6. **Centering**: Everything should be perfectly centered (no left/right bias)

### Subtle Details
1. Dividers have 8px vertical padding (don't touch top/bottom edges)
2. Network icon is red (#ff4444) indicating active connection
3. Light icon is gray (#909090) indicating off state
4. Icon-to-label spacing is 8px (margin_top)
5. Card backgrounds are distinct from but harmonious with panel background

### Context
This is the initial implementation - no fixes or improvements yet. Everything is new. Future iterations will refine spacing, add interactions, and connect to real printer data.

---

## Related Files

### Modified
- `ui_xml/home_panel.xml` - Complete new panel layout
- `src/ui_panel_home.cpp` - Subject initialization
- `include/ui_panel_home.h` - API declarations

### Referenced
- `ui_xml/globals.xml` - Theme constants and icon definitions
- `assets/images/printer_400.png` - Printer visualization image
- `assets/fonts/fa_icons_48.c` - FontAwesome icon font (48px)

### Commits
- `ce3a16f` - Add comprehensive LVGL 9 prototype enhancements (includes home panel)
