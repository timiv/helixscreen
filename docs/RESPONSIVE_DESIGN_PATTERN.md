## Responsive Design Pattern

**Pattern Established:** 2025-10-25 with numeric keypad redesign

### Mobile-First Approach

Design for the smallest screen first (480×320), then let flex layout scale up naturally.

### Key Principles

1. **Use Semantic Constants Only**
   - Never hardcode pixel values in XML
   - Use constants from `globals.xml`:
     - `#padding_small` / `#padding_medium` / `#padding_large`
     - `#gap_tiny` / `#gap_small` / `#gap_normal` / `#gap_large`
     - Font constants: `montserrat_16`, `montserrat_20`, `montserrat_28`
   - Remove component-specific sizing constants if semantic ones work

2. **Flex Layout for Responsiveness**
   - Container: `flex_grow="1"` to fill available space
   - Rows: `flex_grow="1"` with `space_evenly` for equal distribution
   - Buttons/cells: `flex_grow="1"` and `height="100%"`
   - Headers/cards: `height="content"` for natural sizing

3. **Mobile-First Font Sizes**
   - Choose fonts that work on tiny screens
   - Larger screens get the same compact fonts (which look fine)
   - Prefer `montserrat_16` or `montserrat_20` for UI elements

4. **No C++ Runtime Adjustments**
   - All sizing logic stays in XML
   - Easier to maintain and understand
   - One source of truth

### Example: Numeric Keypad

**Before (Fixed Sizing):**
```xml
<lv_obj height="60">  <!-- Hardcoded -->
  <lv_label style_text_font="montserrat_28"/>  <!-- Too big for tiny -->
</lv_obj>
<lv_button height="100">  <!-- Doesn't fit tiny screen -->
```

**After (Responsive):**
```xml
<lv_obj height="content" style_pad_all="#padding_small">
  <lv_label style_text_font="montserrat_20"/>
</lv_obj>
<lv_button flex_grow="1" height="100%">  <!-- Fills available space -->
```

### Testing

Use the multi-size test pattern:

```bash
# Test specific panel at all sizes
for size in tiny small medium large; do
  ./build/bin/helix-ui-proto -s $size -p panel-name
done
```

Or create a dedicated test script (see `scripts/test_keypad_sizes.sh`).

### Screen Size Reference

- Tiny: 480×320 (height: 320px) - **Design for this first**
- Small: 800×480 (height: 480px)
- Medium: 1024×600 (height: 600px)
- Large: 1280×720 (height: 720px)

### C++ Component Pattern for Dynamic Responsiveness

**When XML constants aren't sufficient** (e.g., header height, vertical padding), use C++ component wrappers:

#### Example: Header Bar Component (2025-10-25)

**Problem:** Fixed `#header_height` constant (60px) wastes space on tiny/small screens.

**Solution:** Component wrapper with responsive behavior:

```cpp
// src/ui_component_header_bar.cpp
void ui_component_header_bar_setup(lv_obj_t* header, lv_obj_t* screen) {
    // Apply responsive height based on screen size
    lv_coord_t height = ui_get_responsive_header_height(lv_obj_get_height(screen));
    lv_obj_set_height(header, height);

    // Register for global resize events
    header_instances.push_back(header);
}
```

**Usage in panels:**
```cpp
void ui_panel_motion_setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    // One-line setup call
    lv_obj_t* motion_header = lv_obj_find_by_name(panel, "motion_header");
    if (motion_header) {
        ui_component_header_bar_setup(motion_header, parent_screen);
    }
}
```

**Helper functions pattern:**
```cpp
// ui_utils.h / ui_theme.h
lv_coord_t ui_get_responsive_header_height(lv_coord_t screen_height) {
    if (screen_height >= UI_SCREEN_MEDIUM_H) return 60;  // Large/medium
    else if (screen_height >= UI_SCREEN_SMALL_H) return 48;  // Small
    else return 40;  // Tiny
}
```

**Benefits:**
- Component encapsulates its own behavior
- Single setup call per panel (explicit, simple)
- Automatic resize handling through global system
- Follows LVGL patterns (no automatic constructors)

#### Vertical Padding Pattern

For content areas below headers, use **split padding** (vertical responsive, horizontal fixed):

```cpp
lv_coord_t v_padding = ui_get_header_content_padding(screen_height);
lv_obj_set_style_pad_top(content, v_padding, 0);
lv_obj_set_style_pad_bottom(content, v_padding, 0);
lv_obj_set_style_pad_left(content, UI_PADDING_MEDIUM, 0);  // Fixed 12px
lv_obj_set_style_pad_right(content, UI_PADDING_MEDIUM, 0);
```

**Rationale:** Vertical space is scarce on small screens; horizontal padding can stay consistent.

### Panels Updated with Responsive Pattern

- [x] Numeric keypad ✅ (2025-10-24)
- [x] Header heights (all panels) ✅ (2025-10-25)
- [x] Vertical padding (all panels) ✅ (2025-10-25)
  - Motion panel
  - Temperature panels (nozzle/bed)
  - Extrusion panel
  - Print status panel
