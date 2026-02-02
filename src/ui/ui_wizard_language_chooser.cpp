// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_wizard_language_chooser.h"

#include "config.h"
#include "settings_manager.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <memory>

// ============================================================================
// Welcome Translations
// ============================================================================

// Welcome text in each supported language (cycles during animation)
static const char* WELCOME_TRANSLATIONS[] = {
    "Welcome!",          // en
    "Willkommen!",       // de
    "Bienvenue!",        // fr
    "¡Bienvenido!",      // es
    "Добро пожаловать!", // ru
    "Bem-vindo!",        // pt
    "Benvenuto!",        // it
    "欢迎！",            // zh
    "ようこそ！",        // ja
};
static constexpr int WELCOME_COUNT = 9;

// Language codes for saving to config (matches button order in XML)
static const char* LANGUAGE_CODES[] = {"en", "de", "fr", "es", "ru", "pt", "it", "zh", "ja"};

// Timer period for cycling welcome text
static constexpr uint32_t WELCOME_CYCLE_MS = 2500;

// Animation durations
static constexpr int32_t CROSSFADE_DURATION_MS = 150;

// ============================================================================
// Global Instance
// ============================================================================

static std::unique_ptr<WizardLanguageChooserStep> g_wizard_language_chooser_step;

WizardLanguageChooserStep* get_wizard_language_chooser_step() {
    if (!g_wizard_language_chooser_step) {
        g_wizard_language_chooser_step = std::make_unique<WizardLanguageChooserStep>();
        StaticPanelRegistry::instance().register_destroy(
            "WizardLanguageChooserStep", []() { g_wizard_language_chooser_step.reset(); });
    }
    return g_wizard_language_chooser_step.get();
}

