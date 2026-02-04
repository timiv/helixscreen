#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: requirements
# Pre-flight checks: commands, dependencies, disk space, init system
#
# Reads: PLATFORM, SUDO
# Writes: INIT_SYSTEM

# Source guard
[ -n "${_HELIX_REQUIREMENTS_SOURCED:-}" ] && return 0
_HELIX_REQUIREMENTS_SOURCED=1

# Initialize INIT_SYSTEM (will be set by detect_init_system)
INIT_SYSTEM=""

# Check required commands exist
# Requires: curl or wget, tar, gunzip
check_requirements() {
    local missing=""

    # Need either curl or wget
    if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
        missing="curl or wget"
    fi

    # Need tar
    if ! command -v tar >/dev/null 2>&1; then
        if [ -n "$missing" ]; then
            missing="$missing, tar"
        else
            missing="tar"
        fi
    fi

    # Need gunzip (for AD5M BusyBox tar which doesn't support -z)
    if ! command -v gunzip >/dev/null 2>&1; then
        if [ -n "$missing" ]; then
            missing="$missing, gunzip"
        else
            missing="gunzip"
        fi
    fi

    if [ -n "$missing" ]; then
        log_error "Missing required commands: $missing"
        log_error "Please install them and try again."
        exit 1
    fi

    log_success "All required commands available"
}

# Install runtime dependencies for Pi platform
# Required for DRM display and evdev input handling
# AD5M uses framebuffer with static linking, no deps needed
install_runtime_deps() {
    local platform=$1

    # Only needed for Pi - AD5M uses framebuffer with static linking
    if [ "$platform" != "pi" ]; then
        return 0
    fi

    log_info "Checking runtime dependencies for display/input..."

    # Required libraries for DRM display and libinput
    # Note: GPU libs (libgles2, libegl1, libgbm1) not needed - using software rendering
    local deps="libdrm2 libinput10"
    local missing=""

    for dep in $deps; do
        # Check if package is installed (dpkg-query returns 0 if installed)
        if ! dpkg-query -W -f='${Status}' "$dep" 2>/dev/null | grep -q "install ok installed"; then
            if [ -n "$missing" ]; then
                missing="$missing $dep"
            else
                missing="$dep"
            fi
        fi
    done

    if [ -n "$missing" ]; then
        log_info "Installing missing libraries: $missing"
        $SUDO apt-get update -qq
        # shellcheck disable=SC2086
        $SUDO apt-get install -y --no-install-recommends $missing
        log_success "Runtime libraries installed"
    else
        log_success "All runtime libraries already installed"
    fi
}

# Check available disk space
# Requires at least 50MB free on the install directory's filesystem
# Note: INSTALL_DIR must be set before calling this function
check_disk_space() {
    local platform=$1
    local required_mb=50

    # Get the parent directory of install location (the filesystem to check)
    local check_dir
    check_dir=$(dirname "${INSTALL_DIR:-/opt/helixscreen}")
    # Walk up until we find an existing directory
    while [ ! -d "$check_dir" ] && [ "$check_dir" != "/" ]; do
        check_dir=$(dirname "$check_dir")
    done
    if [ "$check_dir" = "/" ]; then
        check_dir="/"
    fi

    # Get available space in MB
    local available_mb
    if [ "$platform" = "ad5m" ] || [ "$platform" = "k1" ]; then
        # BusyBox df output format: blocks are in KB by default
        available_mb=$(df "$check_dir" 2>/dev/null | tail -1 | awk '{print int($4/1024)}')
    else
        # GNU df with -m flag outputs in MB
        available_mb=$(df -m "$check_dir" 2>/dev/null | tail -1 | awk '{print $4}')
    fi

    if [ -n "$available_mb" ] && [ "$available_mb" -lt "$required_mb" ]; then
        log_error "Insufficient disk space on $check_dir"
        log_error "Required: ${required_mb}MB, Available: ${available_mb}MB"
        exit 1
    fi

    log_info "Disk space check: ${available_mb}MB available on $check_dir"
}

# Detect init system (systemd vs SysV)
# Sets: INIT_SYSTEM to "systemd" or "sysv"
detect_init_system() {
    # Check for systemd
    if command -v systemctl >/dev/null 2>&1 && [ -d /run/systemd/system ]; then
        INIT_SYSTEM="systemd"
        log_info "Init system: systemd"
        return
    fi

    # Check for SysV init (BusyBox or traditional)
    if [ -d /etc/init.d ]; then
        INIT_SYSTEM="sysv"
        log_info "Init system: SysV (BusyBox/traditional)"
        return
    fi

    log_error "Could not detect init system."
    log_error "Neither systemd nor /etc/init.d found."
    exit 1
}
