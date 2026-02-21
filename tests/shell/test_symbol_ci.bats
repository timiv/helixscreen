#!/usr/bin/env bats
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tests for symbol extraction CI workflow
# Split from test_symbol_extraction.bats for parallel execution.

@test "release.yml build matrix includes all platforms" {
    local yml=".github/workflows/release.yml"
    [ -f "$yml" ]

    # Verify the build matrix includes all expected platforms
    for platform in pi pi32 ad5m k1 k2; do
        grep -q "platform: ${platform}" "$yml"
    done
}

@test "release.yml uploads symbol artifacts via matrix" {
    local yml=".github/workflows/release.yml"

    # Symbol upload uses matrix.platform variable, not literal platform names
    grep -q 'name: symbols-\${{ matrix.platform }}' "$yml"
    grep -q 'helix-screen.sym' "$yml"
}

@test "release.yml uploads symbol maps to R2" {
    local yml=".github/workflows/release.yml"
    grep -q 'symbols/v.*\.sym' "$yml"
}
