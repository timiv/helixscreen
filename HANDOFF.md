# Session Handoff Document

**Last Updated:** 2025-10-25
**Current Focus:** ✅ **Responsive Header Heights & Vertical Padding System**

---

## What Was Just Accomplished (2025-10-25 Latest Session)

### 0. Responsive Header Heights & Vertical Padding
- **Problem:** Fixed 60px headers and 20px padding wasted ~19% of tiny screen vertical space
- **Solution:** Created `ui_component_header_bar` component system for responsive management
- **Headers:** 40px (tiny), 48px (small), 60px (medium/large) - saves 20px on tiny screens
- **Padding:** Split pattern - vertical responsive (6/10/20px), horizontal fixed (12px)
- **Space Saved:** 34px on tiny (10.6% more usable), 22px on small (4.6% more usable)
- **Pattern:** Component wrapper with single setup call per panel
- **Updated Panels:** motion, extrusion, nozzle_temp, bed_temp, print_status
- **Files Created:** `ui_component_header_bar.cpp/h` (203 lines total)
- **Files Modified:** 9 files (ui_utils, main.cpp, 5 panel files)
- **Testing:** Verified on tiny/small/large - header heights and padding apply correctly
- **Documentation:** Updated `docs/RESPONSIVE_DESIGN_PATTERN.md` with C++ component pattern

### 1. Controls Panel Responsive Launcher Cards
- **Problem:** Cards were 400×200px hardcoded - too large on tiny/small screens (only 1 card fit on 480×320)
- **Problem:** `flex_grow` caused uneven card sizing when rows had different numbers of cards
- **Solution:** Switched to percentage-based width (`47%`) with no flex_grow
- **Result:** Exactly 2 cards per row on ALL screen sizes (tiny through large)
- **Result:** All cards identical size across all rows (no uneven distribution)
- **Fonts:** Reduced title to `montserrat_16`, subtitle to `montserrat_14` for better density
- **Padding:** Reduced from 24px → 16px for compact layouts
- **Files:** `ui_xml/globals.xml` (launcher card constants), `ui_xml/controls_panel.xml` (all 6 cards)
- **Pattern:** Simple percentage width for uniform sizing (simpler than print file cards)

---

## Previous Session (2025-10-25 Earlier Session)

### 0. Command-Q Quit Shortcut Implementation
- **Added:** Cmd+Q (macOS) / Win+Q (Windows) keyboard shortcut to quit application
- **Location:** `src/main.cpp` lines 555-561 (main event loop)
- **Approach:** Uses SDL state query functions (no SDL_PollEvent() interference with LVGL)
- **Detection:** `SDL_GetModState()` for KMOD_GUI + `SDL_GetKeyboardState()` for Q key
- **Behavior:** Clean exit - breaks loop, runs cleanup, logs action
- **Cross-platform:** Works on macOS (Cmd+Q) and Windows/Linux (Win+Q)

### 1. Makefile Parallel Building by Default
- **Fixed:** Default `make` command now uses all CPU cores automatically
- **Change:** Added `MAKEFLAGS += -j$(NPROC)` to Makefile line 57
- **Benefit:** Faster builds without needing to remember `make build` vs `make`
- **Auto-detection:** NPROC detects CPU cores (macOS sysctl, Linux nproc, fallback 4)
- **Consistency:** Matches HANDOFF.md documentation ("make # Incremental build (auto-parallel)")

---

## Previous Session (2025-10-25 Earlier)

### Documentation Cleanup - Event Handlers Clarification
- **Clarification:** Event handlers were already implemented in Phase 5.2 (2025-10-12 Late Night)
- **Removed:** Stale "Wire Keypad Event Handlers" task from HANDOFF.md
- **Updated:** STATUS.md with clarification entry documenting implementation timeline
- **Status:** Numeric keypad is 100% complete (sizing, appearance, event handling, testing)
- **Next Priority:** Apply responsive design pattern to other panels

---

## Previous Session (2025-10-25)

### 1. Removed Keypad Button Height Constraint
- **Problem:** Keypad buttons capped at 70px max height, wasted vertical space
- **Solution:** Removed `style_max_height="#button_max_height"` from all 11 buttons
- **Result:** Buttons now fill available space with `height="100%"` and `flex_grow="1"`
- **Files:** `ui_xml/numeric_keypad_modal.xml`

### 2. FontAwesome Backspace Icon Integration
- **Problem:** Backspace button used placeholder character ⌫ (U+232B) not in Montserrat fonts
- **Solution:**
  - Integrated FontAwesome icon (U+F55A delete-left)
  - Used `fa_icons_24` font to match `montserrat_20` button label size
  - Updated button: `text="#icon_backspace" style_text_font="fa_icons_24"`
- **Files:** `ui_xml/numeric_keypad_modal.xml`

