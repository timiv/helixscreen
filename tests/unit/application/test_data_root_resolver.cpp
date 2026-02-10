// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_data_root_resolver.cpp
 * @brief Tests for data root resolution logic
 *
 * Verifies that the binary can correctly find its data root (the directory
 * containing ui_xml/) from various deployment layouts:
 *   - Dev builds:   /project/build/bin/helix-screen → /project
 *   - Deployed:     /home/pi/helixscreen/bin/helix-screen → /home/pi/helixscreen
 *   - Wrong CWD:    Binary launched from / but data root exists at exe parent
 */

#include "data_root_resolver.h"

#include <filesystem>
#include <fstream>
#include <unistd.h>

#include "../../catch_amalgamated.hpp"

namespace fs = std::filesystem;

/**
 * @brief Test fixture that creates temporary directory trees
 *
 * Builds realistic directory layouts (build/bin, bin, ui_xml) in a temp dir
 * and cleans up after each test.
 */
class DataRootFixture {
  protected:
    fs::path temp_root;

    DataRootFixture() {
        temp_root = fs::temp_directory_path() / ("test_data_root_" + std::to_string(getpid()));
        fs::remove_all(temp_root);
        fs::create_directories(temp_root);
    }

    ~DataRootFixture() {
        fs::remove_all(temp_root);
    }

    /// Create a simulated install directory with ui_xml/ and a binary path
    fs::path make_install_layout(const std::string& name, const std::string& bin_subdir) {
        fs::path install_dir = temp_root / name;
        fs::create_directories(install_dir / "ui_xml");
        fs::create_directories(install_dir / bin_subdir);
        return install_dir;
    }

    /// Create a directory WITHOUT ui_xml/ (invalid data root)
    fs::path make_invalid_layout(const std::string& name, const std::string& bin_subdir) {
        fs::path install_dir = temp_root / name;
        fs::create_directories(install_dir / bin_subdir);
        return install_dir;
    }
};

// ============================================================================
// is_valid_data_root
// ============================================================================

TEST_CASE_METHOD(DataRootFixture, "is_valid_data_root: directory with ui_xml/ is valid",
                 "[data_root][validation]") {
    auto dir = temp_root / "valid";
    fs::create_directories(dir / "ui_xml");

    REQUIRE(helix::is_valid_data_root(dir.string()));
}

TEST_CASE_METHOD(DataRootFixture, "is_valid_data_root: directory without ui_xml/ is invalid",
                 "[data_root][validation]") {
    auto dir = temp_root / "no_xml";
    fs::create_directories(dir);

    REQUIRE_FALSE(helix::is_valid_data_root(dir.string()));
}

TEST_CASE_METHOD(DataRootFixture, "is_valid_data_root: nonexistent directory is invalid",
                 "[data_root][validation]") {
    REQUIRE_FALSE(helix::is_valid_data_root("/nonexistent/path/that/does/not/exist"));
}

TEST_CASE_METHOD(DataRootFixture, "is_valid_data_root: empty string is invalid",
                 "[data_root][validation]") {
    REQUIRE_FALSE(helix::is_valid_data_root(""));
}

TEST_CASE_METHOD(DataRootFixture, "is_valid_data_root: ui_xml as file (not dir) is invalid",
                 "[data_root][validation]") {
    auto dir = temp_root / "file_not_dir";
    fs::create_directories(dir);
    // Create ui_xml as a regular file, not a directory
    std::ofstream(dir / "ui_xml") << "not a directory";

    REQUIRE_FALSE(helix::is_valid_data_root(dir.string()));
}

// ============================================================================
// resolve_data_root_from_exe — deployed layout (/bin)
// ============================================================================

TEST_CASE_METHOD(DataRootFixture, "resolve: deployed layout strips /bin from exe path",
                 "[data_root][resolve]") {
    // Simulates: /home/pi/helixscreen/bin/helix-screen
    auto install = make_install_layout("deployed", "bin");
    std::string exe = (install / "bin" / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    REQUIRE(result == install.string());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: deployed layout with different binary name",
                 "[data_root][resolve]") {
    auto install = make_install_layout("deployed2", "bin");
    std::string exe = (install / "bin" / "my-custom-binary").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    REQUIRE(result == install.string());
}

// ============================================================================
// resolve_data_root_from_exe — dev layout (/build/bin)
// ============================================================================

TEST_CASE_METHOD(DataRootFixture, "resolve: dev layout strips /build/bin from exe path",
                 "[data_root][resolve]") {
    // Simulates: /path/to/project/build/bin/helix-screen
    auto install = make_install_layout("devbuild", "build/bin");
    std::string exe = (install / "build" / "bin" / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    REQUIRE(result == install.string());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: /build/bin preferred over /bin when both valid",
                 "[data_root][resolve]") {
    // A dev project has both build/bin AND bin - /build/bin should win
    auto install = make_install_layout("both", "build/bin");
    fs::create_directories(install / "bin"); // also has /bin
    std::string exe = (install / "build" / "bin" / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    // Should resolve to the project root (stripping /build/bin)
    REQUIRE(result == install.string());
}

// ============================================================================
// resolve_data_root_from_exe — failure cases
// ============================================================================

TEST_CASE_METHOD(DataRootFixture, "resolve: returns empty when ui_xml missing",
                 "[data_root][resolve]") {
    // Binary exists in /bin but parent has no ui_xml/
    auto install = make_invalid_layout("no_assets", "bin");
    std::string exe = (install / "bin" / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    REQUIRE(result.empty());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: returns empty for empty path", "[data_root][resolve]") {
    REQUIRE(helix::resolve_data_root_from_exe("").empty());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: returns empty for path without slashes",
                 "[data_root][resolve]") {
    REQUIRE(helix::resolve_data_root_from_exe("helix-screen").empty());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: returns empty for unknown directory layout",
                 "[data_root][resolve]") {
    // Binary in /opt/weird/place/helix-screen — no /bin or /build/bin suffix
    auto dir = temp_root / "weird" / "place";
    fs::create_directories(dir);
    // Even if parent has ui_xml, path doesn't end in /bin or /build/bin
    fs::create_directories(temp_root / "weird" / "ui_xml");
    std::string exe = (dir / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    REQUIRE(result.empty());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: /bin suffix only matches at path boundary",
                 "[data_root][resolve]") {
    // Path like /home/pi/cabin/helix-screen should NOT match /bin
    auto dir = temp_root / "cabin";
    fs::create_directories(dir);
    fs::create_directories(temp_root / "ui_xml"); // parent is valid
    std::string exe = (dir / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    // "cabin" doesn't end with "/bin", so no match
    REQUIRE(result.empty());
}

TEST_CASE_METHOD(DataRootFixture, "resolve: deep nested deploy path works",
                 "[data_root][resolve]") {
    // /opt/printers/voron/helixscreen/bin/helix-screen
    auto install = make_install_layout("opt/printers/voron/helixscreen", "bin");
    std::string exe = (install / "bin" / "helix-screen").string();

    std::string result = helix::resolve_data_root_from_exe(exe);
    REQUIRE(result == install.string());
}
