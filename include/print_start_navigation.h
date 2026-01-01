// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_observer_guard.h"

namespace helix {

/**
 * @brief Initialize auto-navigation to print status panel on print start
 *
 * Registers an observer on PrinterState's print_state_enum subject that
 * automatically navigates to the print status panel when a print starts,
 * regardless of which panel the user is currently viewing.
 *
 * This handles the case where a print is started externally (via Mainsail,
 * OrcaSlicer, API, etc.) - the display reacts appropriately by showing
 * the print status overlay.
 *
 * Safe to call even when starting prints from the UI - the observer checks
 * if print status is already showing and won't double-navigate.
 *
 * @return ObserverGuard that manages the observer's lifetime
 */
ObserverGuard init_print_start_navigation_observer();

} // namespace helix
