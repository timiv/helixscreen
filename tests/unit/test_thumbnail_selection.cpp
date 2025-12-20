// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_thumbnail_selection.cpp
 * @brief Unit tests for ThumbnailInfo and FileMetadata::get_largest_thumbnail()
 *
 * Tests the thumbnail selection logic that picks the largest available
 * thumbnail by pixel count for best display quality.
 */

#include "../../include/moonraker_api.h"

#include "../catch_amalgamated.hpp"

// ============================================================================
// ThumbnailInfo Tests
// ============================================================================

TEST_CASE("ThumbnailInfo pixel_count calculation", "[assets]") {
    SECTION("Calculates correct pixel count for standard dimensions") {
        ThumbnailInfo info;
        info.width = 300;
        info.height = 300;
        REQUIRE(info.pixel_count() == 90000);
    }

    SECTION("Calculates correct pixel count for rectangular thumbnails") {
        ThumbnailInfo info;
        info.width = 400;
        info.height = 300;
        REQUIRE(info.pixel_count() == 120000);
    }

    SECTION("Returns zero for uninitialized thumbnail") {
        ThumbnailInfo info;
        REQUIRE(info.pixel_count() == 0);
    }

    SECTION("Handles small thumbnails") {
        ThumbnailInfo info;
        info.width = 32;
        info.height = 32;
        REQUIRE(info.pixel_count() == 1024);
    }
}

// ============================================================================
// FileMetadata::get_largest_thumbnail Tests
// ============================================================================

TEST_CASE("FileMetadata get_largest_thumbnail", "[assets]") {
    SECTION("Returns empty string when no thumbnails") {
        FileMetadata metadata;
        REQUIRE(metadata.get_largest_thumbnail().empty());
    }

    SECTION("Returns only thumbnail when one available") {
        FileMetadata metadata;
        ThumbnailInfo thumb;
        thumb.relative_path = ".thumbnails/test-300x300.png";
        thumb.width = 300;
        thumb.height = 300;
        metadata.thumbnails.push_back(thumb);

        REQUIRE(metadata.get_largest_thumbnail() == ".thumbnails/test-300x300.png");
    }

    SECTION("Selects largest thumbnail by pixel count") {
        FileMetadata metadata;

        // Small thumbnail (32x32 = 1024 pixels)
        ThumbnailInfo small;
        small.relative_path = ".thumbnails/test-32x32.png";
        small.width = 32;
        small.height = 32;
        metadata.thumbnails.push_back(small);

        // Medium thumbnail (150x150 = 22500 pixels)
        ThumbnailInfo medium;
        medium.relative_path = ".thumbnails/test-150x150.png";
        medium.width = 150;
        medium.height = 150;
        metadata.thumbnails.push_back(medium);

        // Large thumbnail (300x300 = 90000 pixels)
        ThumbnailInfo large;
        large.relative_path = ".thumbnails/test-300x300.png";
        large.width = 300;
        large.height = 300;
        metadata.thumbnails.push_back(large);

        REQUIRE(metadata.get_largest_thumbnail() == ".thumbnails/test-300x300.png");
    }

    SECTION("Handles thumbnails in any order") {
        FileMetadata metadata;

        // Add largest first
        ThumbnailInfo large;
        large.relative_path = ".thumbnails/test-300x300.png";
        large.width = 300;
        large.height = 300;
        metadata.thumbnails.push_back(large);

        // Add smallest last
        ThumbnailInfo small;
        small.relative_path = ".thumbnails/test-32x32.png";
        small.width = 32;
        small.height = 32;
        metadata.thumbnails.push_back(small);

        REQUIRE(metadata.get_largest_thumbnail() == ".thumbnails/test-300x300.png");
    }

    SECTION("Handles rectangular thumbnails correctly") {
        FileMetadata metadata;

        // 400x300 = 120000 pixels
        ThumbnailInfo rect;
        rect.relative_path = ".thumbnails/test-400x300.png";
        rect.width = 400;
        rect.height = 300;
        metadata.thumbnails.push_back(rect);

        // 300x300 = 90000 pixels (smaller even though same height)
        ThumbnailInfo square;
        square.relative_path = ".thumbnails/test-300x300.png";
        square.width = 300;
        square.height = 300;
        metadata.thumbnails.push_back(square);

        REQUIRE(metadata.get_largest_thumbnail() == ".thumbnails/test-400x300.png");
    }

    SECTION("Falls back to first thumbnail when dimensions are zero") {
        FileMetadata metadata;

        // First thumbnail with no dimensions
        ThumbnailInfo first;
        first.relative_path = ".thumbnails/test-first.png";
        first.width = 0;
        first.height = 0;
        metadata.thumbnails.push_back(first);

        // Second thumbnail with no dimensions
        ThumbnailInfo second;
        second.relative_path = ".thumbnails/test-second.png";
        second.width = 0;
        second.height = 0;
        metadata.thumbnails.push_back(second);

        // When all have 0 pixels, returns first (stable selection)
        REQUIRE(metadata.get_largest_thumbnail() == ".thumbnails/test-first.png");
    }

    SECTION("Prefers thumbnail with dimensions over ones without") {
        FileMetadata metadata;

        // Thumbnail without dimensions
        ThumbnailInfo no_dims;
        no_dims.relative_path = ".thumbnails/test-unknown.png";
        no_dims.width = 0;
        no_dims.height = 0;
        metadata.thumbnails.push_back(no_dims);

        // Thumbnail with dimensions
        ThumbnailInfo with_dims;
        with_dims.relative_path = ".thumbnails/test-300x300.png";
        with_dims.width = 300;
        with_dims.height = 300;
        metadata.thumbnails.push_back(with_dims);

        REQUIRE(metadata.get_largest_thumbnail() == ".thumbnails/test-300x300.png");
    }
}