### 3. Icon Generation Script Fixes
- **Problem:** `scripts/generate-icon-consts.py` pattern matching failed on globals.xml
- **Solution:**
  - Fixed regex to handle tab/space variations: `r'(\t<!-- ={69} -->.*?FontAwesome.*?<!-- ={69} -->.*?)(?=\n\t<!-- ={69} -->|\n</consts>)'`
  - Script now reliably replaces existing icon section
  - Generated 33 icon constants with proper UTF-8 encoding
- **Files:** `scripts/generate-icon-consts.py`, `ui_xml/globals.xml`

### 4. Documentation Updates
- **STATUS.md:** Added detailed session entry with technical details
- **HANDOFF.md:** Updated current focus and completed phases
- **CLAUDE.md:** Added gotcha about icon constants appearing empty in terminal

### Technical Details

**Icon Constant Encoding:**
Icon constants use FontAwesome Private Use Area (U+F000-U+F8FF):
- Characters appear **empty** in terminal/grep output
- UTF-8 bytes are present: e.g., `ef959a` for U+F55A
- Rendered correctly when using FontAwesome fonts in LVGL

**Verification:**
```bash
# Check UTF-8 encoding
python3 << 'PYEOF'
with open('ui_xml/globals.xml', 'r') as f:
    for line in f:
        if 'icon_backspace' in line:
            import re
            match = re.search(r'value="([^"]*)"', line)
            if match:
                val = match.group(1)
                print(f"Length: {len(val)}, Bytes: {val.encode('utf-8').hex()}")
PYEOF
# Output: Length: 1, Bytes: ef959a
```

**Font Size Consistency:**
- Number buttons: `montserrat_20`
- Backspace icon: `fa_icons_24`
- Rationale: 24px FontAwesome ≈ 20px Montserrat for visual consistency

**Build & Test:**
```bash
make clean && make                    # Clean rebuild
./build/bin/helix-ui-proto -k         # Test keypad
./scripts/test_keypad_sizes.sh        # Test all screen sizes
```

**Previous Session (2025-10-25) - Responsive Numeric Keypad:**
- ✅ Keypad works on ALL screen sizes (480×320 through 1280×720)
- ✅ Semantic constants only, flex layout, responsive design pattern established
- ✅ Test automation: `./scripts/test_keypad_sizes.sh`

---

## Current State Summary

**Project Status:** All UI components functional and tested. Navigation system robust. Visual polish complete. **Responsive design pattern established.**

**Automated Testing:** ✅ 26/26 tests passing
- All command-line flags working (8 panel flags, 4 screen sizes)
- All panels render without errors
- UI polish verified (rounded corners, thumbnail scaling, gradients)
- Test script: `./scripts/test_navigation.sh`

---

## Completed Phases

**Recent (2025-10-25):**
- ✅ **Phase 6.16: Controls Panel Responsive Design** - Fixed oversized launcher cards, switched to 47% width for uniform sizing (2 cards/row on all screens), reduced fonts/padding for better density
- ✅ **Phase 6.15: Command-Q Quit Shortcut & Makefile Auto-Parallel** - Added Cmd+Q/Win+Q keyboard shortcut (src/main.cpp:555-561), enabled parallel builds by default (MAKEFLAGS += -j$(NPROC))
- ✅ **Phase 6.14: Keypad Documentation Cleanup** - Clarified event handlers were complete from Phase 5.2, removed stale tasks, updated HANDOFF.md and STATUS.md
- ✅ **Phase 6.13: Keypad Button Sizing & Icon Integration** - Removed max height constraint, FontAwesome backspace icon with fa_icons_24, icon generation script fixes
- ✅ **Phase 6.12: Responsive Numeric Keypad** - Mobile-first design, semantic constants only, works on all screen sizes (480×320 - 1280×720)
- ✅ **Phase 5.2: Numeric Keypad Modal Component** - Complete implementation with event handlers (2025-10-12 Late Night)
- ✅ **Phase 6.11: Interactive Testing & Automation** - Created automated test suite, verified all navigation flows and UI polish
- ✅ **Phase 6.10: Navigation System Refactoring & Bug Fixes** - Robust navigation with unified panel stack, app_layout protection, UI polish
- ✅ **Phase 6.9: Navigation History Stack** - Back button navigation with state preservation

