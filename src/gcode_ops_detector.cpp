// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gcode_ops_detector.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace helix {
namespace gcode {

// ============================================================================
// DetectedOperation implementation
// ============================================================================

std::string DetectedOperation::display_name() const {
    switch (type) {
    case OperationType::BED_LEVELING:
        return "Bed Leveling";
    case OperationType::QGL:
        return "Quad Gantry Level";
    case OperationType::Z_TILT:
        return "Z Tilt Adjust";
    case OperationType::NOZZLE_CLEAN:
        return "Nozzle Cleaning";
    case OperationType::HOMING:
        return "Homing";
    case OperationType::CHAMBER_SOAK:
        return "Chamber Soak";
    case OperationType::PURGE_LINE:
        return "Purge Line";
    case OperationType::START_PRINT:
        return "Start Print";
    }
    return "Unknown";
}

// ============================================================================
// PrintStartCallInfo implementation
// ============================================================================

std::string PrintStartCallInfo::with_skip_params(
    const std::vector<std::pair<std::string, std::string>>& skip_params) const {
    if (!found || skip_params.empty()) {
        return raw_line;
    }

    // Start with the original line, trimmed of trailing whitespace/newlines
    std::string modified = raw_line;
    while (!modified.empty() && (modified.back() == '\n' || modified.back() == '\r' ||
                                 modified.back() == ' ' || modified.back() == '\t')) {
        modified.pop_back();
    }

    // Append skip parameters (validated for safe characters)
    for (const auto& [param_name, param_value] : skip_params) {
        // Validate param_name contains only safe characters (A-Z, 0-9, _)
        // This prevents injection of malformed parameters
        bool valid_name =
            !param_name.empty() && std::all_of(param_name.begin(), param_name.end(), [](char c) {
                return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
            });
        if (!valid_name) {
            spdlog::warn("[PrintStartCallInfo] Skipping invalid param name: {}", param_name);
            continue;
        }

        modified += " ";
        modified += param_name;
        modified += "=";
        modified += param_value;
    }

    spdlog::debug("[PrintStartCallInfo] Modified line: {}... -> {}...",
                  raw_line.substr(0, std::min<size_t>(raw_line.size(), 50)),
                  modified.substr(0, std::min<size_t>(modified.size(), 80)));

    return modified;
}

// ============================================================================
// ScanResult implementation
// ============================================================================

bool ScanResult::has_operation(OperationType type) const {
    return std::any_of(operations.begin(), operations.end(),
                       [type](const DetectedOperation& op) { return op.type == type; });
}

std::optional<DetectedOperation> ScanResult::get_operation(OperationType type) const {
    auto it = std::find_if(operations.begin(), operations.end(),
                           [type](const DetectedOperation& op) { return op.type == type; });
    if (it != operations.end()) {
        return *it;
    }
    return std::nullopt;
}

std::vector<DetectedOperation> ScanResult::get_operations(OperationType type) const {
    std::vector<DetectedOperation> result;
    std::copy_if(operations.begin(), operations.end(), std::back_inserter(result),
                 [type](const DetectedOperation& op) { return op.type == type; });
    return result;
}

// ============================================================================
// GCodeOpsDetector implementation
// ============================================================================

GCodeOpsDetector::GCodeOpsDetector(const DetectionConfig& config) : config_(config) {
    init_default_patterns();
}

std::string GCodeOpsDetector::operation_type_name(OperationType type) {
    switch (type) {
    case OperationType::BED_LEVELING:
        return "bed_leveling";
    case OperationType::QGL:
        return "qgl";
    case OperationType::Z_TILT:
        return "z_tilt";
    case OperationType::NOZZLE_CLEAN:
        return "nozzle_clean";
    case OperationType::HOMING:
        return "homing";
    case OperationType::CHAMBER_SOAK:
        return "chamber_soak";
    case OperationType::PURGE_LINE:
        return "purge_line";
    case OperationType::START_PRINT:
        return "start_print";
    }
    return "unknown";
}

void GCodeOpsDetector::init_default_patterns() {
    // ========================================================================
    // Bed Leveling patterns
    // ========================================================================

    // Direct commands
    patterns_.push_back({OperationType::BED_LEVELING, "BED_MESH_CALIBRATE",
                         OperationEmbedding::DIRECT_COMMAND, false});
    patterns_.push_back({OperationType::BED_LEVELING, "G29", OperationEmbedding::DIRECT_COMMAND,
                         true}); // Case sensitive for G-codes
    patterns_.push_back({OperationType::BED_LEVELING, "BED_MESH_PROFILE LOAD",
                         OperationEmbedding::DIRECT_COMMAND, false});

    // Macro calls
    patterns_.push_back(
        {OperationType::BED_LEVELING, "AUTO_BED_MESH", OperationEmbedding::MACRO_CALL, false});

    // ========================================================================
    // Quad Gantry Level patterns
    // ========================================================================

    patterns_.push_back(
        {OperationType::QGL, "QUAD_GANTRY_LEVEL", OperationEmbedding::DIRECT_COMMAND, false});
    patterns_.push_back({OperationType::QGL, "QGL", OperationEmbedding::MACRO_CALL, false});

    // ========================================================================
    // Z Tilt patterns
    // ========================================================================

    patterns_.push_back(
        {OperationType::Z_TILT, "Z_TILT_ADJUST", OperationEmbedding::DIRECT_COMMAND, false});
    patterns_.push_back({OperationType::Z_TILT, "Z_TILT", OperationEmbedding::MACRO_CALL, false});

    // ========================================================================
    // Nozzle cleaning patterns
    // ========================================================================

    patterns_.push_back(
        {OperationType::NOZZLE_CLEAN, "CLEAN_NOZZLE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::NOZZLE_CLEAN, "NOZZLE_WIPE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::NOZZLE_CLEAN, "WIPE_NOZZLE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::NOZZLE_CLEAN, "BRUSH_NOZZLE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::NOZZLE_CLEAN, "NOZZLE_BRUSH", OperationEmbedding::MACRO_CALL, false});

    // ========================================================================
    // Homing patterns
    // ========================================================================

    patterns_.push_back({OperationType::HOMING, "G28", OperationEmbedding::DIRECT_COMMAND, true});
    patterns_.push_back(
        {OperationType::HOMING, "SAFE_HOME", OperationEmbedding::MACRO_CALL, false});

    // ========================================================================
    // Chamber soak patterns
    // ========================================================================

    patterns_.push_back(
        {OperationType::CHAMBER_SOAK, "HEAT_SOAK", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::CHAMBER_SOAK, "CHAMBER_SOAK", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back({OperationType::CHAMBER_SOAK, "SET_HEATER_TEMPERATURE HEATER=chamber",
                         OperationEmbedding::DIRECT_COMMAND, false});

    // ========================================================================
    // Purge line patterns
    // ========================================================================

    patterns_.push_back(
        {OperationType::PURGE_LINE, "PURGE_LINE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::PURGE_LINE, "PRIME_LINE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::PURGE_LINE, "PRIME_NOZZLE", OperationEmbedding::MACRO_CALL, false});
    patterns_.push_back(
        {OperationType::PURGE_LINE, "INTRO_LINE", OperationEmbedding::MACRO_CALL, false});
}

void GCodeOpsDetector::add_pattern(OperationPattern pattern) {
    patterns_.push_back(std::move(pattern));
}

ScanResult GCodeOpsDetector::scan_file(const std::filesystem::path& filepath) const {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::warn("[GCodeOpsDetector] Failed to open file: {}", filepath.string());
        return {};
    }

    spdlog::debug("[GCodeOpsDetector] Scanning file: {}", filepath.string());
    return scan_stream(file);
}

ScanResult GCodeOpsDetector::scan_content(const std::string& content) const {
    std::istringstream stream(content);
    return scan_stream(stream);
}

ScanResult GCodeOpsDetector::scan_stream(std::istream& stream) const {
    ScanResult result;
    std::string line;
    size_t line_number = 0;
    size_t byte_offset = 0;

    while (std::getline(stream, line)) {
        line_number++;

        // Check limits
        if (byte_offset >= config_.max_scan_bytes) {
            spdlog::debug("[GCodeOpsDetector] Reached byte limit at {} bytes", byte_offset);
            result.reached_limit = true;
            break;
        }

        if (static_cast<int>(line_number) > config_.max_scan_lines) {
            spdlog::debug("[GCodeOpsDetector] Reached line limit at line {}", line_number);
            result.reached_limit = true;
            break;
        }

        // Check for first extrusion (stop scanning)
        if (config_.stop_at_first_extrusion && is_first_extrusion(line)) {
            spdlog::debug("[GCodeOpsDetector] First extrusion at line {}, stopping", line_number);
            break;
        }

        // Check for layer marker (stop scanning)
        if (config_.stop_at_layer_marker && is_layer_marker(line)) {
            spdlog::debug("[GCodeOpsDetector] Layer marker at line {}, stopping", line_number);
            break;
        }

        // Skip comment-only lines and empty lines (but still track byte offset)
        if (!line.empty() && line[0] != ';') {
            // Check for PRINT_START or START_PRINT macro call (case-insensitive)
            std::string upper_line = line;
            std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(), ::toupper);

            // Capture the PRINT_START call info (first occurrence only)
            if (!result.print_start.found) {
                // Look for PRINT_START (more common) or START_PRINT
                size_t ps_pos = upper_line.find("PRINT_START");
                size_t sp_pos = upper_line.find("START_PRINT");

                if (ps_pos != std::string::npos || sp_pos != std::string::npos) {
                    result.print_start.found = true;
                    result.print_start.macro_name =
                        (ps_pos != std::string::npos) ? "PRINT_START" : "START_PRINT";
                    result.print_start.raw_line = line;
                    result.print_start.line_number = line_number;
                    result.print_start.byte_offset = byte_offset;

                    spdlog::debug("[GCodeOpsDetector] Found {} call at line {}: {}",
                                  result.print_start.macro_name, line_number,
                                  line.substr(0, std::min<size_t>(line.size(), 60)));
                }
            }

            // Also parse params from the START_PRINT line
            if (upper_line.find("START_PRINT") != std::string::npos) {
                parse_start_print_params(line, line_number, byte_offset, result);
            }

            // Check against all patterns
            check_line(line, line_number, byte_offset, result);
        }

        byte_offset += line.size() + 1; // +1 for newline
    }

    result.lines_scanned = line_number;
    result.bytes_scanned = byte_offset;

    spdlog::debug("[GCodeOpsDetector] Scan complete: {} lines, {} bytes, {} operations found",
                  result.lines_scanned, result.bytes_scanned, result.operations.size());

    return result;
}

void GCodeOpsDetector::check_line(const std::string& line, size_t line_number, size_t byte_offset,
                                  ScanResult& result) const {
    // Trim leading whitespace for matching
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return;
    }
    std::string trimmed = line.substr(start);