// ============================================================================
// ThumbnailProcessor Resolution Target Tests
// ============================================================================

#include "../../include/thumbnail_processor.h"

using helix::ThumbnailProcessor;
using helix::ThumbnailTarget;

TEST_CASE("ThumbnailProcessor breakpoint selection", "[assets][processor]") {
    SECTION("SMALL breakpoint: 480x320 → 120x120") {
        auto target = ThumbnailProcessor::get_target_for_resolution(480, 320);
        REQUIRE(target.width == 120);
        REQUIRE(target.height == 120);
    }

    SECTION("SMALL breakpoint: 320x480 (portrait) → 120x120") {
        auto target = ThumbnailProcessor::get_target_for_resolution(320, 480);
        REQUIRE(target.width == 120);
        REQUIRE(target.height == 120);
    }

    SECTION("MEDIUM breakpoint: 800x480 (AD5M) → 160x160") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, 480);
        REQUIRE(target.width == 160);
        REQUIRE(target.height == 160);
    }

    SECTION("MEDIUM breakpoint: 640x480 → 160x160") {
        auto target = ThumbnailProcessor::get_target_for_resolution(640, 480);
        REQUIRE(target.width == 160);
        REQUIRE(target.height == 160);
    }

    SECTION("LARGE breakpoint: 1024x600 → 220x220") {
        auto target = ThumbnailProcessor::get_target_for_resolution(1024, 600);
        REQUIRE(target.width == 220);
        REQUIRE(target.height == 220);
    }

    SECTION("LARGE breakpoint: 1280x720 → 220x220") {
        auto target = ThumbnailProcessor::get_target_for_resolution(1280, 720);
        REQUIRE(target.width == 220);
        REQUIRE(target.height == 220);
    }

    SECTION("Boundary: exactly 480px → SMALL") {
        auto target = ThumbnailProcessor::get_target_for_resolution(480, 320);
        REQUIRE(target.width == 120);
    }

    SECTION("Boundary: 481px → MEDIUM") {
        auto target = ThumbnailProcessor::get_target_for_resolution(481, 320);
        REQUIRE(target.width == 160);
    }

    SECTION("Boundary: exactly 800px → MEDIUM") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, 600);
        REQUIRE(target.width == 160);
    }

    SECTION("Boundary: 801px → LARGE") {
        auto target = ThumbnailProcessor::get_target_for_resolution(801, 600);
        REQUIRE(target.width == 220);
    }
}

TEST_CASE("ThumbnailProcessor color format selection", "[assets][processor]") {
    SECTION("Default is ARGB8888 (0x10)") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, 480);
        REQUIRE(target.color_format == 0x10);
    }

    SECTION("RGB565 when requested (0x12)") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, 480, true);
        REQUIRE(target.color_format == 0x12);
    }

    SECTION("ARGB8888 when explicitly disabled") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, 480, false);
        REQUIRE(target.color_format == 0x10);
    }
}

TEST_CASE("ThumbnailProcessor uses max(width, height) for breakpoint", "[assets][processor]") {
    SECTION("Portrait 600x1024 uses 1024 → LARGE") {
        auto target = ThumbnailProcessor::get_target_for_resolution(600, 1024);
        REQUIRE(target.width == 220);
    }

    SECTION("Landscape 1024x600 uses 1024 → LARGE") {
        auto target = ThumbnailProcessor::get_target_for_resolution(1024, 600);
        REQUIRE(target.width == 220);
    }

    SECTION("Square 800x800 uses 800 → MEDIUM") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, 800);
        REQUIRE(target.width == 160);
    }
}

TEST_CASE("ThumbnailProcessor edge cases", "[assets][processor]") {
    SECTION("Zero dimensions → SMALL fallback") {
        auto target = ThumbnailProcessor::get_target_for_resolution(0, 0);
        REQUIRE(target.width == 120);
        REQUIRE(target.height == 120);
    }

    SECTION("Negative width → SMALL fallback") {
        auto target = ThumbnailProcessor::get_target_for_resolution(-100, 480);
        REQUIRE(target.width == 120);
    }

    SECTION("Negative height → SMALL fallback") {
        auto target = ThumbnailProcessor::get_target_for_resolution(800, -1);
        REQUIRE(target.width == 120);
    }

    SECTION("Very large display (4K) → LARGE") {
        auto target = ThumbnailProcessor::get_target_for_resolution(3840, 2160);
        REQUIRE(target.width == 220);
    }

    SECTION("Zero dimensions preserve color format choice") {
        auto target_argb = ThumbnailProcessor::get_target_for_resolution(0, 0, false);
        REQUIRE(target_argb.color_format == 0x10);

        auto target_rgb = ThumbnailProcessor::get_target_for_resolution(0, 0, true);
        REQUIRE(target_rgb.color_format == 0x12);
    }
}
