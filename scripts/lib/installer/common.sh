#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: common
# Core utilities: logging, colors, error handling, cleanup
#
# Reads: -
# Writes: RED, GREEN, YELLOW, CYAN, BOLD, NC, CLEANUP_TMP, BACKUP_CONFIG, INSTALL_DIR, GITHUB_REPO

# Source guard
[ -n "${_HELIX_COMMON_SOURCED:-}" ] && return 0
_HELIX_COMMON_SOURCED=1

# Default configuration (can be overridden before sourcing)
: "${GITHUB_REPO:=prestonbrown/helixscreen}"
: "${INSTALL_DIR:=/opt/helixscreen}"

# Track what we've done for cleanup
CLEANUP_TMP=false
CLEANUP_SERVICE=false
BACKUP_CONFIG=""
ORIGINAL_INSTALL_EXISTS=false

# Colors (if terminal supports it)
setup_colors() {
    if [ -t 1 ]; then
        RED='\033[0;31m'
        GREEN='\033[0;32m'
        YELLOW='\033[1;33m'
        CYAN='\033[0;36m'
        BOLD='\033[1m'
        NC='\033[0m'
    else
        RED=''
        GREEN=''
        YELLOW=''
        CYAN=''
        BOLD=''
        NC=''
    fi
}

# Initialize colors immediately
setup_colors

# Logging functions
log_info() { echo "${CYAN}[INFO]${NC} $1"; }
log_success() { echo "${GREEN}[OK]${NC} $1"; }
log_warn() { echo "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo "${RED}[ERROR]${NC} $1" >&2; }

# Error handler - cleanup and report what went wrong
# Usage: trap 'error_handler $LINENO' ERR
error_handler() {
    local exit_code=$?
    local line_no=$1

    echo ""
    log_error "=========================================="
    log_error "Installation FAILED at line $line_no"
    log_error "Exit code: $exit_code"
    log_error "=========================================="
    echo ""

    # Cleanup temporary files
    if [ "$CLEANUP_TMP" = true ] && [ -d "$TMP_DIR" ]; then
        log_info "Cleaning up temporary files..."
        rm -rf "$TMP_DIR"
    fi

    # If we backed up config and install failed, try to restore state
    if [ -n "$BACKUP_CONFIG" ] && [ -f "$BACKUP_CONFIG" ]; then
        log_info "Restoring backed up configuration..."
        if $SUDO mkdir -p "${INSTALL_DIR}/config" 2>/dev/null; then
            if $SUDO cp "$BACKUP_CONFIG" "${INSTALL_DIR}/config/helixconfig.json" 2>/dev/null; then
                log_success "Configuration restored"
            else
                log_warn "Could not restore config. Backup saved at: $BACKUP_CONFIG"
            fi
        else
            log_warn "Could not create config directory. Backup saved at: $BACKUP_CONFIG"
        fi
    fi

    echo ""
    log_error "Installation was NOT completed."
    log_error "Your system should be in its original state."
    echo ""
    log_info "For help, please:"
    log_info "  1. Check the error message above"
    log_info "  2. Verify network connectivity"
    log_info "  3. Report issues at: https://github.com/${GITHUB_REPO}/issues"
    echo ""

    exit $exit_code
}

# Cleanup function for normal exit
cleanup_on_success() {
    if [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
    fi
}
