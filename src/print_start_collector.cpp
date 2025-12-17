// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "print_start_collector.h"

#include <spdlog/spdlog.h>

#include <atomic>

using json = nlohmann::json;

// ============================================================================
// STATIC PATTERN DEFINITIONS
// ============================================================================

// Phase detection patterns with progress weights
// Weights should roughly sum to 100% for typical PRINT_START macros
const std::vector<PrintStartCollector::PhasePattern> PrintStartCollector::phase_patterns_ = {
    // Homing (usually first step)
    {PrintStartPhase::HOMING, std::regex(R"(G28|Homing|Home All Axes|homing)", std::regex::icase),
     "Homing...", 10},

    // Heating - bed
    {PrintStartPhase::HEATING_BED,
     std::regex(R"(M190|M140\s+S[1-9]|Heating bed|Heat Bed|BED_TEMP|bed.*heat)", std::regex::icase),
     "Heating Bed...", 20},

    // Heating - nozzle
    {PrintStartPhase::HEATING_NOZZLE,
     std::regex(R"(M109|M104\s+S[1-9]|Heating (nozzle|hotend|extruder)|EXTRUDER_TEMP)",
                std::regex::icase),
     "Heating Nozzle...", 20},

    // Quad gantry level
    {PrintStartPhase::QGL,
     std::regex(R"(QUAD_GANTRY_LEVEL|quad.?gantry.?level|QGL)", std::regex::icase),
     "Leveling Gantry...", 15},

    // Z tilt adjust
    {PrintStartPhase::Z_TILT, std::regex(R"(Z_TILT_ADJUST|z.?tilt.?adjust)", std::regex::icase),
     "Z Tilt Adjust...", 15},

    // Bed mesh
    {PrintStartPhase::BED_MESH,
     std::regex(R"(BED_MESH_CALIBRATE|BED_MESH_PROFILE\s+LOAD=|Loading bed mesh|mesh.*load)",
                std::regex::icase),
     "Loading Bed Mesh...", 10},

    // Nozzle cleaning
    {PrintStartPhase::CLEANING,
     std::regex(R"(CLEAN_NOZZLE|NOZZLE_CLEAN|WIPE_NOZZLE|nozzle.?wipe|clean.?nozzle)",
                std::regex::icase),
     "Cleaning Nozzle...", 5},

    // Purge line
    {PrintStartPhase::PURGING,
     std::regex(R"(VORON_PURGE|LINE_PURGE|PURGE_LINE|Prime.?Line|Priming|KAMP_.*PURGE|purge.?line)",
                std::regex::icase),
     "Purging...", 5},
};

// Pattern to detect PRINT_START macro invocation
const std::regex
    PrintStartCollector::print_start_pattern_(R"(PRINT_START|START_PRINT|_PRINT_START)",
                                              std::regex::icase);

// Pattern to detect print start completion (first layer indicator)
const std::regex PrintStartCollector::completion_pattern_(
    R"(SET_PRINT_STATS_INFO\s+CURRENT_LAYER=|LAYER:?\s*1\b|;LAYER:1|First layer)",
    std::regex::icase);

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

PrintStartCollector::PrintStartCollector(MoonrakerClient& client, PrinterState& state)
    : client_(client), state_(state) {
    spdlog::debug("[PrintStartCollector] Constructed");
}

PrintStartCollector::~PrintStartCollector() {
    // Don't call stop() here - it uses client_ and state_ references which may
    // already be destroyed during static destruction order. Callers should
    // explicitly call stop() before letting the shared_ptr go out of scope.
    // Just mark as inactive to prevent any pending callbacks from running.
    active_.store(false);
    registered_.store(false);
}

// ============================================================================
// PUBLIC METHODS
// ============================================================================

