#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# HelixScreen Codebase Audit Script
# Checks for violations of coding standards and best practices.
# Run periodically or in CI to catch regressions.
#
# Usage:
#   ./scripts/audit_codebase.sh [--strict] [--files FILE...]
#
# Modes:
#   Full audit (no files):  Scans entire codebase with threshold checks
#   File mode (--files):    Checks only specified files (for pre-commit hooks)
#
# Exit codes:
#   0 = All checks passed (or only warnings)
#   1 = Critical violations found (with --strict, any warnings also fail)

set -euo pipefail

# Colors for output
RED='\033[0;31m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Parse arguments
STRICT_MODE=false
FILE_MODE=false
FILES=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --strict)
            STRICT_MODE=true
            shift
            ;;
        --files)
            FILE_MODE=true
            shift
            # Collect all remaining arguments as files
            while [[ $# -gt 0 && ! "$1" =~ ^-- ]]; do
                FILES+=("$1")
                shift
            done
            ;;
        *)
            # Treat as file if not a flag
            if [[ -f "$1" ]]; then
                FILE_MODE=true
                FILES+=("$1")
            fi
            shift
            ;;
    esac
done

# Counters
ERRORS=0
WARNINGS=0

# Helper functions
error() {
    echo -e "${RED}ERROR:${NC} $1"
    ((ERRORS++)) || true
}

warning() {
    echo -e "${YELLOW}WARNING:${NC} $1"
    ((WARNINGS++)) || true
}

info() {
    echo -e "${CYAN}INFO:${NC} $1"
}

success() {
    echo -e "${GREEN}✓${NC} $1"
}

section() {
    echo ""
    echo -e "${CYAN}=== $1 ===${NC}"
}

# Change to repo root
cd "$(git rev-parse --show-toplevel 2>/dev/null || pwd)"

# Filter files to only include relevant types
filter_cpp_files() {
    local result=()
    for f in "${FILES[@]}"; do
        if [[ "$f" == *.cpp && -f "$f" ]]; then
            result+=("$f")
        fi
    done
    echo "${result[@]:-}"
}

filter_xml_files() {
    local result=()
    for f in "${FILES[@]}"; do
        if [[ "$f" == *.xml && -f "$f" ]]; then
            result+=("$f")
        fi
    done
    echo "${result[@]:-}"
}

filter_ui_cpp_files() {
    local result=()
    for f in "${FILES[@]}"; do
        if [[ "$f" == *ui_*.cpp && -f "$f" ]]; then
            result+=("$f")
        fi
    done
    echo "${result[@]:-}"
}

# ============================================================================
# FILE MODE: Check only specified files (for pre-commit)
# ============================================================================
if [ "$FILE_MODE" = true ]; then
    echo "========================================"
    echo "HelixScreen Audit (File Mode)"
    echo "Checking ${#FILES[@]} file(s)"
    echo "========================================"

    # Get filtered file lists
    cpp_files=($(filter_cpp_files))
    ui_cpp_files=($(filter_ui_cpp_files))
    xml_files=($(filter_xml_files))

    if [ ${#cpp_files[@]} -eq 0 ] && [ ${#xml_files[@]} -eq 0 ]; then
        echo "No auditable files (.cpp, .xml) in changeset"
        exit 0
    fi

    #
    # === P1: Timer Safety (per-file) ===
    #
    if [ ${#cpp_files[@]} -gt 0 ]; then
        section "P1: Timer Safety"
        for f in "${cpp_files[@]}"; do
            set +e
            creates=$(grep -c "lv_timer_create" "$f" 2>/dev/null || echo "0")
            deletes=$(grep -c "lv_timer_del" "$f" 2>/dev/null || echo "0")
            set -e
            creates=$(echo "$creates" | tr -d '[:space:]')
            deletes=$(echo "$deletes" | tr -d '[:space:]')
            if [ "$creates" -gt 0 ] && [ "$creates" -gt "$deletes" ]; then
                warning "$(basename "$f"): creates $creates timers but only deletes $deletes"
            fi
        done
        success "Timer check complete"
    fi

    #
    # === P2b: Memory Safety Anti-Patterns (per-file, CRITICAL) ===
    #
    if [ ${#ui_cpp_files[@]} -gt 0 ]; then
        section "P2b: Memory Safety Anti-Patterns"

        for f in "${ui_cpp_files[@]}"; do
            fname=$(basename "$f")

            # Check for vector element pointer storage
            set +e
            vector_issues=$(grep -n '&[a-z_]*_\.\(back\|front\)()' "$f" 2>/dev/null)
            set -e
            if [ -n "$vector_issues" ]; then
                error "$fname: stores pointer to vector element (dangling pointer risk)"
                echo "$vector_issues" | head -3
            fi

            # Check for user_data allocation without DELETE handler
            set +e
            has_new_userdata=$(grep -l 'set_user_data.*new\|new.*set_user_data' "$f" 2>/dev/null)
            set -e
            if [ -n "$has_new_userdata" ]; then
                set +e
                has_delete_handler=$(grep -l 'LV_EVENT_DELETE' "$f" 2>/dev/null)
                set -e
                if [ -z "$has_delete_handler" ]; then
                    error "$fname: allocates user_data with 'new' but has no LV_EVENT_DELETE handler"
                fi
            fi
        done

        if [ "$ERRORS" -eq 0 ]; then
            success "No memory safety anti-patterns found"
        fi
    fi

    #
    # === P3: Design Tokens (per-file) ===
    #
    if [ ${#xml_files[@]} -gt 0 ]; then
        section "P3: Design Token Compliance"

        for f in "${xml_files[@]}"; do
            fname=$(basename "$f")
            set +e
            hardcoded=$(grep -n 'style_pad[^=]*="[1-9]\|style_margin[^=]*="[1-9]\|style_gap[^=]*="[1-9]' "$f" 2>/dev/null)
            set -e
            if [ -n "$hardcoded" ]; then
                count=$(echo "$hardcoded" | wc -l | tr -d ' ')
                warning "$fname: $count hardcoded spacing value(s) (should use design tokens)"
            fi
        done

        if [ "$WARNINGS" -eq 0 ]; then
            success "All spacing uses design tokens"
        fi
    fi

    #
    # === C++ Hardcoded Colors (per-file) ===
    #
    if [ ${#ui_cpp_files[@]} -gt 0 ]; then
        section "C++ Hardcoded Colors"

        for f in "${ui_cpp_files[@]}"; do
            fname=$(basename "$f")
            set +e
            color_issues=$(grep -n 'lv_color_hex\|lv_color_make' "$f" 2>/dev/null | grep -v 'theme\|parse')
            set -e
            if [ -n "$color_issues" ]; then
                count=$(echo "$color_issues" | wc -l | tr -d ' ')
                warning "$fname: $count hardcoded color literal(s) (should use theme API)"
            fi
        done
    fi

    #
    # === Summary ===
    #
    section "Summary"
    echo ""
    echo "Files checked: ${#FILES[@]}"
    echo "Errors:   $ERRORS"
    echo "Warnings: $WARNINGS"
    echo ""

    if [ "$ERRORS" -gt 0 ]; then
        echo -e "${RED}AUDIT FAILED${NC} - $ERRORS critical error(s) found"
        exit 1
    elif [ "$WARNINGS" -gt 0 ] && [ "$STRICT_MODE" = true ]; then
        echo -e "${YELLOW}AUDIT FAILED (strict mode)${NC} - $WARNINGS warning(s) found"
        exit 1
    elif [ "$WARNINGS" -gt 0 ]; then
        echo -e "${YELLOW}AUDIT PASSED WITH WARNINGS${NC} - $WARNINGS warning(s) found"
        exit 0
    else
        echo -e "${GREEN}AUDIT PASSED${NC} - No issues found"
        exit 0
    fi
fi

# ============================================================================
# FULL MODE: Scan entire codebase (default)
# ============================================================================

echo "========================================"
echo "HelixScreen Codebase Audit"
echo "Date: $(date)"
echo "========================================"

#
# === P1: Memory Safety (Critical) ===
#
section "P1: Memory Safety (Critical)"

# Timer leak detection
set +e
timer_creates=$(grep -rn 'lv_timer_create' src/ --include='*.cpp' 2>/dev/null | wc -l | tr -d ' ')
timer_deletes=$(grep -rn 'lv_timer_del' src/ --include='*.cpp' 2>/dev/null | wc -l | tr -d ' ')
set -e
echo "Timer creates: $timer_creates"
echo "Timer deletes: $timer_deletes"

# Check for files with more creates than deletes (potential leaks)
echo ""
echo "Checking for unbalanced timer usage:"
unbalanced=0
for f in src/*.cpp; do
    [ -f "$f" ] || continue
    creates=$(grep -c "lv_timer_create" "$f" 2>/dev/null || true)
    creates=$(echo "$creates" | tr -d '[:space:]')
    creates=${creates:-0}
    deletes=$(grep -c "lv_timer_del" "$f" 2>/dev/null || true)
    deletes=$(echo "$deletes" | tr -d '[:space:]')
    deletes=${deletes:-0}
    if [ "$creates" -gt 0 ] 2>/dev/null && [ "$creates" -gt "$deletes" ] 2>/dev/null; then
        echo "  REVIEW: $f (creates: $creates, deletes: $deletes)"
        ((unbalanced++)) || true
    fi
done
if [ "$unbalanced" -eq 0 ]; then
    success "All timer usage appears balanced"
fi

#
# === P2: RAII Compliance ===
#
section "P2: RAII Compliance"

# Manual new/delete in UI files
# Note: Temporarily disable errexit for grep commands (grep returns 1 when no matches)
set +e
manual_new=$(grep -rn '\bnew \w\+(' src/ui_*.cpp 2>/dev/null | grep -v 'make_unique\|placement' | wc -l | tr -d ' ')
manual_delete=$(grep -rn '^\s*delete ' src/ --include='*.cpp' 2>/dev/null | grep -v lib/ | wc -l | tr -d ' ')
lv_malloc_count=$(grep -rn 'lv_malloc' src/ --include='*.cpp' 2>/dev/null | grep -v lib/ | wc -l | tr -d ' ')
set -e

echo "Manual 'new' in UI code: $manual_new"
echo "Manual 'delete': $manual_delete"
echo "lv_malloc in src/: $lv_malloc_count"

# Thresholds (adjust based on migration progress)
MANUAL_NEW_THRESHOLD=20
MANUAL_DELETE_THRESHOLD=35
LV_MALLOC_THRESHOLD=5

if [ "$manual_new" -gt "$MANUAL_NEW_THRESHOLD" ]; then
    warning "Manual 'new' count ($manual_new) exceeds threshold ($MANUAL_NEW_THRESHOLD)"
fi
if [ "$manual_delete" -gt "$MANUAL_DELETE_THRESHOLD" ]; then
    warning "Manual 'delete' count ($manual_delete) exceeds threshold ($MANUAL_DELETE_THRESHOLD)"
fi
if [ "$lv_malloc_count" -gt "$LV_MALLOC_THRESHOLD" ]; then
    warning "lv_malloc count ($lv_malloc_count) exceeds threshold ($LV_MALLOC_THRESHOLD)"
fi

#
# === P2b: Memory Safety Anti-Patterns (Critical) ===
#
section "P2b: Memory Safety Anti-Patterns"

# Check for dangerous vector element pointer storage
# Pattern: &vec.back(), &vec[i], &vec_.back() stored in user_data
echo "Checking for vector element pointer storage (dangling pointer risk):"
set +e
vector_ptr_issues=$(grep -rn '&[a-z_]*_\.\(back\|front\)()' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
set -e

if [ "$vector_ptr_issues" -gt 0 ]; then
    error "Found $vector_ptr_issues instances of &vec.back()/.front() - dangling pointer risk!"
    set +e
    grep -rn '&[a-z_]*_\.\(back\|front\)()' src/ui_*.cpp 2>/dev/null | head -5
    set -e
else
    success "No vector element pointer storage found"
fi

# Check for user_data allocations without DELETE handlers
# Files that have 'new' + 'set_user_data' should also have 'LV_EVENT_DELETE'
echo ""
echo "Checking for user_data allocations without DELETE handlers:"
userdata_leak_risk=0
for f in src/ui_*.cpp; do
    [ -f "$f" ] || continue
    set +e
    has_new_userdata=$(grep -l 'set_user_data.*new\|new.*set_user_data' "$f" 2>/dev/null)
    if [ -n "$has_new_userdata" ]; then
        # Check if this file registers a DELETE handler
        has_delete_handler=$(grep -l 'LV_EVENT_DELETE' "$f" 2>/dev/null)
        if [ -z "$has_delete_handler" ]; then
            error "$(basename "$f"): allocates user_data but has no LV_EVENT_DELETE handler!"
            ((userdata_leak_risk++)) || true
        fi
    fi
    set -e
done
if [ "$userdata_leak_risk" -eq 0 ]; then
    success "All user_data allocations have DELETE handlers"
fi

#
# === P3: Design Tokens ===
#
section "P3: XML Design Token Compliance"

set +e
hardcoded_padding=$(grep -rn 'style_pad[^=]*="[1-9]' ui_xml/ --include='*.xml' 2>/dev/null | wc -l | tr -d ' ')
hardcoded_margin=$(grep -rn 'style_margin[^=]*="[1-9]' ui_xml/ --include='*.xml' 2>/dev/null | wc -l | tr -d ' ')
hardcoded_gap=$(grep -rn 'style_gap[^=]*="[1-9]' ui_xml/ --include='*.xml' 2>/dev/null | wc -l | tr -d ' ')
set -e

echo "Hardcoded padding values: $hardcoded_padding"
echo "Hardcoded margin values: $hardcoded_margin"
echo "Hardcoded gap values: $hardcoded_gap"

HARDCODED_SPACING_THRESHOLD=30
total_hardcoded=$((hardcoded_padding + hardcoded_margin + hardcoded_gap))
if [ "$total_hardcoded" -gt "$HARDCODED_SPACING_THRESHOLD" ]; then
    warning "Hardcoded spacing values ($total_hardcoded) exceed threshold ($HARDCODED_SPACING_THRESHOLD)"
fi

#
# === P4: Declarative UI Compliance ===
#
section "P4: Declarative UI Compliance"

set +e
# Event handlers - categorize by type
event_delete=$(grep -r 'lv_obj_add_event_cb.*DELETE' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
event_gesture=$(grep -rE 'lv_obj_add_event_cb.*(GESTURE|SCROLL|DRAW|PRESS)' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
event_clicked=$(grep -rE 'lv_obj_add_event_cb.*(CLICKED|VALUE_CHANGED)' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
event_total=$(grep -r 'lv_obj_add_event_cb' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')

# Text updates
text_updates=$(grep -rn 'lv_label_set_text' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')

# Visibility - categorize by pattern
visibility_pool=$(grep -rE 'lv_obj_(add|clear)_flag.*(pool|Pool).*HIDDEN' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
visibility_total=$(grep -rn 'lv_obj_add_flag.*HIDDEN\|lv_obj_clear_flag.*HIDDEN' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
visibility_actionable=$((visibility_total - visibility_pool))

# Inline styles
inline_styles=$(grep -rn 'lv_obj_set_style_' src/ui_*.cpp 2>/dev/null | wc -l | tr -d ' ')
set -e

echo ""
echo "Event Handlers:"
echo "  DELETE (legitimate):     $event_delete"
echo "  Gesture/Draw (legit):    $event_gesture"
echo "  CLICKED/VALUE_CHANGED:   $event_clicked  ← should use XML event_cb"
echo "  Total:                   $event_total"
echo ""
echo "Visibility Toggles:"
echo "  Widget pool (legit):     $visibility_pool"
echo "  Actionable:              $visibility_actionable  ← consider bind_flag"
echo "  Total:                   $visibility_total"
echo ""
echo "Text Updates:              $text_updates  ← consider bind_text subjects"
echo "Inline Style Setters:      $inline_styles  (many are dynamic/legitimate)"

# Thresholds for actionable items only
EVENT_CLICKED_THRESHOLD=100
VISIBILITY_THRESHOLD=120
TEXT_UPDATE_THRESHOLD=200
INLINE_STYLE_THRESHOLD=600

if [ "$event_clicked" -gt "$EVENT_CLICKED_THRESHOLD" ]; then
    warning "CLICKED/VALUE_CHANGED handlers ($event_clicked) exceed threshold ($EVENT_CLICKED_THRESHOLD)"
fi
if [ "$visibility_actionable" -gt "$VISIBILITY_THRESHOLD" ]; then
    warning "Actionable visibility toggles ($visibility_actionable) exceed threshold ($VISIBILITY_THRESHOLD)"
fi
if [ "$text_updates" -gt "$TEXT_UPDATE_THRESHOLD" ]; then
    warning "Direct text updates ($text_updates) exceed threshold ($TEXT_UPDATE_THRESHOLD)"
fi
if [ "$inline_styles" -gt "$INLINE_STYLE_THRESHOLD" ]; then
    warning "Inline style setters ($inline_styles) exceed threshold ($INLINE_STYLE_THRESHOLD)"
fi

#
# === P5: Code Size ===
#
section "P5: Code Organization (File Size)"

MAX_LINES=2500
echo "Files exceeding $MAX_LINES lines:"
oversized=0
for f in src/ui_panel_*.cpp; do
    [ -f "$f" ] || continue
    lines=$(wc -l < "$f")
    if [ "$lines" -gt "$MAX_LINES" ]; then
        warning "$(basename "$f") has $lines lines (max: $MAX_LINES)"
        ((oversized++)) || true
    fi
done
if [ "$oversized" -eq 0 ]; then
    success "No files exceed $MAX_LINES lines"
fi

#
# === Hardcoded Colors in C++ ===
#
section "C++ Hardcoded Colors"

set +e
color_literals=$(grep -rn 'lv_color_hex\|lv_color_make' src/ui_*.cpp 2>/dev/null | grep -v 'theme\|parse' | wc -l | tr -d ' ')
set -e
echo "Hardcoded color literals: $color_literals"

COLOR_LITERAL_THRESHOLD=50
if [ "$color_literals" -gt "$COLOR_LITERAL_THRESHOLD" ]; then
    warning "Hardcoded color literals ($color_literals) exceed threshold ($COLOR_LITERAL_THRESHOLD)"
fi

#
# === Summary ===
#
section "Summary"

echo ""
echo "Errors:   $ERRORS"
echo "Warnings: $WARNINGS"
echo ""

if [ "$ERRORS" -gt 0 ]; then
    echo -e "${RED}AUDIT FAILED${NC} - $ERRORS critical error(s) found"
    exit 1
elif [ "$WARNINGS" -gt 0 ] && [ "$STRICT_MODE" = true ]; then
    echo -e "${YELLOW}AUDIT FAILED (strict mode)${NC} - $WARNINGS warning(s) found"
    exit 1
elif [ "$WARNINGS" -gt 0 ]; then
    echo -e "${YELLOW}AUDIT PASSED WITH WARNINGS${NC} - $WARNINGS warning(s) found"
    exit 0
else
    echo -e "${GREEN}AUDIT PASSED${NC} - No issues found"
    exit 0
fi
