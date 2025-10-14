/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of GuppyScreen.
 *
 * GuppyScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GuppyScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GuppyScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "lvgl/lvgl.h"
#include "lvgl/src/others/xml/lv_xml.h"
#include "ui_nav.h"
#include "ui_theme.h"
#include "ui_fonts.h"
#include "ui_panel_home.h"
#include "ui_panel_print_select.h"
#include "ui_panel_controls.h"
#include "ui_panel_motion.h"
#include "ui_panel_controls_temp.h"
#include "ui_panel_controls_extrusion.h"
#include "ui_component_keypad.h"
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <ctime>

// LVGL display and input
static lv_display_t* display = nullptr;
static lv_indev_t* indev_mouse = nullptr;

// Screen dimensions (default to medium size)
static const int SCREEN_WIDTH = UI_SCREEN_MEDIUM_W;
static const int SCREEN_HEIGHT = UI_SCREEN_MEDIUM_H;

// Initialize LVGL with SDL
static bool init_lvgl() {
    lv_init();

    // LVGL's SDL driver handles window creation internally
    display = lv_sdl_window_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display) {
        LV_LOG_ERROR("Failed to create LVGL SDL display");
        return false;
    }

    // Create mouse input device
    indev_mouse = lv_sdl_mouse_create();
    if (!indev_mouse) {
        LV_LOG_ERROR("Failed to create LVGL SDL mouse input");
        return false;
    }

    LV_LOG_USER("LVGL initialized: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
    return true;
}

// Save screenshot using SDL renderer
// Simple BMP file writer for ARGB8888 format
static bool write_bmp(const char* filename, const uint8_t* data, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) return false;

    // BMP header (54 bytes total)
    uint32_t file_size = 54 + (width * height * 4);
    uint32_t pixel_offset = 54;
    uint32_t dib_size = 40;
    uint16_t planes = 1;
    uint16_t bpp = 32;

    // BMP file header (14 bytes)
    fputc('B', f); fputc('M', f);                        // Signature
    fwrite(&file_size, 4, 1, f);                         // File size
    fwrite((uint32_t[]){0}, 4, 1, f);                    // Reserved
    fwrite(&pixel_offset, 4, 1, f);                      // Pixel data offset

    // DIB header (40 bytes)
    fwrite(&dib_size, 4, 1, f);                          // DIB header size
    fwrite(&width, 4, 1, f);                             // Width
    fwrite(&height, 4, 1, f);                            // Height
    fwrite(&planes, 2, 1, f);                            // Planes
    fwrite(&bpp, 2, 1, f);                               // Bits per pixel
    fwrite((uint32_t[]){0}, 4, 1, f);                    // Compression (none)
    uint32_t image_size = width * height * 4;
    fwrite(&image_size, 4, 1, f);                        // Image size
    fwrite((uint32_t[]){2835}, 4, 1, f);                 // X pixels per meter
    fwrite((uint32_t[]){2835}, 4, 1, f);                 // Y pixels per meter
    fwrite((uint32_t[]){0}, 4, 1, f);                    // Colors in palette
    fwrite((uint32_t[]){0}, 4, 1, f);                    // Important colors

    // Write pixel data (BMP is bottom-up, so flip rows)
    for (int y = height - 1; y >= 0; y--) {
        fwrite(data + (y * width * 4), 4, width, f);
    }

    fclose(f);
    return true;
}

static void save_screenshot() {
    // Generate unique filename with timestamp
    char filename[256];
    snprintf(filename, sizeof(filename), "/tmp/ui-screenshot-%lu.bmp",
             (unsigned long)time(NULL));

    // Take snapshot using LVGL's native API (platform-independent)
    lv_obj_t* screen = lv_screen_active();
    lv_draw_buf_t* snapshot = lv_snapshot_take(screen, LV_COLOR_FORMAT_ARGB8888);

    if (!snapshot) {
        LV_LOG_ERROR("Failed to take screenshot");
        return;
    }

    // Write BMP file
    if (write_bmp(filename, snapshot->data, snapshot->header.w, snapshot->header.h)) {
        LV_LOG_USER("Screenshot saved: %s", filename);
    } else {
        LV_LOG_ERROR("Failed to save screenshot");
    }

    // Free snapshot buffer
    lv_draw_buf_destroy(snapshot);
}

