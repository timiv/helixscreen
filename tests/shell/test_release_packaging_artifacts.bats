#!/usr/bin/env bats
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tests for release packaging — gitignore, install.sh, LZ4 compression
# Split from test_release_packaging.bats for parallel execution.

load helpers

# ============================================================================
# Release targets call release-clean-assets
# ============================================================================

@test "all release targets call release-clean-assets" {
    # Count release targets that include the cleanup call
    local target_count cleanup_count
    target_count=$(grep -c '^release-.*:' mk/cross.mk | head -1)
    cleanup_count=$(grep -c 'release-clean-assets' mk/cross.mk)

    # At least 6 release targets should call it (pi, pi32, ad5m, k1, k1-dynamic, k2)
    # Plus the definition = 7 occurrences minimum
    [ "$cleanup_count" -ge 7 ]
}

# ============================================================================
# Source tree doesn't contain files that shouldn't be deployed
# ============================================================================

@test "CJK fonts are gitignored" {
    grep -q 'NotoSansCJK' .gitignore
}

# ============================================================================
# install.sh included in release packages
# ============================================================================

@test "all release targets include install.sh in package" {
    # Every release target must copy install.sh into the tarball root (helixscreen/).
    # This is critical: update_checker.cpp extracts helixscreen/INSTALLER_FILENAME
    # from the tarball to run the version-matched installer. Without it, updates
    # fail with "Installer not found".
    local targets=(release-pi release-pi32 release-ad5m release-k1 release-k1-dynamic release-k2 release-snapmaker-u1)
    local count
    count=$(grep -c 'cp scripts/\$(INSTALLER_FILENAME).*\$(RELEASE_DIR)' mk/cross.mk)
    # Must have one cp per release target
    [ "$count" -ge "${#targets[@]}" ]
}

@test "install.sh exists in scripts directory" {
    [ -f scripts/install.sh ]
}

@test "install.sh is executable" {
    [ -x scripts/install.sh ]
}

# ============================================================================
# LZ4 compression enabled
# ============================================================================

@test "LV_USE_LZ4_INTERNAL is enabled in lv_conf.h" {
    grep -q '#define LV_USE_LZ4_INTERNAL.*1' lv_conf.h
}

@test "image generation scripts use LZ4 compression" {
    grep -q '\-\-compress.*LZ4' scripts/lib/lvgl_image_lib.sh
}

@test "3D splash generation uses LZ4 compression" {
    grep -q '"--compress".*"LZ4"' scripts/gen_splash_3d.py
}
