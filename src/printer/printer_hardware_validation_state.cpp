// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file printer_hardware_validation_state.cpp
 * @brief Hardware validation state management extracted from PrinterState
 *
 * Manages hardware validation subjects for UI display including issue counts,
 * severity levels, and formatted status text for the Settings panel.
 *
 * Extracted from PrinterState as part of god class decomposition.
 */

#include "printer_hardware_validation_state.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdio>

namespace helix {

void PrinterHardwareValidationState::init_subjects(bool register_xml) {
    if (subjects_initialized_) {
        spdlog::debug("[PrinterHardwareValidationState] Subjects already initialized, skipping");
        return;
    }

    spdlog::debug("[PrinterHardwareValidationState] Initializing subjects (register_xml={})",
                  register_xml);

    // Initialize string buffers
    std::memset(hardware_status_title_buf_, 0, sizeof(hardware_status_title_buf_));
    std::memset(hardware_status_detail_buf_, 0, sizeof(hardware_status_detail_buf_));
    std::memset(hardware_issues_label_buf_, 0, sizeof(hardware_issues_label_buf_));

    // Set default values
    std::strcpy(hardware_status_title_buf_, "Healthy");
    std::strcpy(hardware_issues_label_buf_, "No Hardware Issues");

    // Initialize hardware validation subjects
    lv_subject_init_int(&hardware_has_issues_, 0);
    lv_subject_init_int(&hardware_issue_count_, 0);
    lv_subject_init_int(&hardware_max_severity_, 0);
    lv_subject_init_int(&hardware_validation_version_, 0);
    lv_subject_init_int(&hardware_critical_count_, 0);
    lv_subject_init_int(&hardware_warning_count_, 0);
    lv_subject_init_int(&hardware_info_count_, 0);
    lv_subject_init_int(&hardware_session_count_, 0);
    lv_subject_init_string(&hardware_status_title_, hardware_status_title_buf_, nullptr,
                           sizeof(hardware_status_title_buf_), "Healthy");
    lv_subject_init_string(&hardware_status_detail_, hardware_status_detail_buf_, nullptr,
                           sizeof(hardware_status_detail_buf_), "");
    lv_subject_init_string(&hardware_issues_label_, hardware_issues_label_buf_, nullptr,
                           sizeof(hardware_issues_label_buf_), "No Hardware Issues");

    // Register with SubjectManager for automatic cleanup
    subjects_.register_subject(&hardware_has_issues_);
    subjects_.register_subject(&hardware_issue_count_);
    subjects_.register_subject(&hardware_max_severity_);
    subjects_.register_subject(&hardware_validation_version_);
    subjects_.register_subject(&hardware_critical_count_);
    subjects_.register_subject(&hardware_warning_count_);
    subjects_.register_subject(&hardware_info_count_);
    subjects_.register_subject(&hardware_session_count_);
    subjects_.register_subject(&hardware_status_title_);
    subjects_.register_subject(&hardware_status_detail_);
    subjects_.register_subject(&hardware_issues_label_);

    // Register with LVGL XML system for XML bindings
    if (register_xml) {
        spdlog::debug("[PrinterHardwareValidationState] Registering subjects with XML system");
        lv_xml_register_subject(NULL, "hardware_has_issues", &hardware_has_issues_);
        lv_xml_register_subject(NULL, "hardware_issue_count", &hardware_issue_count_);
        lv_xml_register_subject(NULL, "hardware_max_severity", &hardware_max_severity_);
        lv_xml_register_subject(NULL, "hardware_validation_version", &hardware_validation_version_);
        lv_xml_register_subject(NULL, "hardware_critical_count", &hardware_critical_count_);
        lv_xml_register_subject(NULL, "hardware_warning_count", &hardware_warning_count_);
        lv_xml_register_subject(NULL, "hardware_info_count", &hardware_info_count_);
        lv_xml_register_subject(NULL, "hardware_session_count", &hardware_session_count_);
        lv_xml_register_subject(NULL, "hardware_status_title", &hardware_status_title_);
        lv_xml_register_subject(NULL, "hardware_status_detail", &hardware_status_detail_);
        lv_xml_register_subject(NULL, "hardware_issues_label", &hardware_issues_label_);
    } else {
        spdlog::debug("[PrinterHardwareValidationState] Skipping XML registration (tests mode)");
    }

    subjects_initialized_ = true;
    spdlog::debug("[PrinterHardwareValidationState] Subjects initialized successfully");
}

void PrinterHardwareValidationState::deinit_subjects() {
    if (!subjects_initialized_) {
        return;
    }

    spdlog::debug("[PrinterHardwareValidationState] Deinitializing subjects");
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterHardwareValidationState::reset_for_testing() {
    if (!subjects_initialized_) {
        spdlog::debug("[PrinterHardwareValidationState] reset_for_testing: subjects not "
                      "initialized, nothing to reset");
        return;
    }

    spdlog::info("[PrinterHardwareValidationState] reset_for_testing: Deinitializing subjects to "
                 "clear observers");

    // Clear the validation result
    hardware_validation_result_ = HardwareValidationResult{};

    // Use SubjectManager for automatic subject cleanup
    subjects_.deinit_all();
    subjects_initialized_ = false;
}

void PrinterHardwareValidationState::set_hardware_validation_result(
    const HardwareValidationResult& result) {
    // Store the full result for UI access
    hardware_validation_result_ = result;

    // Update summary subjects
    lv_subject_set_int(&hardware_has_issues_, result.has_issues() ? 1 : 0);
    lv_subject_set_int(&hardware_issue_count_, static_cast<int>(result.total_issue_count()));
    lv_subject_set_int(&hardware_max_severity_, static_cast<int>(result.max_severity()));

    // Update category counts
    lv_subject_set_int(&hardware_critical_count_, static_cast<int>(result.critical_missing.size()));
    lv_subject_set_int(&hardware_warning_count_, static_cast<int>(result.expected_missing.size()));
    lv_subject_set_int(&hardware_info_count_, static_cast<int>(result.newly_discovered.size()));
    lv_subject_set_int(&hardware_session_count_,
                       static_cast<int>(result.changed_from_last_session.size()));

    // Update status text
    if (!result.has_issues()) {
        snprintf(hardware_status_title_buf_, sizeof(hardware_status_title_buf_), "All Healthy");
        snprintf(hardware_status_detail_buf_, sizeof(hardware_status_detail_buf_),
                 "All configured hardware detected");
    } else {
        size_t total = result.total_issue_count();
        snprintf(hardware_status_title_buf_, sizeof(hardware_status_title_buf_),
                 "%zu Issue%s Detected", total, total == 1 ? "" : "s");

        // Build detail string
        std::string detail;
        if (!result.critical_missing.empty()) {
            detail += std::to_string(result.critical_missing.size()) + " critical";
        }
        if (!result.expected_missing.empty()) {
            if (!detail.empty())
                detail += ", ";
            detail += std::to_string(result.expected_missing.size()) + " missing";
        }
        if (!result.newly_discovered.empty()) {
            if (!detail.empty())
                detail += ", ";
            detail += std::to_string(result.newly_discovered.size()) + " new";
        }
        if (!result.changed_from_last_session.empty()) {
            if (!detail.empty())
                detail += ", ";
            detail += std::to_string(result.changed_from_last_session.size()) + " changed";
        }
        snprintf(hardware_status_detail_buf_, sizeof(hardware_status_detail_buf_), "%s",
                 detail.c_str());
    }
    lv_subject_copy_string(&hardware_status_title_, hardware_status_title_buf_);
    lv_subject_copy_string(&hardware_status_detail_, hardware_status_detail_buf_);

    // Update issues label for settings panel ("1 Hardware Issue" / "5 Hardware Issues")
    size_t total = result.total_issue_count();
    if (total == 0) {
        snprintf(hardware_issues_label_buf_, sizeof(hardware_issues_label_buf_),
                 "No Hardware Issues");
    } else if (total == 1) {
        snprintf(hardware_issues_label_buf_, sizeof(hardware_issues_label_buf_),
                 "1 Hardware Issue");
    } else {
        snprintf(hardware_issues_label_buf_, sizeof(hardware_issues_label_buf_),
                 "%zu Hardware Issues", total);
    }
    lv_subject_copy_string(&hardware_issues_label_, hardware_issues_label_buf_);

    // Increment version to notify UI observers
    int version = lv_subject_get_int(&hardware_validation_version_);
    lv_subject_set_int(&hardware_validation_version_, version + 1);

    spdlog::debug("[PrinterHardwareValidationState] Hardware validation updated: {} issues, "
                  "max_severity={}",
                  result.total_issue_count(), static_cast<int>(result.max_severity()));
}

void PrinterHardwareValidationState::remove_hardware_issue(const std::string& hardware_name) {
    // Helper lambda to remove an issue from a vector by hardware_name
    auto remove_by_name = [&hardware_name](std::vector<HardwareIssue>& issues) {
        issues.erase(std::remove_if(issues.begin(), issues.end(),
                                    [&hardware_name](const HardwareIssue& issue) {
                                        return issue.hardware_name == hardware_name;
                                    }),
                     issues.end());
    };

    // Remove from all issue lists
    remove_by_name(hardware_validation_result_.critical_missing);
    remove_by_name(hardware_validation_result_.expected_missing);
    remove_by_name(hardware_validation_result_.newly_discovered);
    remove_by_name(hardware_validation_result_.changed_from_last_session);

    // Re-apply the updated result to refresh all subjects
    set_hardware_validation_result(hardware_validation_result_);

    spdlog::debug("[PrinterHardwareValidationState] Removed hardware issue: {}", hardware_name);
}

} // namespace helix
