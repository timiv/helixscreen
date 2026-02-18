// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>

namespace helix {

/// Result of asset extraction attempt
enum class AssetExtractionResult {
    EXTRACTED,       // Fresh extraction performed
    ALREADY_CURRENT, // Version matched, no extraction needed
    FAILED           // Extraction failed
};

/// Extract assets from source_dir to target_dir if version doesn't match.
/// Writes a VERSION file to target_dir on success.
/// The source_dir/target_dir abstraction lets us test without Android APIs.
AssetExtractionResult extract_assets_if_needed(const std::string& source_dir,
                                               const std::string& target_dir,
                                               const std::string& current_version);

/// Android-specific entry point that uses SDL paths.
/// Only callable on Android builds.
#ifdef __ANDROID__
void android_extract_assets_if_needed();
#endif

} // namespace helix
