// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_url_encoding.cpp
 * @brief Unit tests for URL encoding of thumbnail paths
 *
 * Tests that thumbnail paths with special characters (spaces, etc.)
 * are properly URL-encoded for HTTP requests.
 */

#include "hv/hurl.h"

#include "../catch_amalgamated.hpp"

// ============================================================================
// URL Encoding Tests
// ============================================================================

TEST_CASE("HUrl::escape encodes special characters", "[api][encoding]") {
    SECTION("Encodes spaces as %20") {
        std::string path = ".thumbs/Gridfinity bin 3x2x9-300x300.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/Gridfinity%20bin%203x2x9-300x300.png");
    }

    SECTION("Preserves safe characters") {
        std::string path = ".thumbs/simple-file_name.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == path); // No encoding needed
    }

    SECTION("Preserves forward slashes in paths") {
        std::string path = ".thumbs/subdir/file.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/subdir/file.png");
    }

    SECTION("Encodes parentheses") {
        std::string path = ".thumbs/file (copy).png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/file%20%28copy%29.png");
    }

    SECTION("Encodes multiple special characters") {
        std::string path = ".thumbs/My File #1 (v2).png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/My%20File%20%231%20%28v2%29.png");
    }

    SECTION("Handles empty string") {
        std::string path = "";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded.empty());
    }

    SECTION("Encodes plus sign") {
        std::string path = ".thumbs/file+name.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/file%2Bname.png");
    }

    SECTION("Encodes ampersand") {
        std::string path = ".thumbs/file&name.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/file%26name.png");
    }

    SECTION("Preserves alphanumeric characters") {
        std::string path = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == path);
    }
}

// ============================================================================
// Realistic Thumbnail Path Tests
// ============================================================================

TEST_CASE("Thumbnail path encoding for Moonraker URLs", "[api][assets]") {
    SECTION("Real-world Gridfinity filename with spaces") {
        // This is the actual problematic filename from user testing
        std::string path = ".thumbs/Gridfinity bin 3x2x9-300x300.png";
        std::string encoded = HUrl::escape(path, "/.-_");

        // Should be usable in URL without HTTP 400 error
        REQUIRE(encoded.find(' ') == std::string::npos);   // No raw spaces
        REQUIRE(encoded.find("%20") != std::string::npos); // Spaces are encoded
    }

    SECTION("Typical PrusaSlicer thumbnail path") {
        std::string path = ".thumbs/benchy_0.2mm_PLA_MK3S_1h30m-300x300.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == path); // No special chars to encode
    }

    SECTION("Path with subdirectory") {
        std::string path = ".thumbs/models/My Model-300x300.png";
        std::string encoded = HUrl::escape(path, "/.-_");
        REQUIRE(encoded == ".thumbs/models/My%20Model-300x300.png");
    }
}
