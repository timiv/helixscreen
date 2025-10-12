# Print Select Panel - UI Requirements v1

**Panel Name:** Print Select Panel
**Inspiration:** Bambu Lab X1C print file browser
**Status:** Draft - Discussion Phase
**Date:** 2025-10-11

---

## Overview

The Print Select Panel displays a browsable grid of 3D print files from either internal storage or SD card, with thumbnails and metadata (print time, filament usage). Users can select files to print.

**Key Features:**
- Tab navigation between Internal and SD card storage
- Scrollable grid of print file cards (4 columns)
- Each card shows: thumbnail preview, filename, print time, filament weight
- Clock badge overlay on thumbnails
- Responsive card layout

---

## 1. Panel Layout Structure

### 1.1 Overall Layout
- [ ] **Layout Type:** Vertical flex container
- [ ] **Sections:** Two main sections (tab bar + content area)
- [ ] **Background:** Dark panel background (#252729 or `#panel_bg`)

### 1.2 Tab Bar (Top Section)
- [ ] **Height:** 56px
- [ ] **Background:** Transparent or subtle darker shade
- [ ] **Layout:** Horizontal flex, left-aligned
- [ ] **Padding:** 16px horizontal, 8px vertical
- [ ] **Bottom Border:** Optional 1px separator line

### 1.3 Content Area (File Grid)
- [ ] **Layout:** Scrollable container with flex grid
- [ ] **Columns:** 4 cards per row
- [ ] **Scroll:** Vertical scrolling enabled
- [ ] **Padding:** 16px all sides
- [ ] **Gap:** 20px horizontal, 20px vertical between cards

**DECISION POINT:**
- Should cards be fixed width or flex to fill available space?
- What happens on smaller/larger screens?

---

## 2. Tab Navigation

### 2.1 Tab Structure
- [ ] Two tabs: "Internal" and "SD card"
- [ ] Horizontal layout with spacing between tabs
- [ ] Tab spacing: 32px gap between tab labels

### 2.2 Active Tab Style
- [ ] **Text Color:** #ffffff (white) or `#text_primary`
- [ ] **Font:** montserrat_20 or similar
- [ ] **Font Weight:** Medium/bold
- [ ] **Indicator:** Subtle underline or no indicator (TBD)

### 2.3 Inactive Tab Style
- [ ] **Text Color:** #808080 (gray) or `#text_secondary`
- [ ] **Font:** montserrat_20 or similar
- [ ] **Font Weight:** Regular

### 2.4 Tab Interaction
- [ ] Clickable tabs switch active storage source
- [ ] Tab state stored in integer subject (0=Internal, 1=SD)
- [ ] Tab text color updates reactively based on active tab

**DECISION POINTS:**
- Do we add a visual indicator (underline/highlight) for active tab?
- Should there be a transition animation when switching tabs?
- Do we need a "refresh" button in the tab bar?

---

## 3. Print File Card Component

### 3.1 Card Dimensions
- [x] **Width:** Flex-based with min-width ~240px (responsive: 3-5 columns based on screen)
- [x] **Height:** Auto (based on content)
- [x] **Padding:** 12px all sides
- [x] **Border Radius:** 8px
- [x] **Background:** #3a3d42 (dark gray) or `#card_bg`

### 3.2 Card Structure (Vertical Stack)
Components in order from top to bottom:

1. **Thumbnail section** (square, no badge in v1)
2. **Filename label** (single line, truncated)
3. **Metadata row** (print time + filament weight)
4. **Optional: Chevron icon** (DEFERRED - not in v1)

### 3.3 Thumbnail Section
- [x] **Size:** Square, ~236px × 236px (based on card flex width)
- [x] **Aspect Ratio:** Crop to square (center crop)
- [x] **Border Radius:** 6px (slightly less than card radius)
- [x] **Background:** Dark gray if image missing (#3a3d42)
- [ ] **Placeholder:** Icon or text for missing thumbnails (TBD)

### 3.4 Clock Badge Overlay
**STATUS:** DEFERRED - Not included in v1 implementation

### 3.5 Filename Label
- [x] **Font:** montserrat_16
- [x] **Color:** #ffffff (white) or `#text_primary`
- [x] **Alignment:** Left
- [x] **Max Lines:** 1 (single line)
- [x] **Truncation:** "..." ellipsis for long filenames
- [x] **Margin Top:** 8px (spacing from thumbnail)

### 3.6 Metadata Row
- [x] **Layout:** Horizontal flex, flex-start with gap
- [x] **Margin Top:** 4px (spacing from filename)
- [x] **Gap:** 12px between metadata items

**Metadata Item Structure:**
- [x] Icon + text pairs, horizontal layout
- [x] Icon: 16px FontAwesome icon (fa_icons_16 - to be created)
- [x] Text: montserrat_14
- [x] Color: #4a9d5f (green tint) - new constant `#accent_green`
- [x] Gap between icon and text: 4px

**Metadata Items:**
1. **Print Time:**
   - [x] Icon: Clock icon (fa-clock-o, `\uf017`)
   - [x] Text: "19m", "1h20m", "2h1m" format

2. **Filament Weight:**
   - [x] Icon: Leaf icon (fa-leaf, `\uf06c`)
   - [x] Text: "4g", "30g", "12.04g" format

### 3.7 Optional Chevron Icon
**STATUS:** DEFERRED - Not included in v1 implementation

---

## 4. Scrolling Behavior

### 4.1 Scroll Container
- [ ] **Container:** `lv_obj` with `flex_flow="row_wrap"` for grid
- [ ] **Scrollable:** Enable vertical scrolling (`flag_scrollable="true"`)
- [ ] **Scroll Bar:** Show on scroll, auto-hide when not scrolling
- [ ] **Scroll Bar Style:** Subtle, matches theme

### 4.2 Content Layout
- [x] Cards flow left-to-right, wrap to next row
- [x] Responsive: 3-5 cards per row (based on screen width and card min-width)
- [x] Consistent gaps between cards and rows (20px)
- [x] Bottom padding to prevent last row from touching edge (16px)

**Implementation Note:**
- Use `flex_flow="row_wrap"` with card `min_width="240px"` and `flex_grow="1"`
- Cards will naturally wrap based on available space
- No media queries needed (LVGL may not support them anyway)

---

## 5. Color Palette - OUR THEME (NO NEW COLORS)

### 5.1 Existing Colors (from globals.xml) - ALL WE NEED
- [x] `#panel_bg` (0x111410) - Panel background, tab bar background
- [x] `#card_bg` (0x202020) - Card backgrounds
- [x] `#text_primary` (0xffffff) - Filename text, active tab text
- [x] `#text_secondary` (0x909090) - Inactive tab text
- [x] `#primary_color` (0xff4444) - Metadata text, metadata icons
- [x] `#accent_color` (0x00aaff) - (Available if needed)

### 5.2 UI Element → Color Mapping (DECIDED 2025-10-11)
**Backgrounds:**
- Panel background → `#panel_bg`
- Tab bar background → `#panel_bg`
- Card background → `#card_bg`

**Text:**
- Filename (primary info) → `#text_primary` (white)
- Active tab label → `#text_primary` (white)
- Inactive tab label → `#text_secondary` (gray)
- Metadata text (time, weight) → `#primary_color` (red)

**Icons:**
- Metadata icons (clock, leaf) → `#primary_color` (red)

**Result:** Zero new colors needed, using existing palette

---

## 6. Typography

### 6.1 Existing Fonts (from globals.xml)
- [ ] `montserrat_16` - Body text
- [ ] `montserrat_20` - Section headers
- [ ] `fa_icons_32` - FontAwesome icons
- [ ] `fa_icons_48` - Larger icons

### 6.2 New Fonts Needed
- [x] `fa_icons_16` - Small icons for metadata (16px FontAwesome)

**Icons to include in fa_icons_16:**
- Clock icon: `fa-clock-o` (`\uf017`)
- Leaf icon: `fa-leaf` (`\uf06c`)

---

## 7. Spacing & Dimensions

### 7.1 Global Spacing
- [ ] Tab bar height: 56px
- [ ] Content padding: 16px all sides
- [ ] Card gap (horizontal): 20px
- [ ] Card gap (vertical): 20px

### 7.2 Card Internal Spacing
- [ ] Card padding: 12px all sides
- [ ] Thumbnail to filename gap: 8px
- [ ] Filename to metadata gap: 4px
- [ ] Metadata icon to text gap: 4px
- [ ] Metadata items gap: 12px

### 7.3 Badge Positioning
- [ ] Badge offset from thumbnail edge: 8px (top-left)
- [ ] Badge diameter: 36px

**DECISION POINT:**
- Are these spacing values consistent with home panel patterns?

---

## 8. Data Binding & Reactivity

### 8.1 Subject Requirements

**Active Tab Subject:**
- [ ] Type: Integer
- [ ] Name: `active_storage_tab`
- [ ] Values: 0 (Internal), 1 (SD card)
- [ ] Used by: Tab text colors (reactive binding)

**File List Subject:**
- [ ] Type: Array of file data structures (TBD implementation)
- [ ] Name: `print_files`
- [ ] Updates: When tab changes or files refresh
- [ ] Used by: Dynamic card generation

**DECISION POINTS:**
- How do we implement dynamic list of cards in LVGL 9 XML?
- Should each card have its own subjects or use a different pattern?
- Do we generate cards in C++ and add them to XML container?

### 8.2 File Data Structure (C++)
```cpp
struct PrintFile {
    std::string filename;
    std::string thumbnail_path;  // Path to extracted thumbnail
    int print_time_minutes;      // Total print time
    float filament_grams;        // Filament weight
    bool has_thumbnail;          // Flag if thumbnail exists
};
```

---

## 9. Interactive Elements

### 9.1 Tab Clicks
- [ ] Tabs are clickable
- [ ] Click handler switches active tab
- [ ] Tab change triggers file list refresh
- [ ] Visual feedback on click (optional)

### 9.2 Card Clicks
- [ ] Cards are clickable (entire card area)
- [ ] Click opens file details or starts print dialog (future)
- [ ] Visual feedback on click (optional: subtle highlight/scale)
- [ ] Long press for context menu (future)

### 9.3 Scrolling
- [ ] Touch drag for vertical scrolling
- [ ] Momentum scrolling enabled
- [ ] Scroll bar appears on interaction

**DECISION POINTS:**
- What happens when you click a card in this version?
- Do we need a separate "print" button or is card click sufficient?
- Should cards have hover/press states?

---

## 10. Edge Cases & Constraints

### 10.1 Empty States
- [ ] **No files:** Display message "No print files found"
- [ ] **Loading:** Show loading indicator while scanning files
- [ ] **SD card missing:** Show message "SD card not detected"

### 10.2 File Constraints
- [ ] Maximum files displayed: Unlimited (scrollable)
- [ ] File types supported: .gcode, .ufp (Bambu format), .bgcode
- [ ] Thumbnail extraction: From GCode metadata
- [ ] Missing thumbnail: Show placeholder icon/image

### 10.3 Performance
- [ ] Lazy loading for thumbnails (load on demand)
- [ ] Maximum cards loaded at once (virtual scrolling?)
- [ ] Thumbnail caching strategy

**DECISION POINTS:**
- Do we implement virtual scrolling or load all cards?
- How do we handle hundreds of files?
- Should we limit displayed files and add pagination/search?

---

## 11. Integration Questions

### 11.1 Navigation Integration
**Option A: Sub-panel of Home**
- Home panel has "Print Files" card that navigates to this panel
- Uses same panel area, replaces home content
- Needs back button to return to home

**Option B: Separate Top-Level Panel**
- New navigation icon in left navbar
- Becomes 6th panel in navigation
- Direct access from navbar

**DECISION POINT:** Which approach fits better with the overall UI flow?

### 11.2 File System Integration
- [ ] How do we scan for files (Moonraker API)?
- [ ] Where are thumbnails stored (extracted to temp dir)?
- [ ] How do we refresh file list (manual button or automatic)?

---

## 12. Implementation Phases

### Phase 1: Static Layout (No Data)
- [ ] Create XML structure for panel
- [ ] Implement tab bar with static tabs
- [ ] Create single static card with placeholder content
- [ ] Verify layout and styling

### Phase 2: Card Component Refinement
- [ ] Add thumbnail image support
- [ ] Implement badge overlay positioning
- [ ] Style all card elements (filename, metadata)
- [ ] Test with multiple static cards

### Phase 3: Grid & Scrolling
- [ ] Implement 4-column grid layout
- [ ] Add 12-20 static cards for scrolling test
- [ ] Enable and style scrolling behavior
- [ ] Verify spacing and alignment

### Phase 4: Reactive Tab Switching
- [ ] Create C++ wrapper with tab subject
- [ ] Add click handlers to tabs
- [ ] Implement reactive tab color updates
- [ ] Test tab switching

### Phase 5: Dynamic Card Generation
- [ ] Design approach for dynamic card creation
- [ ] Implement C++ file data structure
- [ ] Generate cards from data in C++
- [ ] Test with mock file data

### Phase 6: Real Data Integration
- [ ] Integrate with file system scanning
- [ ] Extract thumbnails from GCode metadata
- [ ] Implement thumbnail loading/caching
- [ ] Handle edge cases (missing thumbnails, empty lists)

---

## 13. Design Decisions (APPROVED 2025-10-11)

**Critical Decisions Made:**

1. ✅ **Navigation:** Separate top-level panel (6th icon in navbar)
2. ✅ **Card Width:** Flex-based responsive (min-width ~240px, adapts to 3-5 columns based on screen width)
3. ✅ **Thumbnails:** Square crop to 236×236px (uniform grid)
4. ✅ **Clock Badge:** SKIPPED in v1 (not necessary for initial implementation)
5. ✅ **Filename:** Single line with ellipsis truncation (uniform card heights)
6. ✅ **Metadata Icons:** Create fa_icons_16 font (16px FontAwesome)
7. ✅ **Filament Icon:** Leaf icon (FontAwesome fa-leaf `\uf06c`)
8. ✅ **Dynamic Cards:** XML template component + C++ instantiation (or hybrid fallback)
9. ✅ **Scrolling:** Load all cards upfront (refactor to virtual scrolling if performance issues)
10. ✅ **Card Click:** Register handler only, implement behavior in later phase

**Design Refinements:**
- Active tab visual indicator (underline/highlight)?
- Tab transition animations?
- Card hover/press visual feedback?
- Refresh button in tab bar?
- Chevron icon in v1 or defer?
- Badge border styling?
- Empty state designs?
- Loading state indicators?

---

## 14. Success Criteria

**Visual Verification:**
- [ ] Layout matches X1C reference screenshot proportions
- [ ] Cards are evenly spaced in 4-column grid
- [ ] Tabs clearly show active/inactive state
- [ ] Badge overlay positioned correctly on thumbnails
- [ ] Metadata icons and text properly aligned
- [ ] Colors match defined palette
- [ ] Scrolling is smooth with proper momentum

**Functional Verification:**
- [ ] Tab clicks switch between Internal/SD card
- [ ] Tab text color updates reactively
- [ ] Grid scrolls vertically without horizontal scroll
- [ ] All spacing matches requirements
- [ ] Cards can be clicked (handler registered)
- [ ] Panel can be navigated to and from

**Code Quality:**
- [ ] XML structure follows existing patterns
- [ ] Global constants used for all repeated values
- [ ] C++ wrapper provides clean API
- [ ] Reactive bindings work correctly
- [ ] Code is documented and maintainable

---

**Next Steps:**
1. Review and discuss each section of requirements
2. Make decisions on all open questions
3. Create implementation plan based on agreed requirements
4. Begin Phase 1 implementation

