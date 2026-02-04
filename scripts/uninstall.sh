#!/bin/sh
# Copyright (C) 2025-2026 356C LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# HelixScreen Uninstaller
# Removes HelixScreen and restores the previous screen UI
#
# Usage:
#   ./uninstall.sh              # Interactive uninstall
#   ./uninstall.sh --force      # Skip confirmation prompt
#
# This script:
#   1. Stops HelixScreen
#   2. Removes init script or systemd service
#   3. Removes installation directory
#   4. Re-enables previous UI (GuppyScreen, FeatherScreen, etc.)

set -e

# Source modules
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LIB_DIR="$SCRIPT_DIR/lib/installer"

. "$LIB_DIR/common.sh"
. "$LIB_DIR/platform.sh"
. "$LIB_DIR/permissions.sh"
. "$LIB_DIR/requirements.sh"
. "$LIB_DIR/forgex.sh"
. "$LIB_DIR/service.sh"
. "$LIB_DIR/moonraker.sh"
. "$LIB_DIR/uninstall.sh"

# Previous UIs we may need to re-enable (for scanning)
PREVIOUS_UIS="guppyscreen GuppyScreen featherscreen FeatherScreen klipperscreen KlipperScreen"

# Re-enable previous UI (extended version with scanning)
reenable_previous_ui() {
    log_info "Looking for previous screen UI to re-enable..."

    found_ui=false
    restored_xorg=false

    # For ForgeX firmware, restore display setting and stock UI in auto_run.sh
    if [ "$AD5M_FIRMWARE" = "forge_x" ]; then
        # Restore display mode to GUPPY if needed
        if [ -f "/opt/config/mod_data/variables.cfg" ]; then
            if grep -q "display[[:space:]]*=[[:space:]]*'STOCK'" "/opt/config/mod_data/variables.cfg"; then
                log_info "Restoring GuppyScreen in ForgeX configuration..."
                $SUDO sed -i "s/display[[:space:]]*=[[:space:]]*'STOCK'/display = 'GUPPY'/" "/opt/config/mod_data/variables.cfg"
                log_success "ForgeX display mode restored to GUPPY"
            fi
        fi
        restore_stock_firmware_ui || true
        unpatch_forgex_screen_sh || true
    fi

    # For K1/Simple AF, check for GuppyScreen
    if [ "$platform" = "k1" ]; then
        for k1_ui in /etc/init.d/S99guppyscreen /etc/init.d/S50guppyscreen; do
            if [ -f "$k1_ui" ]; then
                log_info "Re-enabling GuppyScreen for K1..."
                $SUDO chmod +x "$k1_ui" 2>/dev/null || true
                if "$k1_ui" start 2>/dev/null; then
                    log_success "Re-enabled and started: $k1_ui"
                    found_ui=true
                    break
                fi
            fi
        done
    fi

    # For Klipper Mod, re-enable Xorg first (required for KlipperScreen)
    if [ "$AD5M_FIRMWARE" = "klipper_mod" ] || [ -f "/etc/init.d/S40xorg" ]; then
        if [ -f "/etc/init.d/S40xorg" ]; then
            log_info "Re-enabling Xorg display server..."
            $SUDO chmod +x "/etc/init.d/S40xorg" 2>/dev/null || true
            restored_xorg=true
        fi
    fi

    # First, try the specific previous UI script for this firmware
    if [ -n "$PREVIOUS_UI_SCRIPT" ] && [ -f "$PREVIOUS_UI_SCRIPT" ]; then
        log_info "Found previous UI: $PREVIOUS_UI_SCRIPT"
        $SUDO chmod +x "$PREVIOUS_UI_SCRIPT" 2>/dev/null || true
        if "$PREVIOUS_UI_SCRIPT" start 2>/dev/null; then
            log_success "Re-enabled and started: $PREVIOUS_UI_SCRIPT"
            found_ui=true
        else
            log_warn "Re-enabled but failed to start: $PREVIOUS_UI_SCRIPT"
            log_warn "You may need to reboot"
            found_ui=true
        fi
    fi

    # Also scan for other UIs we might have disabled
    for ui in $PREVIOUS_UIS; do
        # Check for init.d scripts
        for initscript in /etc/init.d/S*${ui}* /opt/config/mod/.root/S*${ui}*; do
            # Skip if this is the PREVIOUS_UI_SCRIPT we already handled
            if [ "$initscript" = "$PREVIOUS_UI_SCRIPT" ]; then
                continue
            fi
            if [ -f "$initscript" ] 2>/dev/null; then
                log_info "Found previous UI: $initscript"
                # Re-enable by making executable
                $SUDO chmod +x "$initscript" 2>/dev/null || true
                # Start it (only if we haven't already started one)
                if [ "$found_ui" = false ]; then
                    if "$initscript" start 2>/dev/null; then
                        log_success "Re-enabled and started: $initscript"
                        found_ui=true
                    else
                        log_warn "Re-enabled but failed to start: $initscript"
                        log_warn "You may need to reboot"
                        found_ui=true
                    fi
                else
                    log_info "Re-enabled: $initscript (not started, another UI already running)"
                fi
            fi
        done

        # Check for systemd services
        if [ "$INIT_SYSTEM" = "systemd" ]; then
            if systemctl list-unit-files "${ui}.service" >/dev/null 2>&1; then
                log_info "Found previous UI (systemd): $ui"
                $SUDO systemctl enable "$ui" 2>/dev/null || true
                if $SUDO systemctl start "$ui" 2>/dev/null; then
                    log_success "Re-enabled and started: $ui"
                    found_ui=true
                else
                    log_warn "Re-enabled but failed to start: $ui"
                    found_ui=true
                fi
            fi
        fi
    done

    # Re-enable tslib for ForgeX
    if [ -f "/opt/config/mod/.root/S35tslib" ]; then
        $SUDO chmod +x "/opt/config/mod/.root/S35tslib" 2>/dev/null || true
    fi

    if [ "$found_ui" = false ]; then
        log_info "No previous screen UI found to re-enable"
        log_info "If you had a stock UI, a reboot may restore it"
    fi

    if [ "$restored_xorg" = true ]; then
        log_info "Re-enabled: Xorg (/etc/init.d/S40xorg)"
    fi
}

