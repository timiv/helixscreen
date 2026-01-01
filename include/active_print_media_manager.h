// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 HelixScreen

#pragma once

#include "ui_observer_guard.h"

#include "moonraker_api.h"
#include "printer_state.h"

#include <atomic>
#include <memory>
#include <string>

namespace helix {

/**
 * @brief Manages display info for the active print (thumbnail, display filename)
 *
 * Decouples shared print media from PrintStatusPanel so that:
 * 1. HomePanel always has current data (regardless of which panels are open)
 * 2. Thread-safe LVGL updates via ui_queue_update()
 * 3. Single point of truth for filename resolution and thumbnail loading
 *
 * Thread Safety:
 * - set_api() must be called from main thread only
 * - set_thumbnail_source() must be called from main thread only
 * - Observer callbacks from PrinterState trigger on main thread (LVGL observer)
 * - All lv_subject updates are deferred to main thread via ui_queue_update()
 *
 * Initialization order: PrinterState -> ActivePrintMediaManager -> Panels
 */
class ActivePrintMediaManager {
  public:
    explicit ActivePrintMediaManager(PrinterState& printer_state);
    ~ActivePrintMediaManager();

    // Non-copyable
    ActivePrintMediaManager(const ActivePrintMediaManager&) = delete;
    ActivePrintMediaManager& operator=(const ActivePrintMediaManager&) = delete;

    /**
     * @brief Set the MoonrakerAPI instance for thumbnail downloads
     *
     * Must be called before thumbnail loading will work.
     *
     * @param api Pointer to MoonrakerAPI (can be nullptr to disable)
     */
    void set_api(MoonrakerAPI* api);

    /**
     * @brief Set the original filename for thumbnail lookup
     *
     * Call this when starting a print with a modified temp file to override
     * the filename used for metadata/thumbnail lookup. This handles the case
     * where Moonraker reports .helix_temp/modified_* but thumbnails are stored
     * under the original filename.
     *
     * @param original_filename The original filename before modification
     */
    void set_thumbnail_source(const std::string& original_filename);

    /**
     * @brief Clear the thumbnail source override
     *
     * Called when print ends to reset state for next print.
     */
    void clear_thumbnail_source();

  private:
    static void on_print_filename_changed(lv_observer_t* observer, lv_subject_t* subject);
    void process_filename(const char* raw_filename);
    void load_thumbnail_for_file(const std::string& filename);
    void clear_print_info();

    PrinterState& printer_state_;
    MoonrakerAPI* api_ = nullptr;
    ObserverGuard print_filename_observer_;
    std::string thumbnail_source_filename_;
    std::string last_effective_filename_;
    std::string last_loaded_thumbnail_filename_;
    uint32_t thumbnail_load_generation_ = 0;

    /// Alive flag for ThumbnailLoadContext compatibility (always true for singleton)
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
};

/**
 * @brief Initialize the global ActivePrintMediaManager singleton
 *
 * Must be called after init_printer_state_subjects() and before panels
 * that depend on print_display_filename/print_thumbnail_path subjects.
 */
void init_active_print_media_manager();

/**
 * @brief Get the global ActivePrintMediaManager instance
 *
 * @return Reference to the singleton instance
 * @throws std::runtime_error if called before init_active_print_media_manager()
 */
ActivePrintMediaManager& get_active_print_media_manager();

} // namespace helix
