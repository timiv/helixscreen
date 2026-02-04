#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: platform
# Platform detection: AD5M vs K1 vs Pi, firmware variant, installation paths
#
# Reads: -
# Writes: PLATFORM, AD5M_FIRMWARE, K1_FIRMWARE, INSTALL_DIR, INIT_SCRIPT_DEST, PREVIOUS_UI_SCRIPT, TMP_DIR

# Source guard
[ -n "${_HELIX_PLATFORM_SOURCED:-}" ] && return 0
_HELIX_PLATFORM_SOURCED=1

# Default paths (may be overridden by set_install_paths)
: "${INSTALL_DIR:=/opt/helixscreen}"
: "${TMP_DIR:=/tmp/helixscreen-install}"
INIT_SCRIPT_DEST=""
PREVIOUS_UI_SCRIPT=""
AD5M_FIRMWARE=""
K1_FIRMWARE=""

# Detect platform
# Returns: "ad5m", "k1", "pi", or "unsupported"
detect_platform() {
    local arch kernel
    arch=$(uname -m)
    kernel=$(uname -r)

    # Check for AD5M (armv7l with specific kernel)
    if [ "$arch" = "armv7l" ]; then
        # AD5M has a specific kernel identifier
        if echo "$kernel" | grep -q "ad5m\|5.4.61"; then
            echo "ad5m"
            return
        fi
    fi

    # Check for Creality K1 series (Simple AF or stock with Klipper)
    # K1 uses buildroot and has /usr/data structure
    if [ -f /etc/os-release ] && grep -q "buildroot" /etc/os-release 2>/dev/null; then
        # Buildroot-based system - check for K1 indicators
        if [ -d "/usr/data" ]; then
            # Check for K1-specific indicators (require at least 2 for confidence)
            # - get_sn_mac.sh is a Creality-specific script
            # - /usr/data/pellcorp is Simple AF
            # - /usr/data/printer_data with klipper is a strong K1 indicator
            local k1_indicators=0
            [ -x "/usr/bin/get_sn_mac.sh" ] && k1_indicators=$((k1_indicators + 1))
            [ -d "/usr/data/pellcorp" ] && k1_indicators=$((k1_indicators + 1))
            [ -d "/usr/data/printer_data" ] && k1_indicators=$((k1_indicators + 1))
            [ -d "/usr/data/klipper" ] && k1_indicators=$((k1_indicators + 1))
            # Also check for Creality-specific paths
            [ -f "/usr/data/creality/userdata/config/system_config.json" ] && k1_indicators=$((k1_indicators + 1))

            if [ "$k1_indicators" -ge 2 ]; then
                echo "k1"
                return
            fi
        fi
    fi

    # Check for Raspberry Pi (aarch64 or armv7l)
    if [ "$arch" = "aarch64" ] || [ "$arch" = "armv7l" ]; then
        if [ -f /etc/os-release ] && grep -q "Raspbian\|Debian" /etc/os-release; then
            echo "pi"
            return
        fi
        # Also check for MainsailOS
        if [ -d /home/pi ] || [ -d /home/mks ]; then
            echo "pi"
            return
        fi
    fi

    # Unknown ARM device - don't assume it's a Pi
    # Require explicit platform indicators to avoid false positives
    if [ "$arch" = "aarch64" ] || [ "$arch" = "armv7l" ]; then
        log_warn "Unknown ARM platform. Cannot auto-detect."
        echo "unsupported"
        return
    fi

    echo "unsupported"
}

# Detect AD5M firmware variant (Klipper Mod vs Forge-X)
# Only called when platform is "ad5m"
# Returns: "klipper_mod" or "forge_x"
detect_ad5m_firmware() {
    # Klipper Mod indicators - check for its specific directory structure
    # Klipper Mod runs in a chroot on /mnt/data/.klipper_mod/chroot
    # and puts printer software in /root/printer_software/
    if [ -d "/root/printer_software" ] || [ -d "/mnt/data/.klipper_mod" ]; then
        echo "klipper_mod"
        return
    fi

    # Forge-X indicators - check for its mod overlay structure
    if [ -d "/opt/config/mod/.root" ]; then
        echo "forge_x"
        return
    fi

    # Default to forge_x (original behavior, most common)
    echo "forge_x"
}

# Detect K1 firmware variant (Simple AF vs other)
# Only called when platform is "k1"
# Returns: "simple_af" or "stock_klipper"
detect_k1_firmware() {
    # Simple AF (pellcorp/creality) indicators
    if [ -d "/usr/data/pellcorp" ]; then
        echo "simple_af"
        return
    fi

    # Check for GuppyScreen which Simple AF installs
    if [ -d "/usr/data/guppyscreen" ] && [ -f "/etc/init.d/S99guppyscreen" ]; then
        echo "simple_af"
        return
    fi

    # Default to stock_klipper (generic K1 with Klipper)
    echo "stock_klipper"
}

# Set installation paths based on platform and firmware
# Sets: INSTALL_DIR, INIT_SCRIPT_DEST, PREVIOUS_UI_SCRIPT, TMP_DIR
set_install_paths() {
    local platform=$1
    local firmware=${2:-}

    if [ "$platform" = "ad5m" ]; then
        case "$firmware" in
            klipper_mod)
                INSTALL_DIR="/root/printer_software/helixscreen"
                INIT_SCRIPT_DEST="/etc/init.d/S80helixscreen"
                PREVIOUS_UI_SCRIPT="/etc/init.d/S80klipperscreen"
                # Klipper Mod has small tmpfs (~54MB), package is ~70MB
                # Use /mnt/data which has 4+ GB available
                TMP_DIR="/mnt/data/helixscreen-install"
                log_info "AD5M firmware: Klipper Mod"
                log_info "Install directory: ${INSTALL_DIR}"
                log_info "Using /mnt/data for temp files (tmpfs too small)"
                ;;
            forge_x|*)
                INSTALL_DIR="/opt/helixscreen"
                INIT_SCRIPT_DEST="/etc/init.d/S90helixscreen"
                PREVIOUS_UI_SCRIPT="/opt/config/mod/.root/S80guppyscreen"
                TMP_DIR="/tmp/helixscreen-install"
                log_info "AD5M firmware: Forge-X"
                log_info "Install directory: ${INSTALL_DIR}"
                ;;
        esac
    elif [ "$platform" = "k1" ]; then
        # Creality K1 series - uses /usr/data structure
        case "$firmware" in
            simple_af|*)
                INSTALL_DIR="/usr/data/helixscreen"
                INIT_SCRIPT_DEST="/etc/init.d/S99helixscreen"
                PREVIOUS_UI_SCRIPT="/etc/init.d/S99guppyscreen"
                TMP_DIR="/tmp/helixscreen-install"
                log_info "K1 firmware: Simple AF"
                log_info "Install directory: ${INSTALL_DIR}"
                ;;
        esac
    else
        # Pi and other platforms - use default paths
        INSTALL_DIR="/opt/helixscreen"
        INIT_SCRIPT_DEST="/etc/init.d/S90helixscreen"
        PREVIOUS_UI_SCRIPT=""
        TMP_DIR="/tmp/helixscreen-install"
    fi
}