# Stop HelixScreen processes and service
stop_helixscreen() {
    log_info "Stopping HelixScreen..."

    # Stop via init system
    if [ "$INIT_SYSTEM" = "systemd" ]; then
        $SUDO systemctl stop helixscreen 2>/dev/null || true
        $SUDO systemctl disable helixscreen 2>/dev/null || true
    fi

    # Stop via configured init script
    if [ -n "$INIT_SCRIPT_DEST" ] && [ -x "$INIT_SCRIPT_DEST" ]; then
        $SUDO "$INIT_SCRIPT_DEST" stop 2>/dev/null || true
    fi

    # Also check all possible init script locations
    for init_script in /etc/init.d/S80helixscreen /etc/init.d/S90helixscreen /etc/init.d/S99helixscreen; do
        if [ -x "$init_script" ]; then
            $SUDO "$init_script" stop 2>/dev/null || true
        fi
    done

    # Kill any remaining processes (watchdog first to prevent crash dialog flash)
    if command -v killall >/dev/null 2>&1; then
        $SUDO killall helix-watchdog 2>/dev/null || true
        $SUDO killall helix-screen 2>/dev/null || true
        $SUDO killall helix-splash 2>/dev/null || true
    elif command -v pidof >/dev/null 2>&1; then
        for proc in helix-watchdog helix-screen helix-splash; do
            for pid in $(pidof "$proc" 2>/dev/null); do
                $SUDO kill "$pid" 2>/dev/null || true
            done
        done
    fi

    log_success "HelixScreen stopped"
}