**Previous:**
- ✅ **Phase 6.8: Print Status Panel Complete** (2025-10-24) - Full layout with metadata overlay, temp cards, control buttons, perfect 2:1 ratio
- ✅ **Phase 6.7: Print Status Panel Layout Rebuild - Foundation** (2025-10-24) - Incremental approach: empty layout working, thumbnails rendering with resize callback
- ✅ **Phase 6.6: Detail View Responsive Layout Fix** (2025-10-24) - Detail view respects navigation bar, uses calculated constants, fits screen height
- ✅ **Phase 6.5: Print Button Integration** (2025-10-24) - Print button launches print status panel with mock print
- ✅ **Phase 6.4: Header Bar Responsive Refactoring** (2025-10-24) - Removed hardcoded widths, added semantic constants, fully responsive layout
- ✅ **Phase 6.3: LVGL 9 XML Syntax Cleanup** (2025-10-24) - Fixed all flag_* attributes across entire codebase
- ✅ **Phase 6.2: Print File Detail View Complete** (2025-10-24) - Including gradient fill fix and LVGL XML syntax corrections
- ✅ Phase 6.1: Print File Card Gradient Background
- ✅ Phase 6: Responsive Print File Card System with App-Level Resize Handler
- ✅ Phase 5.5: Extrusion Panel with safety checks
- ✅ Phase 5.4: Temperature Sub-Screens (Nozzle + Bed)
- ✅ Phase 5.3: Motion Panel with 8-direction jog pad
- ✅ Phase 5.2: Numeric Keypad modal component
- ✅ Phase 5.1: Controls Panel launcher
- ✅ Phase 4: Print Select Panel (dual views, sorting, file operations)

**What Works:**
- **✅ Comprehensive automated testing:**
  - Test script: `./scripts/test_navigation.sh` (26/26 tests passing)
  - All command-line flags verified (-p panel, -s size)
  - All panels render without initialization errors
  - UI polish verified via screenshot review
- **✅ Robust navigation system:**
  - Unified panel stack architecture (all panels tracked in z-order)
  - Nav bar buttons clear stack and show selected panel
  - Back buttons navigate to previous panel (with state preserved)
  - Defensive fallback to HOME panel if stack empty
  - App layout protection prevents navbar from disappearing
  - Handles edge cases (command line flags, empty history)
- Home panel with temperature/network/light displays
- Controls launcher → sub-screens (motion, temps, extrusion)
- **Print Select panel:**
  - Card/list dual views with toggle
  - Fully responsive cards (adapt to any screen size)
  - **Rounded corners with proper clipping** (no image overflow)
  - Dynamic dimension calculation in C++
  - **Gradient background** (diagonal gray-to-black)
  - **Object-fit:cover for cards** (48.8% for 500x500→204x244)
  - **Object-fit:contain for detail view** (shows full thumbnail)
  - New transparent blue egg placeholder (500x500)
  - Metadata overlays with transparency
  - Window resize handling with debounced callbacks
  - Sorting by filename, size, date, print time
  - **File detail view:**
    - **Rounded corners with proper clipping** (no image overflow)
    - Overlay positioned after navigation bar (respects nav width)
    - Responsive width using `UI_NAV_WIDTH()` macro (screen-size agnostic)
    - Proper padding prevents height overflow (fits within screen)
    - Left section styled like large card (gradient fills entire area, metadata overlay)
    - **Thumbnail scaling when shown** (gradient cover, thumbnail contain)
    - Left chevron back button (properly hidden when not needed using correct XML syntax)
    - Delete/Print buttons on right
  - **Print button integration:**
    - Launches print status panel overlay
    - Starts mock print with selected file (250 layers, 3 hours)
    - Progress updates every second
    - Temperature/layer/time simulation
- **Print Status panel:**
  - ✅ **COMPLETE** - Full UI with all components
  - ✅ Perfect 2:1 layout (thumbnail 66%, controls 33%)
  - ✅ Metadata overlay with progress bar, filename, time/percentage
  - ✅ Nozzle and bed temperature cards (clickable)
  - ✅ Control buttons: Light, Pause, Tune, Cancel (red)
  - ✅ Thumbnail scaling with gradient background
  - ✅ Mock print simulation with data updates
  - ✅ Event handlers wired for all buttons
  - Back button returns to Home
  - Screenshot: `/tmp/ui-screenshot-print-status-final.png`
- Responsive design across tiny/small/medium/large screens
- Material Design icon system with dynamic recoloring
- Reactive Subject-Observer data binding
- **App-level resize handler** for responsive layouts

**What Needs Wiring:**
- Temperature panel preset buttons → update target temps
- Custom temp button → open numeric keypad
- Confirm/back buttons on all sub-screens
- Extrusion/retract buttons
- Motion jog buttons → position updates
- Print Select file operations → actual printer integration
- Detail view Print/Delete buttons → actual operations
- All interactive flows need end-to-end testing with Moonraker

See **STATUS.md** for complete chronological development history.

---

## Critical Architecture Patterns

### 0. Navigation System Architecture (UPDATED - 2025-10-25)

**Unified Panel Stack Approach:**

All panel visibility is managed through a single `panel_stack` vector that tracks visible panels in z-order (bottom to top).

**Always use `ui_nav_push_overlay()` and `ui_nav_go_back()` for overlay navigation:**

