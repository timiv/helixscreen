// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file keyboard_shortcuts.h
 * @brief Keyboard shortcut registration and processing
 *
 * Provides a declarative API for keyboard shortcuts with debouncing.
 * Decouples shortcut logic from SDL for testability.
 */

#pragma once

#include <functional>
#include <vector>

namespace helix::input {

/**
 * @brief Keyboard shortcut registry with edge-triggered debouncing
 *
 * Usage:
 * 1. Register shortcuts at startup
 * 2. Call process() each frame with key state provider
 * 3. Actions fire on key press edge (not repeat)
 */
class KeyboardShortcuts {
  public:
    using Action = std::function<void()>;
    using Condition = std::function<bool()>;
    using KeyStateProvider = std::function<bool(int scancode)>;

    /**
     * @brief Register a simple key binding
     *
     * @param scancode SDL scancode (e.g., SDL_SCANCODE_M)
     * @param action Function to call on key press
     */
    void register_key(int scancode, Action action);

    /**
     * @brief Register a conditional key binding
     *
     * Action only fires if condition returns true at press time.
     *
     * @param scancode SDL scancode
     * @param action Function to call on key press
     * @param condition Predicate checked before firing
     */
    void register_key_if(int scancode, Action action, Condition condition);

    /**
     * @brief Register a modifier+key combo
     *
     * @param modifiers Required modifier mask (e.g., KMOD_GUI)
     * @param scancode SDL scancode
     * @param action Function to call on combo press
     */
    void register_combo(int modifiers, int scancode, Action action);

    /**
     * @brief Process keyboard state and fire actions
     *
     * Call once per frame. Uses edge detection to fire actions
     * only on key press, not while held.
     *
     * @param is_key_pressed Function that returns true if scancode is pressed
     * @param current_modifiers Current modifier key state
     * @param suppress_plain_keys When true, skip non-combo shortcuts (e.g., when a text input has
     * focus)
     */
    void process(KeyStateProvider is_key_pressed, int current_modifiers,
                 bool suppress_plain_keys = false);

    /**
     * @brief Remove all registered shortcuts
     */
    void clear();

  private:
    struct Binding {
        int scancode;
        int modifiers; // 0 for no modifier requirement
        Action action;
        Condition condition;
        bool was_pressed{false};
    };

    std::vector<Binding> m_bindings;
};

} // namespace helix::input