    // Check each pattern
    for (const auto& pattern : patterns_) {
        bool found = false;

        if (pattern.case_sensitive) {
            found = trimmed.find(pattern.pattern) != std::string::npos;
        } else {
            // Case-insensitive search
            std::string upper_trimmed = trimmed;
            std::string upper_pattern = pattern.pattern;
            std::transform(upper_trimmed.begin(), upper_trimmed.end(), upper_trimmed.begin(),
                           ::toupper);
            std::transform(upper_pattern.begin(), upper_pattern.end(), upper_pattern.begin(),
                           ::toupper);
            found = upper_trimmed.find(upper_pattern) != std::string::npos;
        }

        if (found) {
            // Check if we already have this operation type (avoid duplicates)
            bool already_detected = std::any_of(
                result.operations.begin(), result.operations.end(),
                [&pattern](const DetectedOperation& op) { return op.type == pattern.type; });

            if (!already_detected) {
                DetectedOperation op;
                op.type = pattern.type;
                op.embedding = pattern.embedding;
                op.raw_line = line;
                op.macro_name = pattern.pattern;
                op.line_number = line_number;
                op.byte_offset = byte_offset;

                result.operations.push_back(std::move(op));

                spdlog::trace("[GCodeOpsDetector] Detected {} at line {}: {}",
                              operation_type_name(pattern.type), line_number, trimmed);
            }
        }
    }
}

