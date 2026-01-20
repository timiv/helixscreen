// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "static_panel_registry.h"

#include <memory>

#define DEFINE_GLOBAL_PANEL(PanelType, global_var, getter_func)                                    \
    static std::unique_ptr<PanelType> global_var;                                                  \
    PanelType& getter_func() {                                                                     \
        if (!global_var) {                                                                         \
            global_var = std::make_unique<PanelType>();                                            \
            StaticPanelRegistry::instance().register_destroy(#PanelType,                           \
                                                             []() { global_var.reset(); });        \
        }                                                                                          \
        return *global_var;                                                                        \
    }
