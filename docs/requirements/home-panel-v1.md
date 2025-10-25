# UI Requirements - Home Panel

> **Purpose**: Define UI specifications for screenshot review agent
> **Version**: v1.0 (2025-01-11)
> **Panel**: home_panel

## Layout Requirements

### Overall Structure
- [x] Panel width: 100% of content area
- [x] Panel height: 100% of content area
- [x] Background color: `#111410` (panel_bg)
- [x] No padding on outer panel
- [x] No border on outer panel
- [x] Flex direction: column (vertical layout)

### Two-Section Layout
- [x] Top section (printer visualization): Takes 66% of vertical space (flex_grow="2")
- [x] Bottom section (info cards): Takes 33% of vertical space (flex_grow="1")

## Visual Components

### Top Section - Printer Visualization

#### Container
- [x] Width: 100%
- [x] Transparent background (bg_opa="0%")
- [x] No border
- [x] Padding: 20px (padding_normal) on all sides
- [x] Flex direction: column
- [x] Content centered on both axes

#### Printer Image
- [x] Source: `printer_400.png`
- [x] Dimensions: 400x400px (printer_img_size constant)
- [x] Centered horizontally
- [x] Positioned above status text

#### Status Text Label
- [x] Binds to: `status_text` subject
- [x] Font: Montserrat 20px (montserrat_20)
- [x] Color: `#ffffff` (text_primary)
- [x] Margin top: 20px from printer image (padding_normal)
- [x] Centered horizontally

### Bottom Section - Info Cards

#### Container
- [x] Width: 100%
- [x] Transparent background (bg_opa="0%")
- [x] No border
- [x] Padding: 20px (padding_normal) on all sides
- [x] Flex direction: row (horizontal layout)
- [x] Content distributed: space_evenly
- [x] Content centered vertically

### Card 1 - Print Files

#### Structure
- [x] Width: 45% (card_width constant)
- [x] Height: 80% (card_height constant)
- [x] Background: `#202020` (card_bg)
- [x] Border radius: 8px (card_radius)
- [x] No border (border_width="0")
- [x] Padding: 20px (padding_normal)
- [x] Flex direction: column
- [x] Content centered on both axes

#### Content
- [x] Label text: "Print Files"
- [x] Font: Montserrat 16px (montserrat_16)
- [x] Color: `#ffffff` (text_primary)

### Card 2 - Status Card (Temperature / Network / Light)

#### Structure
- [x] Width: 45% (card_width constant)
- [x] Height: 80% (card_height constant)
- [x] Background: `#202020` (card_bg)
- [x] Border radius: 8px (card_radius)
- [x] No border (border_width="0")
- [x] Padding: 20px (padding_normal)
- [x] Flex direction: row (horizontal layout)
- [x] Content distributed: space_evenly
- [x] Content centered vertically

#### Section 1 - Temperature Display
- [x] Container: transparent, no border, no padding
- [x] Flex direction: column (icon above text)
- [x] Both elements centered horizontally and vertically
- [x] Icon: `#icon_temperature` (thermometer-half)
- [x] Icon font: FontAwesome 48px (fa_icons_48)
- [x] Icon color: `#909090` (text_secondary)
- [x] Temperature text: Binds to `temp_text` subject
- [x] Text font: Montserrat 16px (montserrat_16)
- [x] Text color: `#ffffff` (text_primary)
- [x] Text margin top: 8px from icon

#### Divider 1
- [x] Width: 1px
- [x] Height: Stretches full container height (align_self="stretch")
- [x] Background: `#909090` (text_secondary) at 50% opacity
- [x] No border
- [x] Vertical padding: 8px top and bottom

#### Section 2 - Network Status
- [x] Container: transparent, no border, no padding
- [x] Flex direction: column (icon above text)
- [x] Both elements centered horizontally and vertically
- [x] Icon: `#icon_wifi` (wifi symbol)
- [x] Icon font: FontAwesome 48px (fa_icons_48)
- [x] Icon color: `#ff4444` (primary_color) - Active state
- [x] Icon name attribute: "network_icon"
- [x] Network label: "Wi-Fi"
- [x] Label font: Montserrat 16px (montserrat_16)
- [x] Label color: `#909090` (text_secondary)
- [x] Label margin top: 8px from icon
- [x] Label name attribute: "network_label"

#### Divider 2
- [x] Same specs as Divider 1

#### Section 3 - Light Control
- [x] Container: transparent, no border, no padding
- [x] Clickable flag: true
- [x] Flex direction: column (icon above text)
- [x] Content centered horizontally and vertically
- [x] Icon: `#icon_lightbulb` (lightbulb)
- [x] Icon font: FontAwesome 48px (fa_icons_48)
- [x] Icon color: `#909090` (text_secondary) - Inactive state
- [x] Icon name attribute: "light_icon"
- [x] Event: clicked trigger calls `light_toggle_cb` callback

