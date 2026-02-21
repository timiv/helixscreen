// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_update_queue.h"

namespace helix::ui {

class UpdateQueueTestAccess {
  public:
    static void drain(UpdateQueue& q) {
        q.process_pending();
    }

    /// Drain repeatedly until the queue is fully empty (handles nested queue_update calls)
    static void drain_all(UpdateQueue& q, int max_iterations = 10) {
        for (int i = 0; i < max_iterations; ++i) {
            {
                std::lock_guard<std::mutex> lock(q.mutex_);
                if (q.pending_.empty())
                    return;
            }
            q.process_pending();
        }
    }
};

} // namespace helix::ui

// Convenience alias for use with 'using namespace helix'
