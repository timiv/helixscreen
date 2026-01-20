// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl.h"

/**
 * @file ui_event_trampoline.h
 * @brief Macros to reduce boilerplate for LVGL event callback trampolines
 *
 * These macros define static member function trampolines that delegate to instance methods.
 * They are designed for out-of-class definitions of static member functions declared in headers.
 */

/**
 * @brief Define a static member function trampoline that delegates to an instance method
 *
 * Reduces the repetitive pattern:
 *   void Class::callback(lv_event_t* e) {
 *       auto* self = static_cast<Class*>(lv_event_get_user_data(e));
 *       if (self) { self->handler(e); }
 *   }
 *
 * To a single line:
 *   DEFINE_EVENT_TRAMPOLINE(Class, callback, handler)
 *
 * The handler method receives lv_event_t* as parameter.
 *
 * @param ClassName The class type to cast to
 * @param callback_name Name for the static callback function (will be prefixed with ClassName::)
 * @param handler_method Instance method to call (must take lv_event_t*)
 */
#define DEFINE_EVENT_TRAMPOLINE(ClassName, callback_name, handler_method)                          \
    void ClassName::callback_name(lv_event_t* e) {                                                 \
        auto* self = static_cast<ClassName*>(lv_event_get_user_data(e));                           \
        if (self) {                                                                                \
            self->handler_method(e);                                                               \
        }                                                                                          \
    }

/**
 * @brief Variant for handlers that don't need the event parameter
 *
 * For simple handlers like handle_click() that don't use the event:
 *   DEFINE_EVENT_TRAMPOLINE_SIMPLE(Class, on_click, handle_click)
 *
 * @param ClassName The class type to cast to
 * @param callback_name Name for the static callback function (will be prefixed with ClassName::)
 * @param handler_method Instance method to call (takes no parameters)
 */
#define DEFINE_EVENT_TRAMPOLINE_SIMPLE(ClassName, callback_name, handler_method)                   \
    void ClassName::callback_name(lv_event_t* e) {                                                 \
        auto* self = static_cast<ClassName*>(lv_event_get_user_data(e));                           \
        if (self) {                                                                                \
            self->handler_method();                                                                \
        }                                                                                          \
    }

/**
 * @brief Trampoline for singleton/global instance patterns
 *
 * For overlays using getter functions:
 *   DEFINE_SINGLETON_TRAMPOLINE(Overlay, on_click, get_overlay, handle_click)
 *
 * @param ClassName The class type (for documentation purposes)
 * @param callback_name Name for the static callback function
 * @param getter_func Function that returns reference to the singleton instance
 * @param handler_method Instance method to call (takes lv_event_t*)
 */
#define DEFINE_SINGLETON_TRAMPOLINE(ClassName, callback_name, getter_func, handler_method)         \
    static void callback_name(lv_event_t* e) {                                                     \
        auto& self = getter_func();                                                                \
        self.handler_method(e);                                                                    \
    }
