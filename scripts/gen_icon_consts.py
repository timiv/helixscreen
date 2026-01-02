#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Generate icon string constants for globals.xml from ui_icon_codepoints.h.

This script parses the C++ icon mapping table and generates XML <str> entries
that can be referenced in LVGL XML files via text="#icon_name" syntax.

Single source of truth: ui_icon_codepoints.h -> globals.xml

Usage:
    python3 scripts/gen_icon_consts.py

The script injects generated content between marker comments in globals.xml:
    <!-- BEGIN GENERATED ICON CONSTS -->
    ...generated content...
    <!-- END GENERATED ICON CONSTS -->
"""

import re
import sys
from pathlib import Path

# Paths relative to project root
PROJECT_ROOT = Path(__file__).parent.parent
CODEPOINTS_H = PROJECT_ROOT / "include" / "ui_icon_codepoints.h"
GLOBALS_XML = PROJECT_ROOT / "ui_xml" / "globals.xml"

BEGIN_MARKER = "<!-- BEGIN GENERATED ICON CONSTS -->"
END_MARKER = "<!-- END GENERATED ICON CONSTS -->"


def parse_codepoints_h(filepath: Path) -> list[tuple[str, bytes]]:
    """Parse ui_icon_codepoints.h and extract icon name -> UTF-8 bytes mappings.

    Returns list of (name, utf8_bytes) tuples.
    """
    content = filepath.read_text()

    # Match lines like: {"arrow_down", "\xF3\xB0\x81\x85"},  // comment
    pattern = r'\{"([^"]+)",\s*"((?:\\x[0-9A-Fa-f]{2})+)"\}'

    icons = []
    for match in re.finditer(pattern, content):
        name = match.group(1)
        hex_string = match.group(2)

        # Convert \xF3\xB0\x81\x85 to actual bytes
        utf8_bytes = bytes.fromhex(hex_string.replace("\\x", ""))
        icons.append((name, utf8_bytes))

    return icons


def generate_xml_entries(icons: list[tuple[str, bytes]], indent: str = "    ") -> str:
    """Generate XML <str> entries for icons.

    Format: <str name="icon_NAME" value="UTF8_CHAR"/>
    """
    lines = []
    for name, utf8_bytes in icons:
        # Decode UTF-8 bytes to get the actual Unicode character
        char = utf8_bytes.decode("utf-8")
        # Escape for XML (though MDI chars don't need escaping)
        lines.append(f'{indent}<str name="icon_{name}" value="{char}"/>')

    return "\n".join(lines)


def inject_into_globals_xml(globals_path: Path, generated_content: str) -> bool:
    """Inject generated content between marker comments in globals.xml.

    Returns True if successful, False if markers not found.
    """
    content = globals_path.read_text()

    # Find marker positions
    begin_idx = content.find(BEGIN_MARKER)
    end_idx = content.find(END_MARKER)

    if begin_idx == -1 or end_idx == -1:
        return False

    # Calculate where content goes (after BEGIN marker, before END marker)
    begin_end = begin_idx + len(BEGIN_MARKER)

    # Build new content
    new_content = (
        content[:begin_end] +
        "\n" + generated_content + "\n" +
        content[end_idx:].lstrip()  # Remove leading whitespace before END marker
    )

    # Ensure proper indentation for END marker
    new_content = new_content.replace("\n" + END_MARKER, "\n    " + END_MARKER)

    globals_path.write_text(new_content)
    return True


def main():
    print(f"Parsing {CODEPOINTS_H}...")

    if not CODEPOINTS_H.exists():
        print(f"Error: {CODEPOINTS_H} not found", file=sys.stderr)
        return 1

    icons = parse_codepoints_h(CODEPOINTS_H)
    print(f"Found {len(icons)} icons")

    if not icons:
        print("Error: No icons found in codepoints file", file=sys.stderr)
        return 1

    # Generate XML entries
    generated = generate_xml_entries(icons)

    if not GLOBALS_XML.exists():
        print(f"Error: {GLOBALS_XML} not found", file=sys.stderr)
        return 1

    # Check if markers exist
    content = GLOBALS_XML.read_text()
    if BEGIN_MARKER not in content or END_MARKER not in content:
        print(f"Error: Marker comments not found in {GLOBALS_XML}", file=sys.stderr)
        print(f"Please add these markers where you want icons injected:")
        print(f"    {BEGIN_MARKER}")
        print(f"    {END_MARKER}")
        return 1

    print(f"Injecting into {GLOBALS_XML}...")
    if inject_into_globals_xml(GLOBALS_XML, generated):
        print(f"✓ Generated {len(icons)} icon constants in globals.xml")
        return 0
    else:
        print("Error: Failed to inject content", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