bool GCodeOpsDetector::is_first_extrusion(const std::string& line) const {
    // Look for G1 with positive E value (actual extrusion, not retract)
    // Must start with G1 (or have whitespace before it)
    size_t g1_pos = line.find("G1");
    if (g1_pos == std::string::npos) {
        // Also check for G1 at start after whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            return false;
        }
        if (line.substr(start, 2) != "G1") {
            return false;
        }
    }

    // Find E parameter
    size_t e_pos = line.find(" E");
    if (e_pos == std::string::npos) {
        e_pos = line.find("\tE");
    }
    if (e_pos == std::string::npos) {
        return false;
    }

    // Extract value after E
    try {
        std::string e_str;
        for (size_t i = e_pos + 2; i < line.size(); i++) {
            char c = line[i];
            if (c == '-' || c == '.' || std::isdigit(c)) {
                e_str += c;
            } else {
                break;
            }
        }
        if (e_str.empty()) {
            return false;
        }
        float e_val = std::stof(e_str);
        return e_val > 0.001f; // Positive extrusion
    } catch (...) {
        return false;
    }
}

bool GCodeOpsDetector::is_layer_marker(const std::string& line) const {
    // Check for common layer change markers
    if (line.find(";LAYER_CHANGE") != std::string::npos) {
        return true;
    }
    if (line.find(";LAYER:") != std::string::npos) {
        return true;
    }
    // PrusaSlicer/OrcaSlicer format: ;Z:0.3
    if (line.find(";Z:") != std::string::npos && line.find(";Z:") < 5) {
        return true;
    }
    return false;
}