// Main application
int main(int argc, char** argv) {
    // Parse command-line arguments for panel selection
    int initial_panel = UI_PANEL_HOME;  // Default to home panel
    bool show_motion = false;  // Special flag for motion sub-screen
    bool show_nozzle_temp = false;  // Special flag for nozzle temp sub-screen
    bool show_bed_temp = false;  // Special flag for bed temp sub-screen
    bool show_extrusion = false;  // Special flag for extrusion sub-screen

    if (argc > 1) {
        const char* panel_arg = argv[1];
        if (strcmp(panel_arg, "home") == 0) {
            initial_panel = UI_PANEL_HOME;
        } else if (strcmp(panel_arg, "controls") == 0) {
            initial_panel = UI_PANEL_CONTROLS;
        } else if (strcmp(panel_arg, "motion") == 0) {
            initial_panel = UI_PANEL_CONTROLS;
            show_motion = true;
        } else if (strcmp(panel_arg, "nozzle-temp") == 0) {
            initial_panel = UI_PANEL_CONTROLS;
            show_nozzle_temp = true;
        } else if (strcmp(panel_arg, "bed-temp") == 0) {
            initial_panel = UI_PANEL_CONTROLS;
            show_bed_temp = true;
        } else if (strcmp(panel_arg, "extrusion") == 0) {
            initial_panel = UI_PANEL_CONTROLS;
            show_extrusion = true;
        } else if (strcmp(panel_arg, "filament") == 0) {
            initial_panel = UI_PANEL_FILAMENT;
        } else if (strcmp(panel_arg, "settings") == 0) {
            initial_panel = UI_PANEL_SETTINGS;
        } else if (strcmp(panel_arg, "advanced") == 0) {
            initial_panel = UI_PANEL_ADVANCED;
        } else if (strcmp(panel_arg, "print-select") == 0 || strcmp(panel_arg, "print_select") == 0) {
            initial_panel = UI_PANEL_PRINT_SELECT;
        } else {
            printf("Unknown panel: %s\n", panel_arg);
            printf("Usage: %s [panel_name]\n", argv[0]);
            printf("Available panels: home, controls, motion, nozzle-temp, bed-temp, extrusion, filament, settings, advanced, print-select\n");
            return 1;
        }
    }

    printf("GuppyScreen UI Prototype\n");
    printf("========================\n");
    printf("Target: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("Nav Width: %d pixels\n", UI_NAV_WIDTH(SCREEN_WIDTH));
    printf("Initial Panel: %d\n", initial_panel);
    printf("\n");

    // Initialize LVGL (handles SDL internally)
    if (!init_lvgl()) {
        return 1;
    }

    // Create main screen
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, UI_COLOR_PANEL_BG, LV_PART_MAIN);

    // Register fonts and images for XML (must be done before loading components)
    LV_LOG_USER("Registering fonts and images...");
    lv_xml_register_font(NULL, "fa_icons_64", &fa_icons_64);
    lv_xml_register_font(NULL, "fa_icons_48", &fa_icons_48);
    lv_xml_register_font(NULL, "fa_icons_32", &fa_icons_32);
    lv_xml_register_font(NULL, "fa_icons_16", &fa_icons_16);
    lv_xml_register_font(NULL, "diagonal_arrows_40", &diagonal_arrows_40);
    lv_xml_register_font(NULL, "montserrat_14", &lv_font_montserrat_14);
    lv_xml_register_font(NULL, "montserrat_16", &lv_font_montserrat_16);
    lv_xml_register_font(NULL, "montserrat_20", &lv_font_montserrat_20);
    lv_xml_register_font(NULL, "montserrat_28", &lv_font_montserrat_28);
    lv_xml_register_font(NULL, "montserrat_48", &lv_font_montserrat_48);
    lv_xml_register_image(NULL, "A:assets/images/printer_400.png",
                          "A:assets/images/printer_400.png");
    lv_xml_register_image(NULL, "filament_spool",
                          "A:assets/images/filament_spool.png");
    lv_xml_register_image(NULL, "A:assets/images/placeholder_thumb_centered.png",
                          "A:assets/images/placeholder_thumb_centered.png");

    // Register XML components (globals first to make constants available)
    LV_LOG_USER("Registering XML components...");
    lv_xml_component_register_from_file("A:ui_xml/globals.xml");
    lv_xml_component_register_from_file("A:ui_xml/header_bar.xml");
    lv_xml_component_register_from_file("A:ui_xml/confirmation_dialog.xml");
    lv_xml_component_register_from_file("A:ui_xml/numeric_keypad_modal.xml");
    lv_xml_component_register_from_file("A:ui_xml/print_file_card.xml");
    lv_xml_component_register_from_file("A:ui_xml/print_file_list_row.xml");
    lv_xml_component_register_from_file("A:ui_xml/print_file_detail.xml");
    lv_xml_component_register_from_file("A:ui_xml/navigation_bar.xml");
    lv_xml_component_register_from_file("A:ui_xml/home_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/controls_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/motion_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/nozzle_temp_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/bed_temp_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/extrusion_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/filament_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/settings_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/advanced_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/print_select_panel.xml");
    lv_xml_component_register_from_file("A:ui_xml/app_layout.xml");

    // Initialize reactive subjects BEFORE creating XML
    LV_LOG_USER("Initializing reactive subjects...");
    ui_nav_init();  // Navigation system (icon colors, active panel)
    ui_panel_home_init_subjects();  // Home panel data bindings
    ui_panel_print_select_init_subjects();  // Print select panel (none yet)
    ui_panel_controls_init_subjects();  // Controls panel launcher
    ui_panel_motion_init_subjects();  // Motion sub-screen position display
    ui_panel_controls_temp_init_subjects();  // Temperature sub-screens
    ui_panel_controls_extrusion_init_subjects();  // Extrusion sub-screen

    // Create entire UI from XML (single component contains everything)
    lv_obj_t* app_layout = (lv_obj_t*)lv_xml_create(screen, "app_layout", NULL);

    // Find navbar and panel widgets
    // app_layout > navbar (child 0), content_area (child 1)
    lv_obj_t* navbar = lv_obj_get_child(app_layout, 0);
    lv_obj_t* content_area = lv_obj_get_child(app_layout, 1);

    // Wire up navigation button click handlers and trigger initial color update
    ui_nav_wire_events(navbar);

    // Find all panel widgets in content area
    lv_obj_t* panels[UI_PANEL_COUNT];
    for (int i = 0; i < UI_PANEL_COUNT; i++) {
        panels[i] = lv_obj_get_child(content_area, i);
    }

    // Register panels with navigation system for show/hide management
    ui_nav_set_panels(panels);

    // Setup home panel observers (panels[0] is home panel)
    ui_panel_home_setup_observers(panels[0]);

    // Setup controls panel (wire launcher card click handlers)
    ui_panel_controls_set(panels[UI_PANEL_CONTROLS]);
    ui_panel_controls_wire_events(panels[UI_PANEL_CONTROLS]);

    // Setup print select panel (wires up events, creates overlays, populates data)
    ui_panel_print_select_setup(panels[UI_PANEL_PRINT_SELECT]);
    ui_panel_print_select_populate_test_data(panels[UI_PANEL_PRINT_SELECT]);

    // Initialize numeric keypad modal component (creates reusable keypad widget)
    ui_keypad_init(screen);

    LV_LOG_USER("XML UI created successfully with reactive navigation");

    // Switch to initial panel (if different from default HOME)
    if (initial_panel != UI_PANEL_HOME) {
        ui_nav_set_active((ui_panel_id_t)initial_panel);
        printf("Switched to panel %d\n", initial_panel);
    }

    // Force a few render cycles to ensure panel switch and layout complete
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        SDL_Delay(10);
    }

    // Keypad is initialized and ready to be shown when controls panel buttons are clicked

    // Special case: Show motion panel if requested
    if (show_motion) {
        printf("Creating and showing motion sub-screen...\n");

        // Create motion panel
        lv_obj_t* motion_panel = (lv_obj_t*)lv_xml_create(screen, "motion_panel", nullptr);
        if (motion_panel) {
            ui_panel_motion_setup(motion_panel, screen);

            // Hide controls launcher, show motion panel
            lv_obj_add_flag(panels[UI_PANEL_CONTROLS], LV_OBJ_FLAG_HIDDEN);

            // Set mock position data
            ui_panel_motion_set_position(120.5f, 105.2f, 15.8f);

            printf("Motion panel displayed\n");
        }
    }

    // Special case: Show nozzle temp panel if requested
    if (show_nozzle_temp) {
        printf("Creating and showing nozzle temperature sub-screen...\n");

        // Create nozzle temp panel
        lv_obj_t* nozzle_temp_panel = (lv_obj_t*)lv_xml_create(screen, "nozzle_temp_panel", nullptr);
        if (nozzle_temp_panel) {
            ui_panel_controls_temp_nozzle_setup(nozzle_temp_panel, screen);

            // Hide controls launcher, show nozzle temp panel
            lv_obj_add_flag(panels[UI_PANEL_CONTROLS], LV_OBJ_FLAG_HIDDEN);

            // Set mock temperature data
            ui_panel_controls_temp_set_nozzle(25, 0);

            printf("Nozzle temp panel displayed\n");
        }
    }

    // Special case: Show bed temp panel if requested
    if (show_bed_temp) {
        printf("Creating and showing bed temperature sub-screen...\n");

        // Create bed temp panel
        lv_obj_t* bed_temp_panel = (lv_obj_t*)lv_xml_create(screen, "bed_temp_panel", nullptr);
        if (bed_temp_panel) {
            ui_panel_controls_temp_bed_setup(bed_temp_panel, screen);

            // Hide controls launcher, show bed temp panel
            lv_obj_add_flag(panels[UI_PANEL_CONTROLS], LV_OBJ_FLAG_HIDDEN);

            // Set mock temperature data
            ui_panel_controls_temp_set_bed(25, 0);

            printf("Bed temp panel displayed\n");
        }
    }

    // Special case: Show extrusion panel if requested
    if (show_extrusion) {
        printf("Creating and showing extrusion sub-screen...\n");

        // Create extrusion panel
        lv_obj_t* extrusion_panel = (lv_obj_t*)lv_xml_create(screen, "extrusion_panel", nullptr);
        if (extrusion_panel) {
            ui_panel_controls_extrusion_setup(extrusion_panel, screen);

            // Hide controls launcher, show extrusion panel
            lv_obj_add_flag(panels[UI_PANEL_CONTROLS], LV_OBJ_FLAG_HIDDEN);

            // Set mock temperature data (nozzle at room temp)
            ui_panel_controls_extrusion_set_temp(25, 0);

            printf("Extrusion panel displayed\n");
        }
    }

    // Auto-screenshot timer (2 seconds after UI creation)
    uint32_t screenshot_time = SDL_GetTicks() + 2000;
    bool screenshot_taken = false;

    // Main event loop - Let LVGL handle SDL events internally via lv_timer_handler()
    // Loop continues while display exists (exits when window closed)
    while (lv_display_get_next(NULL)) {
        // Auto-screenshot after 2 seconds
        if (!screenshot_taken && SDL_GetTicks() >= screenshot_time) {
            save_screenshot();
            screenshot_taken = true;
        }

        // Run LVGL tasks - internally polls SDL events and processes input
        lv_timer_handler();
        fflush(stdout);
        SDL_Delay(5);  // Small delay to prevent 100% CPU usage
    }

    // Cleanup
    LV_LOG_USER("Shutting down...");
    lv_deinit();  // LVGL handles SDL cleanup internally

    return 0;
}

