# Adding Material Design Icons to HelixScreen

This guide documents the complete process for adding new Material Design icons to the HelixScreen prototype UI.

## Prerequisites

- Python 3 with virtual environment support
- ImageMagick (`magick` command)
- curl for downloading SVGs
- Material Icons source: https://fonts.google.com/icons

### First-Time Setup

Create a Python virtual environment and install required packages:

```bash
# From project root directory
python3 -m venv .venv
.venv/bin/pip install pypng lz4
```

**Note:** The LVGLImage.py script requires both `pypng` and `lz4` packages. The venv only needs to be created once.

## Step-by-Step Process

### 1. Find and Download the Icon

Browse Material Icons at https://fonts.google.com/icons and find your desired icon.

Download the SVG directly using curl:
```bash
# Navigate to the material icons directory
cd assets/images/material/

# Download the icon (example: list icon)
curl -o list.svg 'https://fonts.gstatic.com/s/i/materialicons/list/v1/24px.svg'
```

### 2. Convert SVG to PNG

Material icons come as 24x24 SVGs. We need to convert them to 64x64 PNGs with transparency:

```bash
# Convert SVG to PNG with ImageMagick
magick -background none list.svg -resize 64x64 list.png
```

### 3. Convert PNG to LVGL C Array

Use the LVGLImage.py script with the RGB565A8 format (RGB with alpha channel):

```bash
# From project root directory
.venv/bin/python3 scripts/LVGLImage.py assets/images/material/list.png \
    --ofmt C --cf RGB565A8 -o assets/images/material/list.c
```

This creates a C file with the image data in LVGL format.

### 4. Register the Icon in Code

#### Add declaration to header file

Edit `include/material_icons.h` and add the declaration in the appropriate category section:

```cpp
// Print & Files (example category)
LV_IMG_DECLARE(print);
LV_IMG_DECLARE(list);
LV_IMG_DECLARE(grid_view);  // Add your new icon here
```

#### Register in material_icons.cpp

Edit `src/material_icons.cpp` and add the registration in the `material_icons_register()` function:

```cpp
// Print & Files
lv_xml_register_image(NULL, "mat_print", &print);
lv_xml_register_image(NULL, "mat_list", &list);
lv_xml_register_image(NULL, "mat_grid_view", &grid_view);  // Add your registration here
```

#### Update the icon count in the log message:
```cpp
LV_LOG_USER("Registering Material Design icons (58 total)...");  // Increment count
```

### 5. Build and Test

```bash
# Clean build to ensure new icon is compiled
make clean && make

# Test with a panel that uses the icon
./build/bin/helix-ui-proto print_select
```

### 6. Use the Icon in XML

In your XML files, reference the icon using the registered name:

```xml
<!-- Using with icon component -->
<icon src="mat_list" size="md" variant="primary"/>

<!-- Or with lv_image directly -->
<lv_image src="mat_list" width="32" height="32"/>
```

## Icon Naming Convention

- All Material Design icons are prefixed with `mat_`
- Use lowercase with underscores for multi-word names
- Examples: `mat_home`, `mat_arrow_up`, `mat_list`

## Batch Conversion

For adding multiple icons at once, you can use the existing script:
```bash
scripts/convert-material-icons-lvgl9.sh
```

Note: This script expects SVGs in a specific directory structure and uses Inkscape for conversion.

## Common Icon Sizes

- Small panels (3col): 12x12 or 14x14 display size (use `size="xs"` with icon component)
- Medium panels (4col): 14x14 or 16x16 display size (use `size="xs"` or `size="sm"`)
- Large panels (5col): 16x16 or 24x24 display size (use `size="sm"` or `size="md"`)

## Troubleshooting

### Icon not found warning
- Ensure the icon is registered with exact name match
- Check that the .c file was generated and is in `assets/images/material/`
- Verify the extern declaration and registration in `material_icons.cpp`

### Icon appears corrupted
- Ensure SVG has proper transparency (use `-background none` in magick)
- Use RGB565A8 format for proper alpha channel support
- Check that the original SVG is valid

### Build errors
- Make sure to run `make clean` after adding new icons
- Check that the icon filename matches the variable name in the .c file

## Example: Adding "grid_view" Icon

```bash
# 1. Download
cd assets/images/material/
curl -o grid_view.svg 'https://fonts.gstatic.com/s/i/materialicons/grid_view/v1/24px.svg'

# 2. Convert to PNG
magick -background none grid_view.svg -resize 64x64 grid_view.png

# 3. Convert to C
cd ../../..  # Back to project root
.venv/bin/python3 scripts/LVGLImage.py assets/images/material/grid_view.png \
    --ofmt C --cf RGB565A8 -o assets/images/material/grid_view.c

# 4. Register the icon
# In include/material_icons.h, add to Print & Files section:
#   LV_IMG_DECLARE(grid_view);
#
# In src/material_icons.cpp, add to Print & Files section:
#   lv_xml_register_image(NULL, "mat_grid_view", &grid_view);
#
# Update count: LV_LOG_USER("Registering Material Design icons (58 total)...");

# 5. Build and test
make clean && make
```

## Notes

- The Makefile automatically compiles all .c files in `assets/images/material/`
- Icons are loaded into XML registry at startup via `material_icons_register()`
- The `icon` component provides automatic sizing and recoloring
- All icons are converted to 64x64 base size and scaled at runtime