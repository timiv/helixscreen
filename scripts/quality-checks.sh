#!/bin/bash
# SPDX-FileCopyrightText: 2024 Patrick Brown <opensource@pbdigital.org>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Quality checks script - single source of truth for pre-commit and CI
# Usage:
#   ./scripts/quality-checks.sh           # Check all files (for CI)
#   ./scripts/quality-checks.sh --staged-only  # Check only staged files (for pre-commit)

set -e

# Parse arguments
STAGED_ONLY=false
if [ "$1" = "--staged-only" ]; then
  STAGED_ONLY=true
fi

# Change to repo root
REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

EXIT_CODE=0

echo "🔍 Running quality checks..."
if [ "$STAGED_ONLY" = true ]; then
  echo "   Mode: Staged files only (pre-commit)"
else
  echo "   Mode: All files (CI)"
fi
echo ""

# ====================================================================
# Copyright Header Check
# ====================================================================
echo "📝 Checking copyright headers..."

if [ "$STAGED_ONLY" = true ]; then
  # Pre-commit mode: check only staged files
  FILES=$(git diff --cached --name-only --diff-filter=ACM | \
    grep -E '\.(cpp|c|h|mm)$' | \
    grep -v '^libhv/' | \
    grep -v '^lvgl/' | \
    grep -v '^spdlog/' | \
    grep -v '^wpa_supplicant/' | \
    grep -v '^node_modules/' | \
    grep -v '^build/' | \
    grep -v '/\.' || true)
else
  # CI mode: check all files in src/ and include/
  FILES=$(find src include -name "*.cpp" -o -name "*.c" -o -name "*.h" -o -name "*.mm" 2>/dev/null | \
    grep -v '/\.' || true)
fi

if [ -n "$FILES" ]; then
  MISSING_HEADERS=""
  for file in $FILES; do
    if [ -f "$file" ]; then
      if ! head -3 "$file" | grep -q "SPDX-License-Identifier: GPL-3.0-or-later"; then
        echo "❌ Missing GPL v3 header: $file"
        MISSING_HEADERS="$MISSING_HEADERS $file"
        EXIT_CODE=1
      fi
    fi
  done

  if [ -n "$MISSING_HEADERS" ]; then
    echo ""
    echo "See docs/COPYRIGHT_HEADERS.md for the required header format"
  else
    echo "✅ All source files have proper copyright headers"
  fi
else
  if [ "$STAGED_ONLY" = true ]; then
    echo "ℹ️  No source files staged for commit"
  else
    echo "ℹ️  No source files found"
  fi
fi

echo ""

# ====================================================================
# Code Style Check
# ====================================================================
echo "🎨 Checking for common issues..."

# Check for TODO/FIXME/XXX comments (informational only)
if [ -n "$FILES" ]; then
  if echo "$FILES" | xargs grep -n "TODO\|FIXME\|XXX" 2>/dev/null | head -20; then
    echo "ℹ️  Found TODO/FIXME markers (informational only)"
  else
    echo "✅ No TODO/FIXME markers found"
  fi
else
  echo "ℹ️  No source files to check"
fi

echo ""

# ====================================================================
# Final Result
# ====================================================================
if [ $EXIT_CODE -eq 0 ]; then
  echo "✅ Quality checks passed!"
  exit 0
else
  echo "❌ Quality checks failed!"
  exit 1
fi