void GCodeOpsDetector::parse_start_print_params(const std::string& line, size_t line_number,
                                                size_t byte_offset, ScanResult& result) const {
    // Parse parameters like: START_PRINT EXTRUDER_TEMP=220 BED_TEMP=60 FORCE_LEVELING=true
    // We're looking for parameters that indicate operations:
    // - FORCE_LEVELING, BED_LEVEL, DO_BED_MESH, MESH -> bed leveling
    // - QGL, GANTRY_LEVEL, DO_QGL -> QGL
    // - Z_TILT, TILT_ADJUST -> Z tilt
    // - NOZZLE_CLEAN, CLEAN_NOZZLE, WIPE -> nozzle clean
    // - CHAMBER, CHAMBER_TEMP, SOAK_TIME -> chamber soak

    // Map of parameter names to operation types
    static const std::vector<std::pair<std::string, OperationType>> param_mappings = {
        // Bed leveling
        {"FORCE_LEVELING", OperationType::BED_LEVELING},
        {"BED_LEVEL", OperationType::BED_LEVELING},
        {"DO_BED_MESH", OperationType::BED_LEVELING},
        {"MESH", OperationType::BED_LEVELING},

        // QGL
        {"QGL", OperationType::QGL},
        {"GANTRY_LEVEL", OperationType::QGL},
        {"DO_QGL", OperationType::QGL},

        // Z tilt
        {"Z_TILT", OperationType::Z_TILT},
        {"TILT_ADJUST", OperationType::Z_TILT},

        // Nozzle clean
        {"NOZZLE_CLEAN", OperationType::NOZZLE_CLEAN},
        {"CLEAN_NOZZLE", OperationType::NOZZLE_CLEAN},
        {"WIPE", OperationType::NOZZLE_CLEAN},

        // Chamber soak
        {"CHAMBER_SOAK", OperationType::CHAMBER_SOAK},
        {"SOAK_TIME", OperationType::CHAMBER_SOAK},

        // Purge
        {"PURGE", OperationType::PURGE_LINE},
        {"PRIME", OperationType::PURGE_LINE},
    };

    // Convert line to uppercase for case-insensitive matching
    std::string upper_line = line;
    std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(), ::toupper);

    for (const auto& mapping : param_mappings) {
        const std::string& param_name = mapping.first;
        const OperationType op_type = mapping.second;

        // Look for PARAM_NAME= pattern
        std::string pattern = param_name + "=";
        size_t pos = upper_line.find(pattern);
        if (pos == std::string::npos) {
            continue;
        }

        // Extract value
        size_t value_start = pos + pattern.size();
        std::string value;
        for (size_t i = value_start; i < line.size(); i++) {
            char c = line[i];
            if (c == ' ' || c == '\t') {
                break;
            }
            value += c;
        }

        // Check if value indicates "enabled" (true, 1, non-zero number)
        std::string upper_value = value;
        std::transform(upper_value.begin(), upper_value.end(), upper_value.begin(), ::toupper);

        bool enabled = false;
        if (upper_value == "TRUE" || upper_value == "1" || upper_value == "YES") {
            enabled = true;
        } else if (upper_value == "FALSE" || upper_value == "0" || upper_value == "NO") {
            enabled = false;
        } else {
            // Try parsing as number
            try {
                float num = std::stof(value);
                enabled = num > 0;
            } catch (...) {
                // Not a number - for soak times etc., non-empty means enabled
                // But we should be conservative and not enable unknown values
                enabled = false;
            }
        }

        if (enabled) {
            // Check if we already have this operation type
            bool already_detected =
                std::any_of(result.operations.begin(), result.operations.end(),
                            [op_type](const DetectedOperation& op) { return op.type == op_type; });

            if (!already_detected) {
                DetectedOperation op;
                op.type = op_type;
                op.embedding = OperationEmbedding::MACRO_PARAMETER;
                op.raw_line = line;
                op.macro_name = "START_PRINT";
                op.param_name = param_name;
                op.param_value = value;
                op.line_number = line_number;
                op.byte_offset = byte_offset;

                result.operations.push_back(std::move(op));

                spdlog::trace(
                    "[GCodeOpsDetector] Detected {} via START_PRINT param {}={} at line {}",
                    operation_type_name(op_type), param_name, value, line_number);
            }
        }
    }
}

} // namespace gcode
} // namespace helix