```cpp
// When showing an overlay panel (motion, temp, extrusion, etc.)
void card_motion_clicked(lv_event_t* e) {
    // Create panel if needed...

    // ✓ CORRECT - Pushes current panel to stack and shows overlay
    ui_nav_push_overlay(motion_panel);
}

// In back button callback
static void back_button_cb(lv_event_t* e) {
    // ✓ CORRECT - Pops stack, hides current, shows previous
    // Defensively hides all overlays and defaults to HOME if stack empty
    ui_nav_go_back();  // No need for fallback - built-in
}
```

**Behavior:**
- `ui_nav_push_overlay()` - Hides current, shows overlay, pushes to stack
- `ui_nav_go_back()` - Defensively hides all overlays, pops stack, shows previous or HOME
- Clicking nav bar icons **clears stack** and shows selected panel (nav_button_clicked_cb in ui_nav.cpp:82-136)

**App Layout Protection (CRITICAL):**

Navigation code must NEVER hide `app_layout` (the container holding navbar + all panels):

```cpp
// In main.cpp after creating app_layout from XML:
lv_obj_t* app_layout = lv_xml_create(screen, "app_layout", NULL);
ui_nav_set_app_layout(app_layout);  // CRITICAL - prevents navbar disappearing
```

**Architecture:**
```
screen
└── app_layout (navbar + content_area container) ← NEVER HIDE THIS
    ├── navigation_bar
    └── content_area
        ├── main panels (home, controls, etc.)
        └── overlay panels shown on top
```

**State Preservation:** Panels remain in memory when hidden, state is preserved when navigating back.

**Reference:** `include/ui_nav.h:54-62`, `src/ui_nav.cpp:39-40, 43-48, 82-136, 183-186, 333-408`

### 1. Name-Based Widget Lookup (CRITICAL)

**Always use names, never indices:**

```cpp
// ✓ CORRECT - Resilient to layout changes
lv_obj_t* widget = lv_obj_find_by_name(parent, "widget_name");

// ✗ WRONG - Breaks when XML changes
lv_obj_t* widget = lv_obj_get_child(parent, 3);
```

### 2. Component Instantiation Names (CRITICAL)

**Always add explicit `name` attributes when instantiating components:**

```xml
<!-- app_layout.xml -->
<lv_obj name="content_area">
  <controls_panel name="controls_panel"/>  <!-- Explicit name required -->
  <home_panel name="home_panel"/>
</lv_obj>
```

**Why:** Component names in `<view name="...">` definitions do NOT propagate to `<component_tag/>` instantiations. Without explicit names, `lv_obj_find_by_name()` returns NULL.

### 3. Subject Initialization Order (CRITICAL)

**Subjects MUST be initialized BEFORE XML creation:**

```cpp
// 1. Register XML components
lv_xml_component_register_from_file("A:/ui_xml/globals.xml");
lv_xml_component_register_from_file("A:/ui_xml/home_panel.xml");

// 2. Initialize subjects FIRST
ui_nav_init();
ui_panel_home_init_subjects();

// 3. NOW create UI
lv_obj_t* screen = lv_xml_create(NULL, "app_layout", NULL);
```

If subjects are created in XML before C++ initialization, they'll have empty/default values.

### 4. LVGL XML Event Callbacks (CRITICAL)

**Use `<lv_event-call_function>`, NOT `<event_cb>`:**

```xml
<!-- CORRECT (matches LVGL source code) -->
<lv_button name="my_button">
    <lv_event-call_function trigger="clicked" callback="my_handler"/>
</lv_button>

<!-- WRONG (online docs show this, but it doesn't exist) -->
<lv_button name="my_button">
    <event_cb trigger="clicked" callback="my_handler"/>
</lv_button>
```

**Registration in C++ (BEFORE XML loads):**
```cpp
lv_xml_register_event_cb(NULL, "my_handler", my_handler_function);
```

**Source:** `lvgl/src/others/xml/lv_xml.c:113` - LVGL registers `"lv_event-call_function"`, not `"event_cb"`. Online docs are incorrect.

### 5. LVGL 9 XML Attribute Names (CRITICAL)

**Two systematic bugs to avoid:**

1. **No abbreviations - Use full words:**
   ```xml
   <!-- ✗ WRONG - Parser silently ignores -->
   <lv_image style_img_recolor="#primary_color" style_img_recolor_opa="255"/>

   <!-- ✓ CORRECT - Full word "image" -->
   <lv_image style_image_recolor="#primary_color" style_image_recolor_opa="255"/>
   ```

2. **No `zoom` attribute - Use `scale_x`/`scale_y`:**
   ```xml
   <!-- ✗ WRONG - zoom doesn't exist in LVGL 9 -->
   <lv_image src="mat_clock" zoom="180"/>

   <!-- ✓ CORRECT - scale where 256 = 100% -->
   <lv_image src="mat_clock" scale_x="72" scale_y="72"/>
   ```

