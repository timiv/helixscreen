// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file test_android_asset_extractor.cpp
 * @brief Unit tests for the Android asset extraction logic
 *
 * Tests the platform-agnostic extract_assets_if_needed() function using
 * temporary directories. The function copies assets from a source directory
 * to a target directory, with a VERSION marker file for cache invalidation.
 */

#include "android_asset_extractor.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "../catch_amalgamated.hpp"

namespace fs = std::filesystem;
using namespace helix;

// ============================================================================
// RAII temp directory helper
// ============================================================================

class TempDir {
  public:
    TempDir(const std::string& prefix) {
        path_ = fs::temp_directory_path() / (prefix + "_" + std::to_string(counter_++));
        fs::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    // Non-copyable
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const fs::path& path() const {
        return path_;
    }
    std::string str() const {
        return path_.string();
    }

  private:
    fs::path path_;
    static inline int counter_ = 0;
};

// Helper to write a file with content
static void write_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream ofs(path, std::ios::trunc);
    ofs << content;
}

// Helper to read a file's content
static std::string read_file(const fs::path& path) {
    std::ifstream ifs(path);
    std::string content;
    std::getline(ifs, content);
    return content;
}

// ============================================================================
// Extraction Tests
// ============================================================================

TEST_CASE("Extracts files from source to target directory", "[android][asset]") {
    TempDir source("asset_src");
    TempDir target("asset_tgt");

    // Remove target so extractor creates it
    fs::remove_all(target.path());

    // Create source files
    write_file(source.path() / "config.json", R"({"key": "value"})");
    write_file(source.path() / "ui_xml" / "main.xml", "<root/>");

    auto result = extract_assets_if_needed(source.str(), target.str(), "1.0.0");

    REQUIRE(result == AssetExtractionResult::EXTRACTED);
    REQUIRE(fs::exists(target.path() / "config.json"));
    REQUIRE(fs::exists(target.path() / "ui_xml" / "main.xml"));
    REQUIRE(read_file(target.path() / "config.json") == R"({"key": "value"})");
    REQUIRE(read_file(target.path() / "ui_xml" / "main.xml") == "<root/>");
}

TEST_CASE("Skips extraction if VERSION marker matches current version", "[android][asset]") {
    TempDir source("asset_src");
    TempDir target("asset_tgt");

    // Create source files
    write_file(source.path() / "data.txt", "original");

    // Pre-populate target with matching version
    write_file(target.path() / "VERSION", "2.0.0");
    write_file(target.path() / "data.txt", "old content");

    auto result = extract_assets_if_needed(source.str(), target.str(), "2.0.0");

    REQUIRE(result == AssetExtractionResult::ALREADY_CURRENT);
    // Target content should be unchanged (old content, not re-extracted)
    REQUIRE(read_file(target.path() / "data.txt") == "old content");
}

TEST_CASE("Re-extracts if VERSION marker differs", "[android][asset]") {
    TempDir source("asset_src");
    TempDir target("asset_tgt");

    // Create source files with new content
    write_file(source.path() / "data.txt", "new content");

    // Pre-populate target with old version
    write_file(target.path() / "VERSION", "1.0.0");
    write_file(target.path() / "data.txt", "old content");

    auto result = extract_assets_if_needed(source.str(), target.str(), "2.0.0");

    REQUIRE(result == AssetExtractionResult::EXTRACTED);
    REQUIRE(read_file(target.path() / "data.txt") == "new content");
    REQUIRE(read_file(target.path() / "VERSION") == "2.0.0");
}

TEST_CASE("Creates target directory if it does not exist", "[android][asset]") {
    TempDir source("asset_src");
    TempDir parent("asset_parent");

    // Target is a subdirectory that does not exist yet
    fs::path target_path = parent.path() / "nested" / "target";

    write_file(source.path() / "file.txt", "hello");

    auto result = extract_assets_if_needed(source.str(), target_path.string(), "1.0.0");

    REQUIRE(result == AssetExtractionResult::EXTRACTED);
    REQUIRE(fs::exists(target_path / "file.txt"));
    REQUIRE(read_file(target_path / "file.txt") == "hello");
}

TEST_CASE("Missing VERSION marker triggers re-extraction", "[android][asset]") {
    TempDir source("asset_src");
    TempDir target("asset_tgt");

    // Create source and target with content but no VERSION file
    write_file(source.path() / "data.txt", "fresh");
    write_file(target.path() / "data.txt", "stale");

    // No VERSION file in target = treat as needing extraction
    auto result = extract_assets_if_needed(source.str(), target.str(), "1.0.0");

    REQUIRE(result == AssetExtractionResult::EXTRACTED);
    REQUIRE(read_file(target.path() / "data.txt") == "fresh");
    REQUIRE(read_file(target.path() / "VERSION") == "1.0.0");
}

TEST_CASE("Writes correct version marker after extraction", "[android][asset]") {
    TempDir source("asset_src");
    TempDir target("asset_tgt");

    write_file(source.path() / "dummy.txt", "x");

    auto result = extract_assets_if_needed(source.str(), target.str(), "3.14.159");

    REQUIRE(result == AssetExtractionResult::EXTRACTED);
    REQUIRE(read_file(target.path() / "VERSION") == "3.14.159");
}

TEST_CASE("Preserves directory structure during extraction", "[android][asset]") {
    TempDir source("asset_src");
    TempDir target("asset_tgt");

    // Remove target so it is freshly created
    fs::remove_all(target.path());

    // Create nested structure
    write_file(source.path() / "a" / "b" / "c.txt", "deep");
    write_file(source.path() / "a" / "sibling.txt", "side");
    write_file(source.path() / "top.txt", "top");

    auto result = extract_assets_if_needed(source.str(), target.str(), "1.0.0");

    REQUIRE(result == AssetExtractionResult::EXTRACTED);
    REQUIRE(fs::exists(target.path() / "a" / "b" / "c.txt"));
    REQUIRE(fs::exists(target.path() / "a" / "sibling.txt"));
    REQUIRE(fs::exists(target.path() / "top.txt"));
    REQUIRE(read_file(target.path() / "a" / "b" / "c.txt") == "deep");
}

TEST_CASE("Returns FAILED when source directory does not exist", "[android][asset]") {
    TempDir target("asset_tgt");

    auto result = extract_assets_if_needed("/nonexistent/source/dir", target.str(), "1.0.0");

    REQUIRE(result == AssetExtractionResult::FAILED);
}
