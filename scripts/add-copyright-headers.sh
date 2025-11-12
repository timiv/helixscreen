#!/bin/bash
# SPDX-FileCopyrightText: 2024 Patrick Brown <opensource@pbdigital.org>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Add SPDX-style copyright headers to source files missing them

set -e

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

# Find all source files in src/ and include/
FILES=$(find src include -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.mm" 2>/dev/null | grep -v '/\.' || true)

if [ -z "$FILES" ]; then
  echo "ℹ️  No source files found"
  exit 0
fi

UPDATED_COUNT=0
SKIPPED_COUNT=0

echo "📝 Adding copyright headers to files missing them..."
echo ""

for file in $FILES; do
  if [ ! -f "$file" ]; then
    continue
  fi

  # Check if file already has SPDX identifier
  if head -3 "$file" | grep -q "SPDX-License-Identifier: GPL-3.0-or-later"; then
    SKIPPED_COUNT=$((SKIPPED_COUNT + 1))
    continue
  fi

  # Determine comment style based on file extension
  if [[ "$file" == *.c ]]; then
    # C files might need /* */ style, but let's use // for consistency
    HEADER="// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

"
  else
    # C++/Objective-C++ use // style
    HEADER="// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

"
  fi

  # Create temp file with header + original content
  TEMP_FILE="${file}.tmp"
  echo -n "$HEADER" > "$TEMP_FILE"
  cat "$file" >> "$TEMP_FILE"

  # Replace original file
  mv "$TEMP_FILE" "$file"

  echo "✅ Added header to: $file"
  UPDATED_COUNT=$((UPDATED_COUNT + 1))
done

echo ""
echo "📊 Summary:"
echo "   Updated: $UPDATED_COUNT files"
echo "   Skipped: $SKIPPED_COUNT files (already have SPDX header)"
echo ""

if [ $UPDATED_COUNT -gt 0 ]; then
  echo "✅ Copyright headers added successfully!"
else
  echo "ℹ️  No files needed updates"
fi
