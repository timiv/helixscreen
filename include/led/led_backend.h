// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace helix::led {

enum class LedBackendType { NATIVE, LED_EFFECT, WLED, MACRO };

struct LedStripInfo {
    std::string name; // Display name
    std::string id;   // Klipper/Moonraker ID (e.g., "neopixel chamber_light")
    LedBackendType backend;
    bool supports_color; // RGB/RGBW capable
    bool supports_white; // Has W channel (RGBW)
};

struct LedEffectInfo {
    std::string name;         // Klipper config name (e.g., "led_effect breathing")
    std::string display_name; // Human-friendly (e.g., "Breathing")
    std::string icon_hint;    // Icon name for card (e.g., "air", "local_fire_department")
    std::vector<std::string>
        target_leds; // Strip IDs this effect targets (e.g., "neopixel chamber_light")
    bool enabled =
        false; // Whether this effect is currently active (tracked via Moonraker subscription)
};

enum class MacroLedType { ON_OFF, TOGGLE, PRESET };

struct LedMacroInfo {
    std::string display_name;                                 // User-friendly label
    MacroLedType type = MacroLedType::TOGGLE;                 // Control style
    std::string on_macro;                                     // ON_OFF type: gcode to turn on
    std::string off_macro;                                    // ON_OFF type: gcode to turn off
    std::string toggle_macro;                                 // TOGGLE type: single toggle macro
    std::vector<std::pair<std::string, std::string>> presets; // PRESET type: {name, macro}
};

/// WLED preset info fetched from device
struct WledPresetInfo {
    int id = -1;
    std::string name;
};

/// WLED strip runtime state (from Moonraker status polling)
struct WledStripState {
    bool is_on = false;
    int brightness = 255;   // 0-255
    int active_preset = -1; // -1 = no preset active
};

} // namespace helix::led
