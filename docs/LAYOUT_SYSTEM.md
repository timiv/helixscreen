# Layout System

> Alternative XML layouts for different screen aspect ratios and orientations.

## Overview

HelixScreen's XML-based UI is authored for standard landscape displays (4:3 to 16:9).
Users with ultrawide screens (1920x480), portrait-mounted displays, or very small screens
need fundamentally different layouts — not just responsive scaling, but different XML structures.

The **Layout System** provides a mechanism for loading alternative XML files based on the
detected (or configured) display geometry, with graceful fallback to the standard layout.

**Key distinction:** Themes control *colors*. Layouts control *structure*. They are orthogonal —
any theme works with any layout.

## Terminology

| Term | Meaning |
|------|---------|
| **Layout** | A named set of XML overrides for a specific display geometry (e.g., `ultrawide`, `portrait`) |
| **Standard** | The default layout — current XML files, no overrides |
| **Fallback** | When a layout doesn't override a file, the standard version is used |
| **Slot** | A content region in multi-panel layouts (ultrawide dashboard) |

## Layout Families

| Layout | Aspect Ratio | Example Resolutions | Notes |
|--------|-------------|---------------------|-------|
| `standard` | ~4:3 to ~16:9 | 800x480, 1024x600, 1280x720 | Current default, all existing XML |
| `ultrawide` | >2.5:1 | 1920x480, 1920x400 | Multi-panel dashboard |
| `portrait` | <0.8:1 | 480x800, 600x1024 | Rotated displays |
| `tiny` | landscape, max dim ≤480 | 480x320 | Very small displays |
| `tiny-portrait` | portrait, max dim ≤480 | 320x480 | Very small portrait |

## Auto-Detection

At startup, the layout is determined by:

1. **Explicit override** — `--layout <name>` CLI flag or `display.layout` in helixconfig.json
2. **Auto-detection** from display resolution (if layout is `"auto"` or unset):

```
ratio = width / height

if (ratio > 2.5)                    → ultrawide
else if (ratio < 0.8 && max ≤ 480)  → tiny-portrait
else if (ratio < 0.8)               → portrait
else if (max(w, h) ≤ 480)           → tiny
else                                 → standard
```

The detected layout name is logged at startup and available via `LayoutManager::layout_name()`.

## Directory Structure

```
ui_xml/
  globals.xml                    # Always shared (design tokens, constants)
  app_layout.xml                 # Standard app chrome
  home_panel.xml                 # Standard panels
  settings_panel.xml
  print_select_panel.xml
  ...
  navigation_bar.xml             # Standard navigation
  button_card.xml                # Shared reusable components
  toggle_row.xml
  ...

  ultrawide/                     # Ultrawide overrides
    app_layout.xml               # Multi-slot dashboard chrome
    home_panel.xml               # Dashboard-width home
    ...
    button_card.xml              # Component override (only if needed)

  portrait/                      # Portrait overrides
    app_layout.xml               # Vertical navigation
    home_panel.xml
    ...

  tiny/                          # Tiny landscape overrides
    ...

  tiny-portrait/                 # Tiny portrait overrides
    ...
```

### Resolution Order

When registering XML component `"home_panel"` with active layout `ultrawide`:

1. Look for `ui_xml/ultrawide/home_panel.xml` — **use it if it exists**
2. Fall back to `ui_xml/home_panel.xml` (standard)

This means:
- A new layout starts **empty** and inherits everything from standard
- Layouts only need to provide files that actually differ
- `globals.xml` is **always** loaded from the root (never overridden per-layout)
- Components, panels, and app_layout all follow the same fallback rule

## Architecture

### LayoutManager

New singleton class responsible for layout detection and XML path resolution.

```cpp
// include/layout_manager.h

class LayoutManager {
public:
    static LayoutManager& instance();

    // Initialize with display dimensions. Call after DisplayManager init.
    void init(int display_width, int display_height);

    // Override auto-detection (from CLI/config). Call before init() or re-call init().
    void set_layout_override(const std::string& layout_name);

    // Get the active layout name ("standard", "ultrawide", etc.)
    const std::string& layout_name() const;

    // Resolve an XML filename to its layout-aware path.
    // Returns "ui_xml/<layout>/filename.xml" if override exists,
    // otherwise "ui_xml/filename.xml".
    std::string resolve_xml_path(const std::string& filename) const;

    // Check if a layout-specific override exists for a file.
    bool has_override(const std::string& filename) const;

private:
    std::string layout_name_{"standard"};
    std::string layout_dir_;          // "ui_xml/<layout>/" or empty for standard
    bool initialized_{false};

    std::string detect_layout(int width, int height) const;
};
```

### Integration Points

#### Startup Sequence (application.cpp)