**When attributes don't work:** Check the parser source code in `/lvgl/src/others/xml/parsers/`, not just online docs.

### 6. Margin vs Padding for Dividers

**Critical distinction for thin elements:**

```xml
<!-- ✗ WRONG - Padding adds to height, causes overflow -->
<lv_obj width="1" height="100%" style_pad_ver="12"/>  <!-- 100% + 24px = overflow -->

<!-- ✓ CORRECT - Margin pushes inward, no overflow -->
<lv_obj width="1" height="100%" style_margin_ver="12"/>  <!-- Creates inset -->
```

**Pattern:** Use **margin for insets**, **padding for internal spacing**.

### 7. Responsive Design Philosophy

**Use responsive percentages for layout, fixed pixels for interactions:**

```xml
<!-- ✓ Responsive layouts -->
<percent name="overlay_panel_width" value="68%"/>
<percent name="file_card_width" value="22%"/>

<!-- ✓ Fixed interactive elements -->
<px name="button_height_normal" value="56"/>  <!-- Minimum touch target -->
<px name="padding_normal" value="20"/>        <!-- Visual consistency -->
```

**Benefits:** Single codebase supports tiny/small/medium/large screens without duplication.

### 8. Responsive Card Layout with Flexbox (CRITICAL)

**Pattern: flex_basis + flex_grow + min/max constraints**

```xml
<!-- Print file card - adapts to any container width -->
<view extends="lv_obj"
      style_flex_basis="#card_base_width"   <!-- Starting point (e.g., 170px) -->
      flex_grow="1"                          <!-- Allow growth to fill space -->
      style_min_width="#card_min_width"      <!-- Floor (e.g., 140px) -->
      style_max_width="#card_max_width"      <!-- Ceiling (e.g., 220px) -->
      height="#card_height">
```

**When cards need identical sizes across rows:**

```cpp
// Calculate exact dimensions based on container
CardDimensions dims = calculate_card_dimensions(container);

// Set explicit size and disable flex_grow
lv_obj_set_width(card, dims.card_width);
lv_obj_set_height(card, dims.card_height);
lv_obj_set_style_flex_grow(card, 0, LV_PART_MAIN);  // Prevent uneven distribution
```

**Why disable flex_grow?** Flexbox distributes *leftover* space unevenly when rows have different numbers of items. Setting explicit dimensions ensures all cards are identical.

**Reference:** `ui_xml/print_file_card.xml`, `src/ui_panel_print_select.cpp:430-510`

### 9. Image Inner Alignment (CRITICAL)

**Distinction: `align` vs `inner_align`**

- `align` - Positions the **widget** relative to its parent
- `inner_align` - Positions the **image content** within the widget bounds

```xml
<!-- Thumbnail fills card, content top-aligned -->
<lv_image src="$thumbnail_src"
          width="100%"
          height="100%"
          align="top_mid"           <!-- Widget position in parent -->
          inner_align="top_mid"     <!-- Image content position in widget -->
          name="thumbnail"/>
```

**Available inner_align values:**
- `top_left`, `top_mid`, `top_right`
- `bottom_left`, `bottom_mid`, `bottom_right`
- `left_mid`, `center`, `right_mid`
- `stretch`, `tile`, `contain`, `cover`

**Scaling images to fill:**
```cpp
// Get source dimensions
int32_t img_w = lv_image_get_src_width(thumbnail);

// Calculate zoom factor (256 = 100% scale in LVGL)
uint16_t zoom = (card_width * 256) / img_w;

// Apply scale
lv_image_set_scale(thumbnail, zoom);
```

**IMPORTANT:** By default, LVGL images maintain aspect ratio and center within their widget. Always set `inner_align` explicitly for predictable behavior.

**Reference:** `ui_xml/print_file_card.xml:72-78`, LVGL docs `widgets/image.html`

### 10. Responsive Overlay Positioning (CRITICAL)

**Pattern: Position overlays after navigation bar using calculated constants**

Full-screen overlays (detail views, modals) should respect the navigation bar:

```xml
<!-- print_file_detail.xml -->
<view extends="lv_obj"
      x="#nav_width"              <!-- Start after nav bar -->
      height="100%"
      flex_flow="column">
```

```cpp
// In C++ - calculate width dynamically
lv_coord_t screen_width = lv_obj_get_width(parent_screen_widget);
lv_coord_t nav_width = UI_NAV_WIDTH(screen_width);  // From ui_theme.h
lv_obj_set_width(detail_view_widget, screen_width - nav_width);
```

**Height constraints:**

Use symmetric padding to prevent overflow:

```xml
<lv_obj width="100%"
        flex_grow="1"
        style_pad_top="#padding_normal"
        style_pad_bottom="#padding_normal"    <!-- Match top padding -->
        flex_flow="row">
```

