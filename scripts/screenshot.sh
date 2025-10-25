#!/bin/bash
# UI Prototype Screenshot Helper
# Builds, runs, captures, and compresses screenshot in one command
# Usage: ./screenshot.sh [binary_name] [output_name] [panel_name] [additional args...]
#   binary_name: Name of binary to run (default: helix-ui-proto)
#   output_name: Name for output file (default: timestamp)
#   panel_name:  Panel to display (default: home)
#   Additional args: Forwarded to binary (e.g., -s small, --size large)
#                Options: home, controls, filament, settings, advanced, print-select

set -e

# Get binary name (first arg) or default
BINARY="${1:-helix-ui-proto}"
BINARY_PATH="./build/bin/${BINARY}"

# Get unique name or use timestamp
NAME="${2:-$(date +%s)}"
BMP_FILE="/tmp/ui-screenshot-${NAME}.bmp"
PNG_FILE="/tmp/ui-screenshot-${NAME}.png"

# Get panel name (third arg) or default to empty (uses app default)
PANEL="${3:-}"

# Get any additional arguments (everything after first 3 args)
shift 3 2>/dev/null || true
EXTRA_ARGS="$@"

# Build
echo "Building prototype..."
make -j$(sysctl -n hw.ncpu) 2>&1 | grep -E "(Compiling|Linking|Build complete|error:)" || true

# Clean old screenshots
rm -f /tmp/ui-screenshot.bmp

# Run and auto-capture (app saves with timestamp)
if [ -n "$PANEL" ]; then
    echo "Running ${BINARY} with panel: ${PANEL} (3 second timeout)..."
    gtimeout 3 "${BINARY_PATH}" $EXTRA_ARGS -p "${PANEL}" 2>&1 | grep -E "(LVGL initialized|Screenshot saved|Switched to panel|Initial Panel|Error|error)" || true
else
    echo "Running ${BINARY} (3 second timeout)..."
    gtimeout 3 "${BINARY_PATH}" $EXTRA_ARGS 2>&1 | grep -E "(LVGL initialized|Screenshot saved|Switched to panel|Error|error)" || true
fi

# Find the most recent screenshot (app now saves with timestamps)
LATEST_BMP=$(ls -t /tmp/ui-screenshot-*.bmp 2>/dev/null | head -1)
if [ -n "$LATEST_BMP" ]; then
    # Rename to requested name
    if [ "$LATEST_BMP" != "$BMP_FILE" ]; then
        mv "$LATEST_BMP" "$BMP_FILE"
    fi
else
    echo "ERROR: Screenshot not captured"
    exit 1
fi

# Convert to PNG
echo "Converting to PNG..."
magick "$BMP_FILE" "$PNG_FILE"

# Show result
SIZE=$(ls -lh "$PNG_FILE" | awk '{print $5}')
echo "Screenshot ready: $PNG_FILE ($SIZE)"
echo "Path: $PNG_FILE"