```
Phase 4: Display init (existing)
Phase 4.5: LayoutManager init    ← NEW
  - Read --layout CLI / display.layout config
  - If auto: detect from display dimensions
  - Log: "Layout: ultrawide (auto-detected from 1920x480)"
Phase 5: Asset registration (existing)
Phase 6: Theme init (existing)
Phase 7: Widget registration (existing)
Phase 8: XML component registration ← MODIFIED to use LayoutManager
Phase 9: Subject init (existing)
Phase 10: UI creation (existing)
```

#### XML Registration (xml_registration.cpp)

Current:
```cpp
lv_xml_register_component_from_file("A:ui_xml/home_panel.xml");
```

Modified:
```cpp
auto& lm = LayoutManager::instance();
lv_xml_register_component_from_file(
    ("A:" + lm.resolve_xml_path("home_panel.xml")).c_str()
);
```

Or, helper function:
```cpp
void register_xml(const std::string& filename) {
    auto& lm = LayoutManager::instance();
    std::string path = "A:" + lm.resolve_xml_path(filename);
    lv_xml_register_component_from_file(path.c_str());
}

// Usage:
register_xml("home_panel.xml");  // Automatically resolves layout override
```

### Config Schema

```json
// helixconfig.json
{
  "display": {
    "layout": "auto"
  }
}
```

Valid values: `"auto"` (default), `"standard"`, `"ultrawide"`, `"portrait"`, `"tiny"`, `"tiny-portrait"`.

CLI: `--layout <name>` overrides config file.

## Ultrawide Dashboard Design (Future — Phase 2)

The ultrawide layout replaces the single `panel_container` with multiple **slots**:

```xml
<!-- ui_xml/ultrawide/app_layout.xml -->
<lv_obj name="dashboard" style_flex_flow="row" width="100%" height="100%">
  <lv_obj name="nav_sidebar" width="64">
    <!-- Vertical icon-only nav -->
  </lv_obj>
  <lv_obj name="slot_primary" style_flex_grow="2">
    <!-- Main panel (larger) -->
  </lv_obj>
  <lv_obj name="slot_secondary" style_flex_grow="1">
    <!-- Secondary panel (smaller) -->
  </lv_obj>
</lv_obj>
```

**Slot defaults** (hardcoded initially):
- Primary: home_panel (or print_status when printing)
- Secondary: controls_panel (or console)

**Navigation model change:**
- Nav items swap the primary slot content
- Secondary slot stays persistent (or has its own mini-nav)
- Overlays span both slots

**Panel width requirements:**
- Panels must render correctly at ~640px wide (1920 - 64 sidebar) / 3 ≈ ~620px
- Or 2-slot: ~960px and ~640px
- Existing responsive tokens help but panels may need ultrawide-specific XML

## Portrait Design (Future — Phase 3)

Portrait layout likely needs:
- Bottom navigation bar (instead of left sidebar)
- Vertically-stacked panel content
- Different overlay positioning
- Scrollable panels for content that doesn't fit vertically

## Implementation Phases

### Phase 1: Layout Infrastructure (Recommended First)

**Goal:** Build the plumbing with zero user-visible changes. `standard` layout = current behavior.

**Files to create:**
- `include/layout_manager.h` — LayoutManager class
- `src/layout_manager.cpp` — Detection logic, path resolution

**Files to modify:**
- `src/application.cpp` — Init LayoutManager after display, before XML registration
- `src/xml_registration.cpp` — Use `resolve_xml_path()` for all XML file loading
- `include/runtime_config.h` — Add `--layout` CLI option
- `src/runtime_config.cpp` — Parse `--layout`

**Tests:**
- Layout detection from various resolutions
- XML path resolution with and without overrides
- Fallback behavior when override directory is empty
- CLI override vs auto-detection precedence

**Acceptance criteria:**
- `--layout standard` produces identical behavior to current (no regression)
- `--layout ultrawide` logs the layout name but falls back to standard XML (no overrides exist yet)
- Auto-detection correctly classifies known resolutions

### Phase 2: Ultrawide Layout

- Create `ui_xml/ultrawide/app_layout.xml` with multi-slot dashboard
- Modify PanelFactory for multi-panel instantiation
- Create ultrawide panel XMLs (start with home + controls)
- Implement slot navigation

### Phase 3: Portrait Layout

- Create `ui_xml/portrait/` directory with vertical-first layouts
- Bottom nav implementation
- Portrait panel XMLs

### Phase 4: Tiny Variants

- Simplified layouts for very small displays
- Reduced information density, larger touch targets

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Naming | "Layouts" | Clear, distinct from "themes" (colors) and "profiles" (print start) |
| Detection | Auto with override | Users shouldn't need to configure, but can |
| Fallback | Per-file to standard | New layouts start empty, override incrementally |
| globals.xml | Never overridden | Design tokens are universal, responsive system still applies |
| Slot config | Hardcoded defaults | Configurability adds complexity; get layouts right first |
| Runtime switching | Not supported | LVGL widget tree would need full rebuild; startup-only is fine |
