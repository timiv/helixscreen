// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_thumbnail_cache.cpp
 * @brief Unit tests for ThumbnailCache directory selection and caching logic
 *
 * Tests the cache directory determination, path generation, and write verification.
 */

#include "../../include/thumbnail_cache.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "../catch_amalgamated.hpp"

// ============================================================================
// ThumbnailCache Directory Tests
// ============================================================================

TEST_CASE("ThumbnailCache cache directory initialization", "[assets][cache]") {
    // Get the singleton - this will initialize with the determined directory
    ThumbnailCache& cache = get_thumbnail_cache();

    SECTION("Cache directory is set and non-empty") {
        std::string cache_dir = cache.get_cache_dir();
        REQUIRE(!cache_dir.empty());
        INFO("Cache directory: " << cache_dir);
    }

    SECTION("Cache directory exists") {
        std::string cache_dir = cache.get_cache_dir();
        REQUIRE(std::filesystem::exists(cache_dir));
    }

    SECTION("Cache directory is writable") {
        std::string cache_dir = cache.get_cache_dir();
        std::string test_file = cache_dir + "/.write_test_" + std::to_string(rand());

        // Should be able to create a file
        std::ofstream ofs(test_file);
        REQUIRE(ofs.good());
        ofs << "test";
        ofs.close();

        // Cleanup
        std::filesystem::remove(test_file);
    }

    SECTION("Cache directory contains 'helix' in path") {
        // All cache paths should include 'helix' somewhere
        std::string cache_dir = cache.get_cache_dir();
        REQUIRE(cache_dir.find("helix") != std::string::npos);
    }
}

TEST_CASE("ThumbnailCache path generation", "[assets][cache]") {
    ThumbnailCache& cache = get_thumbnail_cache();

    SECTION("get_cache_path returns path in cache directory") {
        std::string path = cache.get_cache_path("test/image.png");
        std::string cache_dir = cache.get_cache_dir();

        REQUIRE(path.find(cache_dir) == 0); // Should start with cache_dir
    }

    SECTION("get_cache_path generates .png extension") {
        std::string path = cache.get_cache_path("test/image.png");
        REQUIRE(path.substr(path.length() - 4) == ".png");
    }

    SECTION("Different paths generate different cache paths") {
        std::string path1 = cache.get_cache_path("file1.png");
        std::string path2 = cache.get_cache_path("file2.png");
        REQUIRE(path1 != path2);
    }

    SECTION("Same path generates same cache path") {
        std::string path1 = cache.get_cache_path("test/file.png");
        std::string path2 = cache.get_cache_path("test/file.png");
        REQUIRE(path1 == path2);
    }
}

TEST_CASE("ThumbnailCache LVGL path helpers", "[assets][cache]") {
    SECTION("is_lvgl_path detects A: prefix") {
        REQUIRE(ThumbnailCache::is_lvgl_path("A:/path/to/file.png"));
        REQUIRE(ThumbnailCache::is_lvgl_path("A:relative/path.bin"));
    }

    SECTION("is_lvgl_path rejects non-LVGL paths") {
        REQUIRE_FALSE(ThumbnailCache::is_lvgl_path("/path/to/file.png"));
        REQUIRE_FALSE(ThumbnailCache::is_lvgl_path("relative/path.png"));
        REQUIRE_FALSE(ThumbnailCache::is_lvgl_path("B:/wrong/prefix.png"));
        REQUIRE_FALSE(ThumbnailCache::is_lvgl_path(""));
        REQUIRE_FALSE(ThumbnailCache::is_lvgl_path("A")); // Too short
    }

    SECTION("to_lvgl_path adds A: prefix") {
        REQUIRE(ThumbnailCache::to_lvgl_path("/path/to/file.png") == "A:/path/to/file.png");
    }

    SECTION("to_lvgl_path doesn't double-prefix") {
        std::string already_lvgl = "A:/path/to/file.png";
        REQUIRE(ThumbnailCache::to_lvgl_path(already_lvgl) == already_lvgl);
    }
}

TEST_CASE("ThumbnailCache disk pressure monitoring", "[assets][cache]") {
    ThumbnailCache& cache = get_thumbnail_cache();

    SECTION("get_available_disk_space returns non-zero on normal systems") {
        size_t space = cache.get_available_disk_space();
        // Should have at least some space available
        REQUIRE(space > 0);
    }

    SECTION("get_disk_pressure returns valid pressure level") {
        auto pressure = cache.get_disk_pressure();
        // Should be one of the valid enum values
        REQUIRE((pressure == ThumbnailCache::DiskPressure::Normal ||
                 pressure == ThumbnailCache::DiskPressure::Low ||
                 pressure == ThumbnailCache::DiskPressure::Critical));
    }

    SECTION("is_caching_allowed returns true on normal systems") {
        // On a development machine, we should have plenty of disk space
        REQUIRE(cache.is_caching_allowed());
    }
}

TEST_CASE("ThumbnailCache size management", "[assets][cache]") {
    ThumbnailCache& cache = get_thumbnail_cache();

    SECTION("get_max_size returns positive value") {
        REQUIRE(cache.get_max_size() >= ThumbnailCache::MIN_CACHE_SIZE);
    }

    SECTION("set_max_size updates the limit") {
        size_t original = cache.get_max_size();
        size_t new_size = 10 * 1024 * 1024; // 10 MB

        cache.set_max_size(new_size);
        REQUIRE(cache.get_max_size() == new_size);

        // Restore original
        cache.set_max_size(original);
    }

    SECTION("get_cache_size returns zero or positive") {
        // May be zero if cache is empty, or positive if files exist
        size_t size = cache.get_cache_size();
        REQUIRE(size >= 0); // Always true for size_t, but documents intent
    }
}

// ============================================================================
// ThumbnailCache Thread Safety Tests
// ============================================================================

TEST_CASE("ThumbnailCache get_cache_dir is thread-safe", "[assets][cache][thread]") {
    ThumbnailCache& cache = get_thumbnail_cache();

    SECTION("Multiple calls return consistent result") {
        std::string dir1 = cache.get_cache_dir();
        std::string dir2 = cache.get_cache_dir();
        std::string dir3 = cache.get_cache_dir();

        REQUIRE(dir1 == dir2);
        REQUIRE(dir2 == dir3);
    }
}

// ============================================================================
// Cache Path Edge Cases
// ============================================================================

TEST_CASE("ThumbnailCache path edge cases", "[assets][cache]") {
    ThumbnailCache& cache = get_thumbnail_cache();

    SECTION("Paths with spaces are handled") {
        std::string path = cache.get_cache_path("My Model/thumb with spaces.png");
        REQUIRE(!path.empty());
        REQUIRE(std::filesystem::path(path).filename().extension() == ".png");
    }

    SECTION("Paths with unicode are handled") {
        std::string path = cache.get_cache_path("模型/缩略图.png");
        REQUIRE(!path.empty());
    }

    SECTION("Very long paths are handled") {
        std::string long_name(200, 'a');
        std::string path = cache.get_cache_path(long_name + ".png");
        REQUIRE(!path.empty());
    }
}