## Colors

### Verified Color Palette
- **Panel Background**: `#111410` (panel_bg)
- **Card Background**: `#202020` (card_bg)
- **Primary Text**: `#ffffff` (text_primary)
- **Secondary Text**: `#909090` (text_secondary)
- **Primary Brand**: `#ff4444` (primary_color) - used for active network icon
- **Accent**: `#00aaff` (accent_color) - not used in home panel

## Spacing & Alignment

### Padding
- Panel outer: 0px
- Section padding: 20px (padding_normal)
- Card padding: 20px (padding_normal)
- Icon to label: 8px margin_top

### Margins
- Status text from printer image: 20px (padding_normal)
- Temperature text from icon: 8px
- Network label from icon: 8px

### Vertical Dividers
- Padding top: 8px
- Padding bottom: 8px

### Flex Alignment
- Top section:
  - Main axis (column): center
  - Cross axis: center
- Bottom section:
  - Main axis (row): space_evenly
  - Cross axis: center
- Status card:
  - Main axis (row): space_evenly
  - Cross axis: center
- Icon/label stacks:
  - Main axis (column): center
  - Cross axis: center

## Typography

### Fonts Used
- **montserrat_20**: Status text (primary display text)
- **montserrat_16**: Card labels, temperature, network label
- **fa_icons_48**: All status card icons (temperature, wifi, lightbulb)

### Text Colors
- **Primary text** (#ffffff): Status text, "Print Files" label, temperature value
- **Secondary text** (#909090): Icon colors (inactive), network label

## Images & Assets

### Printer Image
- [x] Source: `A:/Users/pbrown/code/helixscreen/prototype-ui9/assets/images/printer_400.png`
- [x] Dimensions: 400x400px
- [x] Format: PNG
- [x] Recoloring: None

## Interactive Elements

### Light Control Button
- [x] Clickable container (flag_clickable="true")
- [x] Visual indicator: Icon only, no visible button border
- [x] Default state: Secondary text color (#909090)
- [x] Event binding: clicked → light_toggle_cb
- [x] Expected behavior: Toggle light on/off (color changes handled in C++)

## Data Binding

### Reactive Elements
- [x] **Status Text**: Binds to `status_text` subject (updated from C++)
- [x] **Temperature Display**: Binds to `temp_text` subject (updated from C++)
- [x] **Network Icon/Label**: Named elements for C++ access ("network_icon", "network_label")
- [x] **Light Icon**: Named element for C++ access ("light_icon")

## Flex Growth Distribution

### Vertical Space Allocation
- Top section (printer): flex_grow="2" (66.7% of height)
- Bottom section (cards): flex_grow="1" (33.3% of height)

### Card Sizing
- Each card: 45% width (card_width constant)
- Each card: 80% height (card_height constant)
- Space between cards: Automatically distributed by space_evenly

## Known Expectations

### Static Elements
- [x] "Print Files" card is placeholder (no interaction yet)
- [x] Printer image is static (not dynamically updated)

### Dynamic Elements
- [x] Status text updates via subject binding
- [x] Temperature text updates via subject binding
- [x] Network icon/label can be updated from C++ (by name lookup)
- [x] Light icon can be updated from C++ (by name lookup)

## Critical Visual Checks

### Alignment
- [x] Printer image perfectly centered in top section
- [x] Status text perfectly centered below printer image
- [x] Two cards evenly spaced in bottom section
- [x] Icons centered above labels in status card
- [x] Vertical dividers properly stretch full height

### Sizing
- [x] Printer image is 400x400px (dominant visual element)
- [x] Icons are 48px (FontAwesome size)
- [x] Cards are same width (45% each)
- [x] Cards are same height (80% of section)

### Colors
- [x] Background is very dark green-tinted black (#111410)
- [x] Cards are dark gray (#202020) - clearly visible against background
- [x] White text (#ffffff) has high contrast
- [x] Gray text (#909090) is clearly legible but subdued
- [x] Active network icon is red (#ff4444)
- [x] Inactive light icon is gray (#909090)

### Spacing
- [x] No cramping - generous 20px padding
- [x] Icons have breathing room (8px margin) from labels
- [x] Dividers have vertical padding (8px) preventing edge contact
- [x] Content doesn't touch card edges

---

## Notes

This panel serves as the default "idle" screen showing printer status at a glance. Most elements are currently placeholders for future functionality:

- Print Files card will eventually navigate to file browser
- Temperature will show actual nozzle/bed temps
- Network will show connection status
- Light button will control chamber/LED lights

The design prioritizes:
1. **Clear visual hierarchy**: Printer image dominates, cards provide quick status
2. **Generous spacing**: Touch-friendly with room for finger taps
3. **Subdued colors**: Dark theme reduces eye strain during long prints
4. **Reactive updates**: Data binding allows C++ to update display without manual widget management