void destroy_wizard_language_chooser_step() {
    g_wizard_language_chooser_step.reset();
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

WizardLanguageChooserStep::WizardLanguageChooserStep() {
    spdlog::debug("[{}] Instance created", get_name());
}

WizardLanguageChooserStep::~WizardLanguageChooserStep() {
    // Timer guard handles cleanup automatically via RAII

    // Deinitialize subjects to disconnect observers before destruction
    if (subjects_initialized_) {
        lv_subject_deinit(&welcome_text_);
        subjects_initialized_ = false;
    }

    screen_root_ = nullptr;
}

// ============================================================================
// Move Semantics
// ============================================================================

WizardLanguageChooserStep::WizardLanguageChooserStep(WizardLanguageChooserStep&& other) noexcept
    : screen_root_(other.screen_root_),
      // NOTE: Do NOT copy lv_subject_t members - they contain internal pointers.
      // Leave welcome_text_ default-initialized.
      cycle_timer_(std::move(other.cycle_timer_)),
      current_welcome_index_(other.current_welcome_index_),
      subjects_initialized_(false), // Subjects stay with moved-from object
      language_selected_(other.language_selected_) {
    std::memcpy(welcome_buffer_, other.welcome_buffer_, sizeof(welcome_buffer_));
    other.screen_root_ = nullptr;
    other.current_welcome_index_ = 0;
    other.language_selected_ = false;
}

WizardLanguageChooserStep&
WizardLanguageChooserStep::operator=(WizardLanguageChooserStep&& other) noexcept {
    if (this != &other) {
        // Deinit our subjects if they were initialized
        if (subjects_initialized_) {
            lv_subject_deinit(&welcome_text_);
            subjects_initialized_ = false;
        }

        screen_root_ = other.screen_root_;
        std::memcpy(welcome_buffer_, other.welcome_buffer_, sizeof(welcome_buffer_));
        cycle_timer_ = std::move(other.cycle_timer_);
        current_welcome_index_ = other.current_welcome_index_;
        language_selected_ = other.language_selected_;

        other.screen_root_ = nullptr;
        other.current_welcome_index_ = 0;
        other.language_selected_ = false;
    }
    return *this;
}

// ============================================================================
// Subject Initialization
// ============================================================================

void WizardLanguageChooserStep::init_subjects() {
    if (subjects_initialized_) {
        spdlog::debug("[{}] Subjects already initialized", get_name());
        return;
    }

    spdlog::debug("[{}] Initializing subjects", get_name());

    // Initialize welcome text subject with string buffer
    strncpy(welcome_buffer_, WELCOME_TRANSLATIONS[0], sizeof(welcome_buffer_) - 1);
    welcome_buffer_[sizeof(welcome_buffer_) - 1] = '\0';
    lv_subject_init_string(&welcome_text_, welcome_buffer_, nullptr, sizeof(welcome_buffer_),
                           welcome_buffer_);
    lv_xml_register_subject(nullptr, "wizard_welcome_text", &welcome_text_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

// ============================================================================
// Callback Registration
// ============================================================================

static void on_language_selected(lv_event_t* e) {
    // Get the language index from user_data (set in XML via event_cb user_data attribute)
    intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(e));

    if (index < 0 || index >= WELCOME_COUNT) {
        spdlog::warn("[Wizard Language Chooser] Invalid language index: {}", index);
        return;
    }

    spdlog::info("[Wizard Language Chooser] Language selected: {} ({})", LANGUAGE_CODES[index],
                 WELCOME_TRANSLATIONS[index]);

    // Save language to config
    Config* cfg = Config::get_instance();
    if (cfg) {
        cfg->set<std::string>("/language", LANGUAGE_CODES[index]);
        cfg->save();
        spdlog::debug("[Wizard Language Chooser] Saved language '{}' to config",
                      LANGUAGE_CODES[index]);
    }

    // Update step state
    WizardLanguageChooserStep* step = get_wizard_language_chooser_step();
    if (step) {
        step->stop_cycle_timer();
        step->set_language_selected(true);
    }
}

void WizardLanguageChooserStep::register_callbacks() {
    spdlog::debug("[{}] Registering callbacks", get_name());

    lv_xml_register_event_cb(nullptr, "on_language_selected", on_language_selected);
}

// ============================================================================
// Welcome Text Cycling
// ============================================================================

void WizardLanguageChooserStep::cycle_timer_cb(lv_timer_t* timer) {
    WizardLanguageChooserStep* step =
        static_cast<WizardLanguageChooserStep*>(lv_timer_get_user_data(timer));
    if (step) {
        step->cycle_welcome_text();
    }
}

void WizardLanguageChooserStep::cycle_welcome_text() {
    // Advance to next language
    current_welcome_index_ = (current_welcome_index_ + 1) % WELCOME_COUNT;
    const char* new_text = WELCOME_TRANSLATIONS[current_welcome_index_];

    spdlog::trace("[{}] Cycling to welcome text: {}", get_name(), new_text);

    // Animate the crossfade
    animate_crossfade(new_text);
}

void WizardLanguageChooserStep::animate_crossfade(const char* new_text) {
    if (!screen_root_) {
        return;
    }

    lv_obj_t* welcome_header = lv_obj_find_by_name(screen_root_, "welcome_header");
    if (!welcome_header) {
        // Fallback: just update the subject directly
        lv_subject_copy_string(&welcome_text_, new_text);
        return;
    }

    // Check if animations are enabled
    if (!SettingsManager::instance().get_animations_enabled()) {
        // No animation: just update the text immediately
        lv_subject_copy_string(&welcome_text_, new_text);
        return;
    }

    // Store new text for the completion callback
    // We capture it via the subject update in the completion callback
    struct CrossfadeCtx {
        WizardLanguageChooserStep* step;
        const char* new_text;
    };

    // Allocate context (will be cleaned up in completion callback)
    auto* ctx = new CrossfadeCtx{this, new_text};

    // Fade out animation (opacity: current -> 0)
    lv_anim_t fade_out;
    lv_anim_init(&fade_out);
    lv_anim_set_var(&fade_out, welcome_header);
    lv_anim_set_values(&fade_out, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&fade_out, CROSSFADE_DURATION_MS);
    lv_anim_set_path_cb(&fade_out, lv_anim_path_ease_in);
    lv_anim_set_exec_cb(&fade_out, [](void* obj, int32_t value) {
        lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(value),
                             LV_PART_MAIN);
    });
    lv_anim_set_user_data(&fade_out, ctx);
    lv_anim_set_completed_cb(&fade_out, [](lv_anim_t* anim) {
        auto* ctx = static_cast<CrossfadeCtx*>(anim->user_data);
        lv_obj_t* header = static_cast<lv_obj_t*>(anim->var);

        if (ctx && ctx->step) {
            // Update the text while invisible
            lv_subject_copy_string(ctx->step->get_welcome_text_subject(), ctx->new_text);

            // Fade in animation (opacity: 0 -> 255)
            lv_anim_t fade_in;
            lv_anim_init(&fade_in);
            lv_anim_set_var(&fade_in, header);
            lv_anim_set_values(&fade_in, LV_OPA_TRANSP, LV_OPA_COVER);
            lv_anim_set_duration(&fade_in, CROSSFADE_DURATION_MS);
            lv_anim_set_path_cb(&fade_in, lv_anim_path_ease_out);
            lv_anim_set_exec_cb(&fade_in, [](void* obj, int32_t value) {
                lv_obj_set_style_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(value),
                                     LV_PART_MAIN);
            });
            lv_anim_start(&fade_in);
        }

        // Clean up context
        delete ctx;
    });
    lv_anim_start(&fade_out);
}