**Key points:**
- Use `UI_NAV_WIDTH()` macro from `ui_theme.h` - never hardcode
- Calculate remaining width in C++ for screen-size agnostic layout
- Match top/bottom padding to prevent height overflow
- Set `x` position in XML, `width` in C++ for responsive behavior

**Reference:** `ui_xml/print_file_detail.xml:26-47`, `src/ui_panel_print_select.cpp:663-666`

### 11. App-Level Resize Handler (CRITICAL)

**Architecture: Centralized handler with panel registration**

```cpp
// In main.cpp - initialize once at startup
ui_resize_handler_init(screen);

// In panel setup - register callback
static void on_resize() {
    populate_card_view();  // Recalculate dimensions, repopulate
}
ui_resize_handler_register(on_resize);
```

**Implementation details:**
- **Debounce period:** 250ms (prevents excessive calls during rapid resize)
- **Event trigger:** `LV_EVENT_SIZE_CHANGED` on main screen
- **Timer behavior:** One-shot, resets on each resize event
- **Callback execution:** All registered callbacks invoked after debounce settles

**Benefits:**
- Panels own their resize logic (separation of concerns)
- Single timer handles all resize events (no per-panel timers)
- Debouncing prevents performance issues
- Easily scalable for future responsive panels

**Files:**
- API: `include/ui_utils.h:84-106`
- Implementation: `src/ui_utils.cpp:135-207`
- Example usage: `src/ui_panel_print_select.cpp:194-201,286`

**When to use:** Any panel with responsive layouts that need recalculation on window resize (primarily for testing, since real hardware has fixed screen sizes).

---

## Material Design Icon System

**Icon Format:** RGB565A8 (16-bit color + 8-bit alpha) LVGL 9 C arrays

**Conversion Workflow:**
```bash
# Automated: SVG → PNG (Inkscape) → LVGL 9 C array
./scripts/convert-material-icons-lvgl9.sh
```

**CRITICAL:** Use **Inkscape** for SVG→PNG conversion. ImageMagick loses alpha channel, causing icons to render as solid squares.

**Dynamic Recoloring in XML:**
```xml
<lv_image src="mat_heater"
          style_image_recolor="#primary_color"
          style_image_recolor_opa="255"/>
```

**Dynamic Recoloring in C++:**
```cpp
lv_obj_set_style_img_recolor(icon, UI_COLOR_PRIMARY, LV_PART_MAIN);
lv_obj_set_style_img_recolor_opa(icon, 255, LV_PART_MAIN);
```

**Scaling:**
- Material icons are 64×64px at scale 256 (100%)
- For 14px icons: `scale_x="56" scale_y="56"` (14/64 × 256 = 56)
- For 32px icons: `scale_x="128" scale_y="128"`

---

## Component Registration & Initialization

**Order matters in main.cpp:**

```cpp
int main() {
    // 1. Register custom widgets FIRST
    material_icons_register();
    ui_icon_register_widget();

    // 2. Register XML components
    lv_xml_component_register_from_file("A:/ui_xml/globals.xml");
    lv_xml_component_register_from_file("A:/ui_xml/navigation_bar.xml");
    lv_xml_component_register_from_file("A:/ui_xml/home_panel.xml");
    // ... all other panels

    // 3. Initialize subjects BEFORE creating UI
    ui_nav_init();
    ui_panel_home_init_subjects();
    ui_panel_controls_temp_init_subjects();
    // ... all panel subjects

    // 4. Create UI
    lv_obj_t* screen = lv_xml_create(NULL, "app_layout", NULL);

    // 5. Setup observers and wire events
    ui_panel_home_setup_observers(panels[UI_PANEL_HOME]);
    ui_panel_controls_wire_events(panels[UI_PANEL_CONTROLS]);

    // 6. Start event loop
    while (lv_display_get_next(NULL)) {
        lv_timer_handler();
        SDL_Delay(5);
    }
}
```

**Component Naming:** LVGL uses **filename** as component name:
- File: `nozzle_temp_panel.xml` → Component: `nozzle_temp_panel`
- File: `controls_panel.xml` → Component: `controls_panel`

---

## Testing Commands

```bash
# Build
make                          # Incremental build (auto-parallel)
make clean && make            # Clean rebuild

# Run with different configs
./build/bin/helix-ui-proto                    # Default (medium screen, home panel)
./build/bin/helix-ui-proto -s tiny            # 480x320 screen
./build/bin/helix-ui-proto -s large           # 1280x720 screen
./build/bin/helix-ui-proto -p controls        # Start at Controls panel
./build/bin/helix-ui-proto -p print-select    # Print select card view
./build/bin/helix-ui-proto -p file-detail     # Print file detail overlay (NEW)
./build/bin/helix-ui-proto -s small -p print-select  # Combined options

# Controls (when running)
# Press Cmd+Q (macOS) or Win+Q (Windows) to quit
# Press 'S' key to save screenshot
# Close window to exit

# Screenshot
./scripts/screenshot.sh helix-ui-proto output-name [panel-name]

# Examples:
./scripts/screenshot.sh helix-ui-proto home-test home
./scripts/screenshot.sh helix-ui-proto controls-launcher controls
./scripts/screenshot.sh helix-ui-proto motion-panel motion
./scripts/screenshot.sh helix-ui-proto file-detail file-detail

# Unit tests
make test                     # Run all tests
```

