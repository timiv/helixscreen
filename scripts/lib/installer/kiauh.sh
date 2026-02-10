#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: kiauh
# KIAUH extension auto-detection and installation
#
# Reads: INSTALL_DIR
# Writes: -

# Source guard
[ -n "${_HELIX_KIAUH_SOURCED:-}" ] && return 0
_HELIX_KIAUH_SOURCED=1

# Detect KIAUH extensions directory
# Returns the path to the extensions dir, or empty string if not found
detect_kiauh_dir() {
    # Check current user's home first (most common)
    if [ -d "$HOME/kiauh/kiauh/extensions" ]; then
        echo "$HOME/kiauh/kiauh/extensions"
        return 0
    fi

    # Scan /home/*/kiauh/kiauh/extensions/ for other users
    if [ -d "/home" ]; then
        for user_home in /home/*/kiauh/kiauh/extensions; do
            if [ -d "$user_home" ]; then
                echo "$user_home"
                return 0
            fi
        done
    fi

    # Not found
    echo ""
    return 0
}

# Install KIAUH extension for HelixScreen
# Args: $1 = kiauh_mode ("yes", "no", or "" for interactive)
install_kiauh_extension() {
    local kiauh_mode="${1:-}"
    local kiauh_ext_dir
    local src_dir="$INSTALL_DIR/scripts/kiauh/helixscreen"

    kiauh_ext_dir=$(detect_kiauh_dir)

    # No KIAUH installed — nothing to do
    if [ -z "$kiauh_ext_dir" ]; then
        return 0
    fi

    local target_dir="$kiauh_ext_dir/helixscreen"
    local is_update=false

    if [ -d "$target_dir" ]; then
        is_update=true
    fi

    # Check source files exist
    if [ ! -f "$src_dir/__init__.py" ] || [ ! -f "$src_dir/helixscreen_extension.py" ] || [ ! -f "$src_dir/metadata.json" ]; then
        log_warn "KIAUH extension source files not found in release package"
        return 0
    fi

    # For new installs, handle interactive/forced modes
    if [ "$is_update" = false ]; then
        case "$kiauh_mode" in
            yes)
                log_info "Installing KIAUH extension (--kiauh yes)..."
                ;;
            no)
                log_info "Skipping KIAUH extension (--kiauh no)"
                return 0
                ;;
            *)
                # Interactive mode
                printf "KIAUH detected. Install HelixScreen extension for KIAUH? [Y/n] "
                read -r reply </dev/tty 2>/dev/null || reply="y"
                case "$reply" in
                    [Nn]*)
                        log_info "Skipping KIAUH extension"
                        return 0
                        ;;
                esac
                ;;
        esac
    fi

    # Copy extension files
    mkdir -p "$target_dir"
    cp "$src_dir/__init__.py" "$target_dir/"
    cp "$src_dir/helixscreen_extension.py" "$target_dir/"
    cp "$src_dir/metadata.json" "$target_dir/"

    if [ "$is_update" = true ]; then
        log_info "Updated KIAUH extension in $target_dir"
    else
        log_success "Installed KIAUH extension to $target_dir"
    fi
}
