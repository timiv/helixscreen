// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform_info.h"

namespace helix {

// -1 = use compile-time default, 0 = force non-Android, 1 = force Android
static int s_platform_override = -1;

bool is_android_platform() {
    if (s_platform_override >= 0) {
        return s_platform_override != 0;
    }
#ifdef __ANDROID__
    return true;
#else
    return false;
#endif
}

void set_platform_override(int override_value) {
    s_platform_override = override_value;
}

} // namespace helix
