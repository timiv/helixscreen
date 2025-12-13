#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# HelixPrint Moonraker Plugin Installer
#
# This script creates a symlink from Moonraker's components directory
# to the helix_print.py plugin file.
#
# Usage:
#   ./install.sh              # Auto-detect Moonraker location
#   ./install.sh /path/to/moonraker  # Specify Moonraker path
#
# The script will:
#   1. Find Moonraker's installation directory
#   2. Create a symlink to helix_print.py in the components directory
#   3. Remind you to add [helix_print] to moonraker.conf

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_FILE="$SCRIPT_DIR/helix_print.py"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# Find Moonraker installation
find_moonraker() {
    local moonraker_path=""

    # Check common locations
    local locations=(
        "$HOME/moonraker"
        "$HOME/klipper_config/moonraker"
        "/home/pi/moonraker"
        "/home/klipper/moonraker"
        "$1"  # User-provided path
    )

    for loc in "${locations[@]}"; do
        if [[ -n "$loc" && -d "$loc/moonraker/components" ]]; then
            moonraker_path="$loc"
            break
        fi
    done

    # Also check if moonraker is installed as a package
    if [[ -z "$moonraker_path" ]]; then
        local pip_loc=$(python3 -c "import moonraker; print(moonraker.__path__[0])" 2>/dev/null || true)
        if [[ -n "$pip_loc" && -d "$pip_loc/components" ]]; then
            moonraker_path="$(dirname "$pip_loc")"
        fi
    fi

    echo "$moonraker_path"
}

# Main installation
main() {
    info "HelixPrint Moonraker Plugin Installer"
    echo ""

    # Check plugin file exists
    if [[ ! -f "$PLUGIN_FILE" ]]; then
        error "Plugin file not found: $PLUGIN_FILE"
    fi

    # Find Moonraker
    local moonraker_path=$(find_moonraker "$1")

    if [[ -z "$moonraker_path" ]]; then
        error "Could not find Moonraker installation.
Please provide the path: ./install.sh /path/to/moonraker"
    fi

    local components_dir="$moonraker_path/moonraker/components"

    if [[ ! -d "$components_dir" ]]; then
        error "Components directory not found: $components_dir"
    fi

    info "Found Moonraker at: $moonraker_path"
    info "Components directory: $components_dir"
    echo ""

    # Create symlink
    local target="$components_dir/helix_print.py"

    if [[ -L "$target" ]]; then
        warn "Symlink already exists, removing..."
        rm "$target"
    elif [[ -f "$target" ]]; then
        error "A file (not symlink) exists at $target
Please remove it manually before installing."
    fi

    ln -s "$PLUGIN_FILE" "$target"
    info "Created symlink: $target -> $PLUGIN_FILE"
    echo ""

    # Remind about configuration
    info "Installation complete!"
    echo ""
    echo "Next steps:"
    echo "  1. Add the following to your moonraker.conf:"
    echo ""
    echo "     [helix_print]"
    echo "     # enabled: True"
    echo "     # temp_dir: .helix_temp"
    echo "     # symlink_dir: .helix_print"
    echo "     # cleanup_delay: 86400"
    echo ""
    echo "  2. Restart Moonraker:"
    echo "     sudo systemctl restart moonraker"
    echo ""
    echo "  3. Verify the plugin is loaded:"
    echo "     curl http://localhost:7125/server/helix/status"
    echo ""
}

# Uninstall function
uninstall() {
    info "Uninstalling HelixPrint plugin..."

    local moonraker_path=$(find_moonraker "$1")

    if [[ -z "$moonraker_path" ]]; then
        error "Could not find Moonraker installation."
    fi

    local target="$moonraker_path/moonraker/components/helix_print.py"

    if [[ -L "$target" ]]; then
        rm "$target"
        info "Removed symlink: $target"
    elif [[ -f "$target" ]]; then
        warn "Found regular file (not symlink) at $target"
        warn "Please remove it manually if desired."
    else
        info "Plugin symlink not found (already uninstalled?)"
    fi

    echo ""
    echo "Don't forget to:"
    echo "  1. Remove [helix_print] section from moonraker.conf"
    echo "  2. Restart Moonraker: sudo systemctl restart moonraker"
}

# Parse arguments
case "${1:-}" in
    --uninstall|-u)
        uninstall "$2"
        ;;
    --help|-h)
        echo "Usage: $0 [OPTIONS] [MOONRAKER_PATH]"
        echo ""
        echo "Options:"
        echo "  --uninstall, -u    Remove the plugin symlink"
        echo "  --help, -h         Show this help message"
        echo ""
        echo "Arguments:"
        echo "  MOONRAKER_PATH     Path to Moonraker installation (auto-detected if not provided)"
        ;;
    *)
        main "$1"
        ;;
esac