# Remove init script or systemd service
remove_service() {
    log_info "Removing service configuration..."

    if [ "$INIT_SYSTEM" = "systemd" ]; then
        if [ -f "/etc/systemd/system/helixscreen.service" ]; then
            $SUDO rm -f "/etc/systemd/system/helixscreen.service"
            $SUDO systemctl daemon-reload
            log_success "Removed systemd service"
        fi
    fi

    # Remove configured init script
    if [ -n "$INIT_SCRIPT_DEST" ] && [ -f "$INIT_SCRIPT_DEST" ]; then
        $SUDO rm -f "$INIT_SCRIPT_DEST"
        log_success "Removed SysV init script: $INIT_SCRIPT_DEST"
    fi

    # Also check and remove from all possible locations
    for init_script in /etc/init.d/S80helixscreen /etc/init.d/S90helixscreen /etc/init.d/S99helixscreen; do
        if [ -f "$init_script" ]; then
            $SUDO rm -f "$init_script"
            log_success "Removed SysV init script: $init_script"
        fi
    done
}

# Remove installation directory
remove_installation() {
    log_info "Removing installation..."

    removed_any=false

    # Remove from configured location
    if [ -d "$INSTALL_DIR" ]; then
        $SUDO rm -rf "$INSTALL_DIR"
        log_success "Removed $INSTALL_DIR"
        removed_any=true
    fi

    # Also check and remove from all possible locations
    for install_dir in /root/printer_software/helixscreen /opt/helixscreen /usr/data/helixscreen; do
        if [ -d "$install_dir" ] && [ "$install_dir" != "$INSTALL_DIR" ]; then
            $SUDO rm -rf "$install_dir"
            log_success "Removed $install_dir"
            removed_any=true
        fi
    done

    if [ "$removed_any" = false ]; then
        log_warn "No HelixScreen installation found (already removed?)"
    fi

    # Clean up PID files, log file, and active flag
    $SUDO rm -f /var/run/helixscreen.pid 2>/dev/null || true
    $SUDO rm -f /var/run/helix-splash.pid 2>/dev/null || true
    $SUDO rm -f /tmp/helixscreen.log 2>/dev/null || true
    $SUDO rm -f /tmp/helixscreen_active 2>/dev/null || true
}

# Main uninstall
main() {
    force=false

    # Parse arguments
    while [ $# -gt 0 ]; do
        case "$1" in
            --force|-f)
                force=true
                shift
                ;;
            --help|-h)
                echo "HelixScreen Uninstaller"
                echo ""
                echo "Usage: $0 [options]"
                echo ""
                echo "Options:"
                echo "  --force, -f   Skip confirmation prompt"
                echo "  --help, -h    Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done

    echo ""
    echo "${CYAN}========================================${NC}"
    echo "${CYAN}     HelixScreen Uninstaller${NC}"
    echo "${CYAN}========================================${NC}"
    echo ""

    # Detect platform and firmware to set correct paths
    platform=$(detect_platform)
    if [ "$platform" = "ad5m" ]; then
        AD5M_FIRMWARE=$(detect_ad5m_firmware)
        log_info "Detected AD5M firmware: $AD5M_FIRMWARE"
    fi
    set_install_paths "$platform" "$AD5M_FIRMWARE"

    # Check for root
    check_permissions "$platform"

    # Detect init system
    detect_init_system

    # Confirm unless --force
    if [ "$force" = false ]; then
        echo "This will:"
        echo "  - Stop HelixScreen"
        echo "  - Remove $INSTALL_DIR"
        echo "  - Remove service configuration"
        echo "  - Re-enable previous screen UI (if found)"
        if [ "$AD5M_FIRMWARE" = "forge_x" ]; then
            echo "  - Restore ForgeX display configuration (GuppyScreen)"
        fi
        echo ""
        printf "Are you sure you want to continue? [y/N] "
        read -r response
        case "$response" in
            [yY][eE][sS]|[yY])
                ;;
            *)
                log_info "Uninstall cancelled"
                exit 0
                ;;
        esac
        echo ""
    fi

    # Perform uninstall
    stop_helixscreen
    remove_service
    remove_installation
    reenable_previous_ui
    remove_update_manager_section || true

    echo ""
    echo "${GREEN}========================================${NC}"
    echo "${GREEN}    Uninstall Complete!${NC}"
    echo "${GREEN}========================================${NC}"
    echo ""
    log_info "HelixScreen has been removed."
    log_info "A reboot is recommended to ensure clean state."
    echo ""
}

main "$@"
