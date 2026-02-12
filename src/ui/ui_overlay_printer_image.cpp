// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_overlay_printer_image.cpp
 * @brief Implementation of PrinterImageOverlay
 *
 * Displays shipped and custom printer images in a grid layout for selection.
 * Cards are created from the printer_image_card XML component. USB section
 * visibility and status text are driven by subjects for declarative binding.
 */

#include "ui_overlay_printer_image.h"

#include "ui_error_reporting.h"
#include "ui_event_safety.h"
#include "ui_nav.h"
#include "ui_nav_manager.h"
#include "ui_update_queue.h"

#include "printer_image_manager.h"
#include "static_panel_registry.h"
#include "theme_manager.h"
#include "usb_manager.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <filesystem>
#include <memory>

namespace helix::settings {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<PrinterImageOverlay> g_printer_image_overlay;

PrinterImageOverlay& get_printer_image_overlay() {
    if (!g_printer_image_overlay) {
        g_printer_image_overlay = std::make_unique<PrinterImageOverlay>();
        StaticPanelRegistry::instance().register_destroy("PrinterImageOverlay",
                                                         []() { g_printer_image_overlay.reset(); });
    }
    return *g_printer_image_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

PrinterImageOverlay::PrinterImageOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

PrinterImageOverlay::~PrinterImageOverlay() {
    // SubjectManager handles subject deinit via RAII
    if (subjects_initialized_) {
        deinit_subjects_base(subjects_);
    }
    spdlog::trace("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void PrinterImageOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // USB section visibility subject (0=hidden, 1=visible)
    UI_MANAGED_SUBJECT_INT(usb_visible_subject_, 0, "printer_image_usb_visible", subjects_);

    // USB status text subject
    UI_MANAGED_SUBJECT_STRING(usb_status_subject_, usb_status_buf_, "", "printer_image_usb_status",
                              subjects_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void PrinterImageOverlay::register_callbacks() {
    lv_xml_register_event_cb(nullptr, "on_printer_image_auto_detect", on_auto_detect);
    lv_xml_register_event_cb(nullptr, "on_printer_image_card_clicked", on_image_card_clicked);
    lv_xml_register_event_cb(nullptr, "on_printer_image_usb_clicked", on_usb_image_clicked);
    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* PrinterImageOverlay::create(lv_obj_t* parent) {
    if (overlay_root_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_root_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    overlay_root_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "printer_image_overlay", nullptr));
    if (!overlay_root_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_root_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_root_;
}

void PrinterImageOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    parent_screen_ = parent_screen;

    // Ensure subjects and callbacks are initialized
    if (!subjects_initialized_) {
        init_subjects();
        register_callbacks();
    }

    // Lazy create overlay
    if (!overlay_root_ && parent_screen_) {
        create(parent_screen_);
    }

    if (!overlay_root_) {
        spdlog::error("[{}] Cannot show - overlay not created", get_name());
        return;
    }

    // Register for lifecycle callbacks
    NavigationManager::instance().register_overlay_instance(overlay_root_, this);

    // Push onto navigation stack (on_activate will populate grids)
    ui_nav_push_overlay(overlay_root_);
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void PrinterImageOverlay::set_usb_manager(UsbManager* manager) {
    usb_manager_ = manager;
    spdlog::debug("[{}] USB manager set ({})", get_name(), manager ? "valid" : "null");
}

void PrinterImageOverlay::refresh_custom_images() {
    populate_custom_images();
    std::string active_id = helix::PrinterImageManager::instance().get_active_image_id();
    update_selection_indicator(active_id);
}

void PrinterImageOverlay::on_activate() {
    OverlayBase::on_activate();

    populate_shipped_images();
    populate_custom_images();
    scan_usb_drives();

    // Highlight the currently active image
    std::string active_id = helix::PrinterImageManager::instance().get_active_image_id();
    update_selection_indicator(active_id);
}

void PrinterImageOverlay::on_deactivate() {
    OverlayBase::on_deactivate();
}

// ============================================================================
// CARD CREATION (XML-based)
// ============================================================================

lv_obj_t* PrinterImageOverlay::create_card_from_xml(lv_obj_t* parent, const std::string& image_id,
                                                    const std::string& display_name,
                                                    const std::string& preview_path,
                                                    const char* callback_name) {
    // Only show .bin previews (PNGs are too slow to decode during scroll)
    bool is_bin = preview_path.size() > 4 && preview_path.substr(preview_path.size() - 4) == ".bin";
    const char* show_img = (is_bin && !preview_path.empty()) ? "1" : "0";

    const char* attrs[] = {"image_src",          preview_path.c_str(), "label_text",
                           display_name.c_str(), "show_image",         show_img,
                           "callback",           callback_name,        nullptr};
    lv_obj_t* card = static_cast<lv_obj_t*>(lv_xml_create(parent, "printer_image_card", attrs));
    if (!card) {
        spdlog::warn("[{}] Failed to create card for {}", get_name(), image_id);
        return nullptr;
    }

    // Toggle image/placeholder visibility based on show_image
    // (one-time setup since bind_flag_if_eq cannot use prop values as subjects)
    lv_obj_t* card_image = lv_obj_find_by_name(card, "card_image");
    lv_obj_t* card_placeholder = lv_obj_find_by_name(card, "card_placeholder_icon");
    if (is_bin && !preview_path.empty()) {
        // Show image, hide placeholder
        if (card_placeholder)
            lv_obj_add_flag(card_placeholder, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Show placeholder, hide image
        if (card_image)
            lv_obj_add_flag(card_image, LV_OBJ_FLAG_HIDDEN);
    }

    // Store image_id in user_data (freed on card delete)
    char* id_copy = strdup(image_id.c_str());
    if (!id_copy) {
        spdlog::error("[{}] Failed to allocate memory for image id", get_name());
        return card;
    }
    lv_obj_set_user_data(card, id_copy);

    // Free strdup'd user_data when LVGL destroys the card
    // (acceptable exception to declarative UI rule for cleanup)
    lv_obj_add_event_cb(
        card,
        [](lv_event_t* e) {
            auto* obj = lv_event_get_current_target_obj(e);
            void* data = lv_obj_get_user_data(obj);
            if (data) {
                free(data);
            }
        },
        LV_EVENT_DELETE, nullptr);

    return card;
}

// ============================================================================
// GRID POPULATION
// ============================================================================

void PrinterImageOverlay::populate_shipped_images() {
    if (!overlay_root_) {
        return;
    }

    lv_obj_t* grid = lv_obj_find_by_name(overlay_root_, "shipped_images_grid");
    if (!grid) {
        spdlog::warn("[{}] shipped_images_grid not found", get_name());
        return;
    }

    lv_obj_clean(grid);

    auto images = helix::PrinterImageManager::instance().get_shipped_images();
    spdlog::debug("[{}] Populating {} shipped images", get_name(), images.size());

    for (const auto& img : images) {
        create_card_from_xml(grid, img.id, img.display_name, img.preview_path,
                             "on_printer_image_card_clicked");
    }
}

void PrinterImageOverlay::populate_custom_images() {
    if (!overlay_root_) {
        return;
    }

    lv_obj_t* grid = lv_obj_find_by_name(overlay_root_, "custom_images_grid");
    if (!grid) {
        spdlog::warn("[{}] custom_images_grid not found", get_name());
        return;
    }

    lv_obj_clean(grid);

    auto images = helix::PrinterImageManager::instance().get_custom_images();
    spdlog::debug("[{}] Populating {} custom images", get_name(), images.size());

    for (const auto& img : images) {
        create_card_from_xml(grid, img.id, img.display_name, img.preview_path,
                             "on_printer_image_card_clicked");
    }
}

void PrinterImageOverlay::update_selection_indicator(const std::string& active_id) {
    if (!overlay_root_) {
        return;
    }

    lv_color_t highlight_color = theme_manager_get_color("primary_color");

    // Helper lambda to update a grid's card selection styles
    auto update_grid = [&](const char* grid_name) {
        lv_obj_t* grid = lv_obj_find_by_name(overlay_root_, grid_name);
        if (!grid)
            return;

        uint32_t count = lv_obj_get_child_count(grid);
        for (uint32_t i = 0; i < count; i++) {
            lv_obj_t* child = lv_obj_get_child(grid, static_cast<int32_t>(i));
            auto* id = static_cast<const char*>(lv_obj_get_user_data(child));
            if (id && std::string(id) == active_id) {
                lv_obj_set_style_border_color(child, highlight_color, LV_PART_MAIN);
                lv_obj_set_style_border_opa(child, LV_OPA_COVER, LV_PART_MAIN);
            } else {
                lv_obj_set_style_border_opa(child, LV_OPA_TRANSP, LV_PART_MAIN);
            }
        }
    };

    update_grid("shipped_images_grid");
    update_grid("custom_images_grid");
}

// ============================================================================
// USB IMPORT
// ============================================================================

void PrinterImageOverlay::scan_usb_drives() {
    if (!usb_manager_ || !usb_manager_->is_running()) {
        lv_subject_set_int(&usb_visible_subject_, 0);
        return;
    }

    auto drives = usb_manager_->get_drives();
    if (drives.empty()) {
        lv_subject_set_int(&usb_visible_subject_, 0);
        return;
    }

    lv_subject_set_int(&usb_visible_subject_, 1);
    spdlog::debug("[{}] Found {} USB drive(s), scanning first: {}", get_name(), drives.size(),
                  drives[0].mount_path);
    populate_usb_images(drives[0].mount_path);
}

void PrinterImageOverlay::populate_usb_images(const std::string& mount_path) {
    lv_obj_t* grid = lv_obj_find_by_name(overlay_root_, "usb_images_grid");
    if (!grid) {
        spdlog::warn("[{}] usb_images_grid not found", get_name());
        return;
    }

    lv_obj_clean(grid);

    auto image_paths = helix::PrinterImageManager::instance().scan_for_images(mount_path);
    spdlog::debug("[{}] Found {} importable images on USB", get_name(), image_paths.size());

    if (image_paths.empty()) {
        lv_subject_copy_string(&usb_status_subject_, "No PNG or JPEG images found on USB drive");
        return;
    }

    lv_subject_copy_string(&usb_status_subject_, "");

    for (const auto& path : image_paths) {
        std::string filename = std::filesystem::path(path).filename().string();

        // USB cards use a different callback (import behavior vs select)
        // and store the full path as image_id for the import handler
        create_card_from_xml(grid, path, filename, "", "on_printer_image_usb_clicked");
    }
}

void PrinterImageOverlay::handle_usb_import(const std::string& source_path) {
    std::string filename = std::filesystem::path(source_path).filename().string();
    spdlog::info("[{}] Importing USB image: {}", get_name(), filename);

    // Update status via subject binding
    std::string msg = "Importing " + filename + "...";
    lv_subject_copy_string(&usb_status_subject_, msg.c_str());

    // import_image_async() currently runs synchronously, but the callback is wrapped
    // in ui_queue_update() for safety in case the implementation becomes truly async
    helix::PrinterImageManager::instance().import_image_async(
        source_path, [filename](helix::PrinterImageManager::ImportResult result) {
            ui_queue_update([result = std::move(result), filename]() {
                auto& overlay = get_printer_image_overlay();

                if (result.success) {
                    spdlog::info("[Printer Image] USB import success: {}", result.id);
                    lv_subject_copy_string(&overlay.usb_status_subject_, "");
                    overlay.refresh_custom_images();
                    overlay.handle_image_selected(result.id);
                    NOTIFY_SUCCESS("Imported {}", filename);
                } else {
                    spdlog::warn("[Printer Image] USB import failed: {}", result.error);
                    lv_subject_copy_string(&overlay.usb_status_subject_, result.error.c_str());
                    NOTIFY_WARNING("Import failed: {}", result.error);
                }
            });
        });
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void PrinterImageOverlay::handle_auto_detect() {
    spdlog::info("[{}] Auto-detect selected", get_name());
    helix::PrinterImageManager::instance().set_active_image("");
    update_selection_indicator("");
    NOTIFY_INFO("Printer image set to auto-detect");
}

void PrinterImageOverlay::handle_image_selected(const std::string& image_id) {
    spdlog::info("[{}] Image selected: {}", get_name(), image_id);
    helix::PrinterImageManager::instance().set_active_image(image_id);
    update_selection_indicator(image_id);

    // Extract display name from ID for notification
    std::string display_name = image_id;
    auto colon_pos = image_id.find(':');
    if (colon_pos != std::string::npos) {
        display_name = image_id.substr(colon_pos + 1);
    }
    NOTIFY_INFO("Printer image: {}", display_name);
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void PrinterImageOverlay::on_auto_detect(lv_event_t* /*e*/) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrinterImageOverlay] on_auto_detect");
    get_printer_image_overlay().handle_auto_detect();
    LVGL_SAFE_EVENT_CB_END();
}

void PrinterImageOverlay::on_image_card_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrinterImageOverlay] on_image_card_clicked");
    // Get the card root (current_target = obj with the handler = card_root view)
    auto* card = lv_event_get_current_target_obj(e);
    auto* id = static_cast<const char*>(lv_obj_get_user_data(card));
    if (id) {
        get_printer_image_overlay().handle_image_selected(std::string(id));
    }
    LVGL_SAFE_EVENT_CB_END();
}

void PrinterImageOverlay::on_usb_image_clicked(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[PrinterImageOverlay] on_usb_image_clicked");
    // Get the card root (current_target = obj with the handler = card_root view)
    auto* card = lv_event_get_current_target_obj(e);
    auto* path = static_cast<const char*>(lv_obj_get_user_data(card));
    if (path) {
        get_printer_image_overlay().handle_usb_import(std::string(path));
    }
    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::settings
