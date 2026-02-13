// SPDX-License-Identifier: GPL-3.0-or-later
#include "android_asset_extractor.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace helix {

AssetExtractionResult extract_assets_if_needed(const std::string& source_dir,
                                               const std::string& target_dir,
                                               const std::string& current_version) {
    // Check if target already has matching version
    fs::path version_file = fs::path(target_dir) / "VERSION";
    if (fs::exists(version_file)) {
        std::ifstream ifs(version_file);
        std::string existing_version;
        std::getline(ifs, existing_version);
        if (existing_version == current_version) {
            spdlog::debug("Assets already at version {}, skipping extraction", current_version);
            return AssetExtractionResult::ALREADY_CURRENT;
        }
        spdlog::info("Asset version mismatch: have '{}', need '{}' - re-extracting",
                     existing_version, current_version);
    }

    // Create target directory if needed
    std::error_code ec;
    fs::create_directories(target_dir, ec);
    if (ec) {
        spdlog::error("Failed to create target directory '{}': {}", target_dir, ec.message());
        return AssetExtractionResult::FAILED;
    }

    // Verify source directory exists
    if (!fs::exists(source_dir) || !fs::is_directory(source_dir)) {
        spdlog::error("Source directory '{}' does not exist or is not a directory", source_dir);
        return AssetExtractionResult::FAILED;
    }

    // Copy all files recursively from source to target
    fs::copy(source_dir, target_dir,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
    if (ec) {
        spdlog::error("Failed to copy assets from '{}' to '{}': {}", source_dir, target_dir,
                      ec.message());
        return AssetExtractionResult::FAILED;
    }

    // Write version marker
    {
        std::ofstream ofs(version_file, std::ios::trunc);
        if (!ofs) {
            spdlog::error("Failed to write version marker to '{}'", version_file.string());
            return AssetExtractionResult::FAILED;
        }
        ofs << current_version;
    }

    spdlog::info("Extracted assets to '{}' (version {})", target_dir, current_version);
    return AssetExtractionResult::EXTRACTED;
}

#ifdef __ANDROID__

#include "helix_version.h"

#include <SDL.h>
#include <cstdlib>

void android_extract_assets_if_needed() {
    // Get Android internal storage path via SDL
    const char* internal_path = SDL_AndroidGetInternalStoragePath();
    if (!internal_path) {
        spdlog::error("[AndroidAssets] Could not get internal storage path from SDL");
        return;
    }

    std::string target_dir = std::string(internal_path) + "/data";
    spdlog::info("[AndroidAssets] Target directory: {}", target_dir);

    // On Android, assets are bundled in the APK and accessed via SDL_RWops.
    // For MVP, we use SDL_AndroidGetInternalStoragePath() as our data root.
    // The Gradle copyAssets task puts ui_xml/, assets/, config/ into the APK
    // assets directory. Android's AssetManager can read them, but LVGL and
    // our code expect filesystem paths. So we extract to internal storage.
    //
    // For the initial MVP, assets are extracted by the Gradle build task into
    // the APK's assets/ directory. We need to copy them from there to the
    // filesystem using SDL's Android file I/O.
    //
    // TODO: Implement SDL-based APK asset reading for full extraction.
    // For now, set HELIX_DATA_DIR to the internal storage path where
    // the app can find pre-deployed assets.

    auto result = extract_assets_if_needed(
        target_dir + "/source", // Placeholder — full APK extraction needs SDL_RWops
        target_dir, helix_version());

    if (result == AssetExtractionResult::FAILED) {
        spdlog::warn("[AndroidAssets] Asset extraction failed, app may not have UI resources");
    }

    // Set HELIX_DATA_DIR so ensure_project_root_cwd() finds it
    setenv("HELIX_DATA_DIR", target_dir.c_str(), 1);
    spdlog::info("[AndroidAssets] Set HELIX_DATA_DIR={}", target_dir);
}

#endif // __ANDROID__

} // namespace helix