// ============================================================================
// Screen Creation
// ============================================================================

lv_obj_t* WizardLanguageChooserStep::create(lv_obj_t* parent) {
    spdlog::debug("[{}] Creating language chooser screen", get_name());

    // Safety check: cleanup should have been called by wizard navigation
    if (screen_root_) {
        spdlog::warn("[{}] Screen pointer not null - cleanup may not have been called properly",
                     get_name());
        screen_root_ = nullptr; // Reset pointer, wizard framework handles deletion
    }

    // Create screen from XML
    screen_root_ =
        static_cast<lv_obj_t*>(lv_xml_create(parent, "wizard_language_chooser", nullptr));
    if (!screen_root_) {
        spdlog::error("[{}] Failed to create screen from XML", get_name());
        return nullptr;
    }

    // Start the welcome text cycling timer
    lv_timer_t* timer = lv_timer_create(cycle_timer_cb, WELCOME_CYCLE_MS, this);
    cycle_timer_.reset(timer);
    spdlog::debug("[{}] Started welcome text cycle timer ({}ms)", get_name(), WELCOME_CYCLE_MS);

    spdlog::debug("[{}] Screen created successfully", get_name());
    return screen_root_;
}

// ============================================================================
// Cleanup
// ============================================================================

void WizardLanguageChooserStep::cleanup() {
    spdlog::debug("[{}] Cleaning up resources", get_name());

    // Stop the cycling timer
    cycle_timer_.reset();

    // Reset UI references
    // Note: Do NOT call lv_obj_del() here - the wizard framework handles
    // object deletion when clearing wizard_content container
    screen_root_ = nullptr;

    spdlog::debug("[{}] Cleanup complete", get_name());
}

// ============================================================================
// Validation
// ============================================================================

bool WizardLanguageChooserStep::is_validated() const {
    return language_selected_;
}

// ============================================================================
// Skip Logic
// ============================================================================

bool WizardLanguageChooserStep::should_skip() const {
    Config* cfg = Config::get_instance();
    if (!cfg) {
        return false;
    }

    // Check if language has already been set in config
    std::string saved_language = cfg->get<std::string>("/language", "");

    // Skip if a language has been explicitly set (not empty and not just default "en")
    // We show the step for first-time setup even if no language is set
    if (!saved_language.empty() && saved_language != "en") {
        spdlog::info("[{}] Language already set to '{}', skipping step", get_name(),
                     saved_language);
        return true;
    }

    // Also skip if language is "en" but was explicitly set (check for wizard completion)
    // For now, we check if the wizard was already completed
    bool wizard_complete = !cfg->is_wizard_required();
    if (wizard_complete && !saved_language.empty()) {
        spdlog::info("[{}] Wizard complete and language set to '{}', skipping step", get_name(),
                     saved_language);
        return true;
    }

    spdlog::debug("[{}] No language preference saved, showing step", get_name());
    return false;
}
