#!/usr/bin/env bats
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tests for setup_config_symlink() in platform.sh
# Verifies symlink creation from printer_data/config/helixscreen → INSTALL_DIR/config
# and graceful handling of all edge cases.

WORKTREE_ROOT="$(cd "$BATS_TEST_DIRNAME/../.." && pwd)"

setup() {
    load helpers

    # Reset globals before each test
    KLIPPER_USER=""
    KLIPPER_HOME=""
    INIT_SCRIPT_DEST=""
    PREVIOUS_UI_SCRIPT=""
    AD5M_FIRMWARE=""
    K1_FIRMWARE=""
    INSTALL_DIR="/opt/helixscreen"
    TMP_DIR="/tmp/helixscreen-install"
    _USER_INSTALL_DIR=""
    SUDO=""

    # Source platform.sh (skip source guard by unsetting it)
    unset _HELIX_PLATFORM_SOURCED
    . "$WORKTREE_ROOT/scripts/lib/installer/platform.sh"
}

# --- Happy path ---

@test "creates symlink when printer_data/config and install config both exist" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$KLIPPER_HOME/printer_data/config"
    mkdir -p "$INSTALL_DIR/config"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    [ -L "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ "$(readlink "$KLIPPER_HOME/printer_data/config/helixscreen")" = "$INSTALL_DIR/config" ]
}

# --- Graceful skip conditions ---

@test "skips when KLIPPER_HOME is empty" {
    KLIPPER_HOME=""
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$INSTALL_DIR/config"

    run setup_config_symlink
    [ "$status" -eq 0 ]
}

@test "skips when INSTALL_DIR is empty" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR=""
    mkdir -p "$KLIPPER_HOME/printer_data/config"

    run setup_config_symlink
    [ "$status" -eq 0 ]
}

@test "skips when printer_data/config does not exist" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    # Create KLIPPER_HOME but NOT printer_data/config
    mkdir -p "$KLIPPER_HOME"
    mkdir -p "$INSTALL_DIR/config"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    [ ! -e "$KLIPPER_HOME/printer_data/config/helixscreen" ]
}

@test "skips when install config directory does not exist" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$KLIPPER_HOME/printer_data/config"
    # Create INSTALL_DIR but NOT config subdir
    mkdir -p "$INSTALL_DIR"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    [ ! -e "$KLIPPER_HOME/printer_data/config/helixscreen" ]
}

# --- Idempotency ---

@test "no-op when correct symlink already exists" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$KLIPPER_HOME/printer_data/config"
    mkdir -p "$INSTALL_DIR/config"
    # Pre-create the correct symlink
    ln -s "$INSTALL_DIR/config" "$KLIPPER_HOME/printer_data/config/helixscreen"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    [ -L "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ "$(readlink "$KLIPPER_HOME/printer_data/config/helixscreen")" = "$INSTALL_DIR/config" ]
}

@test "updates symlink when pointing to wrong target" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$KLIPPER_HOME/printer_data/config"
    mkdir -p "$INSTALL_DIR/config"
    # Create symlink pointing to wrong place
    ln -s "/old/wrong/path" "$KLIPPER_HOME/printer_data/config/helixscreen"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    [ -L "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ "$(readlink "$KLIPPER_HOME/printer_data/config/helixscreen")" = "$INSTALL_DIR/config" ]
}

# --- Safety: don't destroy existing data ---

@test "refuses to overwrite existing regular file" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$KLIPPER_HOME/printer_data/config"
    mkdir -p "$INSTALL_DIR/config"
    # Create a regular file where symlink would go
    echo "user data" > "$KLIPPER_HOME/printer_data/config/helixscreen"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    # Should NOT be a symlink — original file preserved
    [ ! -L "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ -f "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ "$(cat "$KLIPPER_HOME/printer_data/config/helixscreen")" = "user data" ]
}

@test "refuses to overwrite existing directory" {
    KLIPPER_HOME="$BATS_TEST_TMPDIR/home/pi"
    INSTALL_DIR="$BATS_TEST_TMPDIR/helixscreen"
    mkdir -p "$KLIPPER_HOME/printer_data/config"
    mkdir -p "$INSTALL_DIR/config"
    # Create a real directory where symlink would go (user copied files manually)
    mkdir -p "$KLIPPER_HOME/printer_data/config/helixscreen"
    echo "precious" > "$KLIPPER_HOME/printer_data/config/helixscreen/myconfig.json"

    run setup_config_symlink
    [ "$status" -eq 0 ]
    # Should NOT be a symlink — original directory preserved
    [ ! -L "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ -d "$KLIPPER_HOME/printer_data/config/helixscreen" ]
    [ -f "$KLIPPER_HOME/printer_data/config/helixscreen/myconfig.json" ]
}

# --- Bundled installer parity ---

@test "bundled install.sh has setup_config_symlink function" {
    grep -q 'setup_config_symlink()' "$WORKTREE_ROOT/scripts/install.sh"
}

@test "bundled install.sh calls setup_config_symlink in main flow" {
    grep -q 'setup_config_symlink' "$WORKTREE_ROOT/scripts/install.sh"
}