void PrintStartCollector::start() {
    if (active_.load()) {
        spdlog::debug("[PrintStartCollector] Already active, ignoring start()");
        return;
    }

    // Generate unique handler name
    static std::atomic<uint64_t> s_collector_id{0};
    handler_name_ = "print_start_collector_" + std::to_string(++s_collector_id);

    // Register for G-code responses
    auto self = shared_from_this();
    client_.register_method_callback("notify_gcode_response", handler_name_,
                                     [self](const json& msg) { self->on_gcode_response(msg); });

    registered_.store(true);
    active_.store(true);

    // Set initial state
    state_.set_print_start_state(PrintStartPhase::INITIALIZING, "Preparing Print...", 0);

    spdlog::info("[PrintStartCollector] Started monitoring (handler: {})", handler_name_);
}

void PrintStartCollector::stop() {
    bool was_registered = registered_.exchange(false);
    if (was_registered) {
        client_.unregister_method_callback("notify_gcode_response", handler_name_);
        spdlog::debug("[PrintStartCollector] Unregistered callback");
    }

    if (active_.exchange(false)) {
        state_.reset_print_start_state();
        spdlog::info("[PrintStartCollector] Stopped monitoring");
    }
}

void PrintStartCollector::reset() {
    detected_phases_.clear();
    current_phase_ = PrintStartPhase::IDLE;
    print_start_detected_ = false;

    if (active_.load()) {
        state_.set_print_start_state(PrintStartPhase::INITIALIZING, "Preparing Print...", 0);
    }

    spdlog::debug("[PrintStartCollector] Reset state");
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void PrintStartCollector::on_gcode_response(const json& msg) {
    if (!active_.load()) {
        return;
    }

    // Parse notify_gcode_response format: {"method": "...", "params": ["line"]}
    if (!msg.contains("params") || !msg["params"].is_array() || msg["params"].empty()) {
        return;
    }

    const std::string& line = msg["params"][0].get_ref<const std::string&>();

    // Skip empty lines and common noise
    if (line.empty() || line == "ok") {
        return;
    }

    spdlog::trace("[PrintStartCollector] G-code: {}", line);

    // Check for PRINT_START marker (once per session)
    if (!print_start_detected_ && is_print_start_marker(line)) {
        print_start_detected_ = true;
        update_phase(PrintStartPhase::INITIALIZING, "Starting Print...");
        spdlog::info("[PrintStartCollector] PRINT_START detected");
        return;
    }

    // Check for completion (layer 1 indicator)
    if (is_completion_marker(line)) {
        update_phase(PrintStartPhase::COMPLETE, "Starting Print...");
        spdlog::info("[PrintStartCollector] Print start complete - layer 1 detected");
        // Note: The caller (main.cpp) should stop the collector when print state becomes PRINTING
        return;
    }

    // Check phase patterns
    check_phase_patterns(line);
}

void PrintStartCollector::check_phase_patterns(const std::string& line) {
    for (const auto& pattern : phase_patterns_) {
        if (std::regex_search(line, pattern.pattern)) {
            // Only update if this is a new phase
            if (detected_phases_.find(pattern.phase) == detected_phases_.end()) {
                detected_phases_.insert(pattern.phase);
                update_phase(pattern.phase, pattern.message);

                spdlog::debug("[PrintStartCollector] Detected phase: {} (progress: {}%)",
                              static_cast<int>(pattern.phase), calculate_progress());
            }
            return; // Only match first pattern per line
        }
    }
}

void PrintStartCollector::update_phase(PrintStartPhase phase, const char* message) {
    current_phase_ = phase;
    int progress = calculate_progress();
    state_.set_print_start_state(phase, message, progress);
}

int PrintStartCollector::calculate_progress() const {
    int total_weight = 0;

    for (const auto& pattern : phase_patterns_) {
        if (detected_phases_.find(pattern.phase) != detected_phases_.end()) {
            total_weight += pattern.weight;
        }
    }

    // Cap at 95% - final 5% is for completion transition
    return std::min(total_weight, 95);
}

bool PrintStartCollector::is_print_start_marker(const std::string& line) const {
    return std::regex_search(line, print_start_pattern_);
}

bool PrintStartCollector::is_completion_marker(const std::string& line) const {
    return std::regex_search(line, completion_pattern_);
}