**Screen Size Mappings:**
- `tiny` = 480x320
- `small` = 800x480
- `medium` = 1024x600 (default)
- `large` = 1280x720

**Panel Name Mappings:**
- `home` - Home panel
- `controls` - Controls launcher
- `motion` - Motion sub-screen (overlay)
- `nozzle-temp` - Nozzle temperature sub-screen
- `bed-temp` - Bed temperature sub-screen
- `extrusion` - Extrusion sub-screen
- `print-select` - Print select panel (card view)
- `file-detail` - Print file detail view (overlay)
- `filament` - Filament panel
- `settings` - Settings panel
- `advanced` - Advanced panel

---

## Immediate Next Steps

**These tasks should be completed in the next session:**

### 1. Apply Responsive Pattern to Other Panels
- **Action:** Review motion/temp/extrusion panels for hardcoded sizing
- **Action:** Replace with semantic constants where applicable
- **Pattern:** Use same approach as numeric keypad (flex_grow, semantic constants)
- **Goal:** Ensure all panels work across all screen sizes (480×320 to 1280×720)

### Notes for Next Developer

- Icon constants in `globals.xml` will **look empty** in your editor - this is normal
- Use `python3 scripts/generate-icon-consts.py` to add new icons (edit ICONS dict in script)
- All FontAwesome fonts (16/24/32/48/64) are already declared in `ui_fonts.h`
- Use `fa_icons_24` for small inline icons to match `montserrat_20` text

---

## Next Priorities

### Priority 1: Moonraker Integration 🔌 **PRIMARY FOCUS**

**Status:** ✅ UI complete and tested. Ready to connect to live printer.

**Why this is next:** All UI components are functional with mock data. Navigation system is robust. The next logical step is to replace mock data with live Moonraker API data.

**Integration Tasks:**
1. **WebSocket Client Setup** - Connect to Moonraker API
2. **Printer State Subscription** - Subscribe to printer status updates
3. **Wire Button Actions** - Connect UI buttons to Moonraker commands:
   - Motion jog buttons → `printer.gcode.script` (G0/G1 commands)
   - Temperature presets → `printer.gcode.script` (M104/M140)
   - Print start → `printer.print.start`
   - Print pause/resume → `printer.print.pause`/`printer.print.resume`
   - Print cancel → `printer.print.cancel`
4. **Update Subject Bindings** - Replace mock data with live printer state:
   - Temperature subjects → `extruder.temperature`, `heater_bed.temperature`
   - Position subjects → `toolhead.position`
   - Print progress → `virtual_sdcard.progress`
   - File list → `server.files.list`

**Test Commands:**
```bash
# Test with live printer (requires Moonraker URL in config)
./build/bin/helix-ui-proto
# Click any file → Print button → observe print status panel
```

**Existing Subjects (already wired):**
- `print_filename_text` - File being printed
- `print_progress_text` - "45%" format
- `print_progress_value` - 0-100 integer for progress bar
- `print_layer_text` - "125 / 250" format
- `print_elapsed_time_text` - "1h 23m" format
- `print_remaining_time_text` - "1h 37m" format
- `print_nozzle_temp_text` - "215 / 215°C" format
- `print_bed_temp_text` - "60 / 60°C" format
- `print_speed_text` - "100%" format
- `print_flow_text` - "100%" format
- `print_state_text` - "Printing" / "Paused" / "Complete" / "Cancelled"

**Implementation Steps:**

**Step 1: WebSocket Foundation**
- Review existing HelixScreen Moonraker client code (parent repo)
- Adapt libhv WebSocket implementation for prototype
- Connect to Moonraker on startup
- Handle connection/disconnection events

**Step 2: Printer Status Updates**
- Subscribe to printer object updates
- Wire temperature subjects to live data:
  - `extruder.temperature` → nozzle temp subjects
  - `heater_bed.temperature` → bed temp subjects
- Update home panel displays with real-time temps

**Step 3: Motion & Control Commands**
- Motion jog buttons → `printer.gcode.script` (G0/G1 commands)
- Temperature presets → M104/M140 commands
- Home buttons → G28 commands
- Query printer config for dynamic limits:
  ```cpp
  ui_panel_controls_temp_set_nozzle_limits(extruder.min_temp, extruder.max_temp);
  ui_panel_controls_temp_set_bed_limits(heater_bed.min_temp, heater_bed.max_temp);
  ```

