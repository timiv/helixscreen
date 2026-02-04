#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: release
# Release download and extraction
#
# Reads: GITHUB_REPO, TMP_DIR, INSTALL_DIR, SUDO
# Writes: CLEANUP_TMP, BACKUP_CONFIG, ORIGINAL_INSTALL_EXISTS

# Source guard
[ -n "${_HELIX_RELEASE_SOURCED:-}" ] && return 0
_HELIX_RELEASE_SOURCED=1

# Get latest release version from GitHub
get_latest_version() {
    local url="https://api.github.com/repos/${GITHUB_REPO}/releases/latest"
    local version=""

    log_info "Fetching latest version from GitHub..."

    if command -v curl >/dev/null 2>&1; then
        version=$(curl -sSL --connect-timeout 10 "$url" 2>/dev/null | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')
    elif command -v wget >/dev/null 2>&1; then
        version=$(wget -qO- --timeout=10 "$url" 2>/dev/null | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')
    fi

    if [ -z "$version" ]; then
        log_error "Failed to fetch latest version from GitHub."
        log_error "Check your network connection and try again."
        log_error "URL: $url"
        exit 1
    fi

    echo "$version"
}

# Download release tarball
download_release() {
    local version=$1
    local platform=$2

    local filename="helixscreen-${platform}-${version}.tar.gz"
    local url="https://github.com/${GITHUB_REPO}/releases/download/${version}/${filename}"
    local dest="${TMP_DIR}/helixscreen.tar.gz"

    log_info "Downloading HelixScreen ${version} for ${platform}..."
    log_info "URL: $url"

    mkdir -p "$TMP_DIR"
    CLEANUP_TMP=true

    local http_code=""
    if command -v curl >/dev/null 2>&1; then
        http_code=$(curl -sSL --connect-timeout 30 -w "%{http_code}" -o "$dest" "$url")
    elif command -v wget >/dev/null 2>&1; then
        if wget -q --timeout=30 -O "$dest" "$url"; then
            http_code="200"
        else
            http_code="failed"
        fi
    fi

    if [ ! -f "$dest" ] || [ ! -s "$dest" ]; then
        log_error "Failed to download release."
        log_error "URL: $url"
        if [ -n "$http_code" ] && [ "$http_code" != "200" ]; then
            log_error "HTTP status: $http_code"
        fi
        log_error ""
        log_error "Possible causes:"
        log_error "  - Version ${version} may not exist for platform ${platform}"
        log_error "  - Network connectivity issues"
        log_error "  - GitHub may be unavailable"
        exit 1
    fi

    # Verify it's a valid gzip file
    if ! gunzip -t "$dest" 2>/dev/null; then
        log_error "Downloaded file is not a valid gzip archive."
        log_error "The download may have been corrupted or incomplete."
        exit 1
    fi

    # Verify download isn't truncated (releases should be >1MB)
    local size_kb
    size_kb=$(du -k "$dest" 2>/dev/null | cut -f1)
    if [ "${size_kb:-0}" -lt 1024 ]; then
        log_error "Downloaded file too small (${size_kb}KB). Download may be incomplete."
        exit 1
    fi

    local size
    size=$(ls -lh "$dest" | awk '{print $5}')
    log_success "Downloaded ${filename} (${size})"
}

# Extract tarball (handles BusyBox tar on AD5M)
extract_release() {
    local platform=$1
    local tarball="${TMP_DIR}/helixscreen.tar.gz"

    log_info "Extracting release to ${INSTALL_DIR}..."

    # Check if install dir already exists
    if [ -d "${INSTALL_DIR}" ]; then
        ORIGINAL_INSTALL_EXISTS=true

        # Backup existing config (check new location first, then legacy)
        if [ -f "${INSTALL_DIR}/config/helixconfig.json" ]; then
            BACKUP_CONFIG="${TMP_DIR}/helixconfig.json.backup"
            cp "${INSTALL_DIR}/config/helixconfig.json" "$BACKUP_CONFIG"
            log_info "Backed up existing configuration (from config/)"
        elif [ -f "${INSTALL_DIR}/helixconfig.json" ]; then
            BACKUP_CONFIG="${TMP_DIR}/helixconfig.json.backup"
            cp "${INSTALL_DIR}/helixconfig.json" "$BACKUP_CONFIG"
            log_info "Backed up existing configuration (legacy location)"
        fi
    fi

    # Remove old installation
    $SUDO rm -rf "${INSTALL_DIR}"

    # Create parent directory
    $SUDO mkdir -p "$(dirname "${INSTALL_DIR}")"

    # Extract - AD5M and K1 use BusyBox tar which doesn't support -z
    cd "$(dirname "${INSTALL_DIR}")" || exit 1
    if [ "$platform" = "ad5m" ] || [ "$platform" = "k1" ]; then
        if ! gunzip -c "$tarball" | $SUDO tar xf -; then
            log_error "Failed to extract tarball."
            log_error "The archive may be corrupted."
            exit 1
        fi
    else
        if ! $SUDO tar -xzf "$tarball"; then
            log_error "Failed to extract tarball."
            log_error "The archive may be corrupted."
            exit 1
        fi
    fi

    # Verify extraction succeeded
    if [ ! -f "${INSTALL_DIR}/helix-screen" ]; then
        log_error "Extraction failed - helix-screen binary not found."
        log_error "Expected: ${INSTALL_DIR}/helix-screen"
        exit 1
    fi

    # Restore config to new location (config/helixconfig.json)
    if [ -n "$BACKUP_CONFIG" ] && [ -f "$BACKUP_CONFIG" ]; then
        $SUDO mkdir -p "${INSTALL_DIR}/config"
        $SUDO cp "$BACKUP_CONFIG" "${INSTALL_DIR}/config/helixconfig.json"
        log_info "Restored existing configuration to config/"
    fi

    log_success "Extracted to ${INSTALL_DIR}"
}
