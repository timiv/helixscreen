#!/usr/bin/env python3

# Copyright (C) 2025-2026 356C LLC
# SPDX-License-Identifier: GPL-3.0-or-later

"""
Generate pre-rendered diagonal gradient background images for print file cards.

Creates LVGL-native .bin files that can be loaded directly without runtime
gradient calculation, significantly improving scroll performance on embedded devices.

Supports dark and light theme variants:
  - Dark mode: gray 123 -> 43 (bright top-right to dark bottom-left)
  - Light mode: gray 200 -> 235 (subtle light gradient for light theme backgrounds)

Usage:
    python scripts/generate_gradient_bg.py [--output-dir assets/images] [--mode both]
    python scripts/generate_gradient_bg.py --mode dark   # Dark variants only
    python scripts/generate_gradient_bg.py --mode light  # Light variants only
    python scripts/generate_gradient_bg.py --mode both   # Both (default)
"""

import argparse
import os
import struct
from pathlib import Path

# Dark mode gradient colors (matching ui_gradient_canvas.cpp defaults)
# Diagonal gradient: bright at top-right, dark at bottom-left
DARK_START_GRAY = 123  # Top-right - brighter
DARK_END_GRAY = 43     # Bottom-left - darker

# Light mode gradient colors (subtle gradient for light theme backgrounds)
# Lighter values create a gentle gradient that works on light card backgrounds
LIGHT_START_GRAY = 235  # Top-right - brightest
LIGHT_END_GRAY = 200    # Bottom-left - slightly darker

# 4x4 Bayer dither matrix (normalized to 0-15)
BAYER_4X4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5]
]

# Gradient sizes for different UI elements
# Format: (name, width, height)
GRADIENT_SIZES = [
    # Print file cards (small grid items)
    ("card-small", 140, 200),   # Small screens (480x320)
    ("card-medium", 170, 245),  # Medium screens (800x480, AD5M)
    ("card-large", 230, 280),   # Large screens (1024x600+)
    # Detail panels (print status, file detail, history detail)
    # These are larger thumbnail/preview areas in overlay panels
    ("panel-medium", 380, 400),  # Medium screens - detail panel thumbnail area
    ("panel-large", 480, 500),   # Large screens - detail panel thumbnail area
]


def bayer_threshold(x: int, y: int) -> int:
    """Apply Bayer dithering threshold tuned for RGB565."""
    bayer_val = BAYER_4X4[y & 3][x & 3]
    return (bayer_val * 24 // 16) - 12  # Scale to ±12 range


def clamp(val: int, min_val: int, max_val: int) -> int:
    return max(min_val, min(max_val, val))


def generate_gradient(width: int, height: int, start_gray: int, end_gray: int,
                      dither: bool = True) -> bytes:
    """
    Generate diagonal gradient pixel data in ARGB8888 format.

    Args:
        width: Image width in pixels
        height: Image height in pixels
        start_gray: Gray value at top-right (brighter end)
        end_gray: Gray value at bottom-left (darker end)
        dither: Enable Bayer dithering to reduce banding

    Returns bytes in LVGL draw buffer format (ARGB8888, row-major).
    """
    pixels = bytearray()

    # For diagonal gradient (top-right to bottom-left), max distance is width + height - 2
    max_dist = float(width + height - 2) if (width + height > 2) else 1.0

    for y in range(height):
        for x in range(width):
            # Diagonal interpolation: top-right (bright) to bottom-left (dark)
            # Distance from top-right corner: (width-1-x) + y
            t = float((width - 1 - x) + y) / max_dist

            # Interpolate gray value
            gray = int(start_gray + t * (end_gray - start_gray))

            if dither:
                threshold = bayer_threshold(x, y)
                gray = clamp(gray + threshold, 0, 255)

            # ARGB8888: Blue, Green, Red, Alpha (little-endian BGRA in memory)
            pixels.extend([gray, gray, gray, 255])

    return bytes(pixels)


def write_lvgl_bin(output_path: Path, width: int, height: int, pixel_data: bytes):
    """
    Write LVGL 9.x native binary image format.
    
    Header format (12 bytes):
        - magic: 0x19 (1 byte) - LVGL 9 signature
        - cf: color format (1 byte) - 0x10 = ARGB8888
        - flags: image flags (2 bytes)
        - w: width (2 bytes)
        - h: height (2 bytes)
        - stride: row stride in bytes (2 bytes)
        - reserved: (2 bytes)
    """
    # LVGL 9.x header (matching LVGLImage.py format)
    magic = 0x19  # LVGL 9 signature
    cf = 0x10     # LV_COLOR_FORMAT_ARGB8888 (from ColorFormat enum)
    flags = 0x00  # No special flags
    stride = width * 4  # 4 bytes per pixel (ARGB8888)
    reserved = 0
    
    header = struct.pack('<BBHHHHH',
                         magic,
                         cf,
                         flags,
                         width,
                         height,
                         stride,
                         reserved)
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(pixel_data)
    
    print(f"  Generated: {output_path} ({width}x{height}, {len(pixel_data) + len(header)} bytes)")


def main():
    parser = argparse.ArgumentParser(
        description="Generate pre-rendered gradient backgrounds for print cards"
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("assets/images"),
        help="Output directory for .bin files (default: assets/images)"
    )
    parser.add_argument(
        "--no-dither",
        action="store_true",
        help="Disable Bayer dithering"
    )
    parser.add_argument(
        "--mode",
        choices=["dark", "light", "both"],
        default="both",
        help="Which theme variants to generate (default: both)"
    )
    args = parser.parse_args()

    dither = not args.no_dither
    count = 0

    # Mode configurations: (suffix, start_gray, end_gray)
    modes = []
    if args.mode in ("dark", "both"):
        modes.append(("dark", DARK_START_GRAY, DARK_END_GRAY))
    if args.mode in ("light", "both"):
        modes.append(("light", LIGHT_START_GRAY, LIGHT_END_GRAY))

    print(f"Generating gradient backgrounds (mode={args.mode})...")

    for mode_suffix, start_gray, end_gray in modes:
        print(f"\n  [{mode_suffix}] gray {start_gray} -> {end_gray}")
        for name, width, height in GRADIENT_SIZES:
            pixel_data = generate_gradient(width, height, start_gray, end_gray,
                                           dither=dither)
            output_path = args.output_dir / f"gradient-{name}-{mode_suffix}.bin"
            write_lvgl_bin(output_path, width, height, pixel_data)
            count += 1

    print(f"\nDone! Generated {count} gradient images in {args.output_dir}")


if __name__ == "__main__":
    main()