**Step 4: Print Management**
- File list → `server.files.list` API
- Print start → `printer.print.start`
- Print pause/resume → `printer.print.pause`/`.resume`
- Print cancel → `printer.print.cancel`
- Live print status updates (progress, layer, times)

**Reference Code:**
- Parent repo: `/Users/pbrown/code/helixscreen/` has working Moonraker client
- Look for WebSocket handling, JSON-RPC protocol, subscription management

**Testing Strategy:**
1. Start with read-only status (temps, positions)
2. Add control commands incrementally
3. Test each command with real printer before moving to next
4. Use parent repo's simulator mode if available

---

### Priority 2: Future Enhancements

- Temperature graph visualization (replace static fire icon)
- File browser pagination/search
- Multi-language support
- Animations and transitions

---

## Known Gotchas

### LVGL 9 XML Flag Attribute Syntax (RESOLVED ✅)

**Problem:** Using `flag_*` prefix in XML attributes causes them to be silently ignored.

**Wrong Syntax (doesn't work):**
```xml
<lv_button flag_hidden="true" flag_clickable="false" flag_scrollable="false"/>
```

**Correct Syntax:**
```xml
<lv_button hidden="true" clickable="false" scrollable="false"/>
```

**Why:** LVGL 9 XML property system auto-generates simplified attribute names without the `flag_` prefix. The C enum is `LV_PROPERTY_OBJ_FLAG_HIDDEN` but the XML attribute is just `hidden`.

**Status:** ✅ **FIXED** - All XML files updated as of 2025-10-24. All instances of `flag_hidden`, `flag_clickable`, and `flag_scrollable` have been corrected across the codebase.

**Reference:** `docs/LVGL9_XML_ATTRIBUTES_REFERENCE.md:91-123` for complete list of correct flag attribute names.

### LV_SIZE_CONTENT Width Bug

**Problem:** Labels inside XML components with property substitution render with zero width despite having content.

**Solution:** Use explicit pixel dimensions instead of `width="LV_SIZE_CONTENT"`:

```xml
<!-- ✗ WRONG - Renders with zero width -->
<lv_label width="LV_SIZE_CONTENT" bind_text="time_text"/>

<!-- ✓ CORRECT - Use explicit dimensions -->
<lv_label width="65" bind_text="time_text"/>  <!-- Fits "2h30m" -->
```

See `docs/LVGL9_XML_GUIDE.md` for complete troubleshooting.

### Subject Type Must Match API

**Image recoloring requires color subjects:**
```cpp
// ✓ CORRECT - Color subject + image recolor API
lv_subject_init_color(&subject, lv_color_hex(0xFFD700));
lv_obj_set_style_img_recolor(widget, color, LV_PART_MAIN);
lv_obj_set_style_img_recolor_opa(widget, 255, LV_PART_MAIN);

// ✗ WRONG - String subject + text color (doesn't work on images)
lv_subject_init_string(&subject, buffer, NULL, size, "0xFFD700");
lv_obj_set_style_text_color(widget, color, 0);
```

### Component Instantiation Requires Explicit Names

Always add `name` attributes to component tags in XML or `lv_obj_find_by_name()` will return NULL.

### LVGL 9 XML Silent Failures

Parser ignores unknown attributes without warnings. When attributes don't work, verify against source code in `/lvgl/src/others/xml/parsers/`.

---

## Documentation

- **[STATUS.md](STATUS.md)** - Complete chronological development journal
- **[ROADMAP.md](docs/ROADMAP.md)** - Planned features and milestones
- **[LVGL9_XML_GUIDE.md](docs/LVGL9_XML_GUIDE.md)** - Complete LVGL 9 XML reference
- **[QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md)** - Common patterns quick lookup
- **[COPYRIGHT_HEADERS.md](docs/COPYRIGHT_HEADERS.md)** - Required GPL v3 headers for new files

---

## File Organization

```
prototype-ui9/
├── src/              # C++ business logic
├── include/          # Headers
├── ui_xml/           # XML component definitions
├── assets/           # Fonts, images, Material Design icons
├── scripts/          # Build/screenshot/icon conversion automation
├── docs/             # Documentation
└── Makefile          # Build system
```

**Key Files:**
- `src/main.cpp` - Entry point, initialization order, CLI args
- `ui_xml/globals.xml` - Theme constants, color definitions, icon variants
- `ui_xml/app_layout.xml` - Root layout (navbar + content area)
- `lv_conf.h` - LVGL configuration (has `LV_USE_OBJ_NAME=1`)

---

**End of Handoff Document**

For complete development history, see **STATUS.md**.
For implementation patterns and troubleshooting, see **docs/LVGL9_XML_GUIDE.md**.
