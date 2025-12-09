// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_observer_guard.h"

namespace helix {

/**
 * @brief Initialize print completion notification system
 *
 * Registers an observer on PrinterState's print_state_enum subject that
 * triggers toast or modal notifications when prints complete, are cancelled,
 * or fail. Uses SettingsManager to determine notification mode.
 *
 * @return ObserverGuard that manages the observer's lifetime
 */
ObserverGuard init_print_completion_observer();

} // namespace helix
