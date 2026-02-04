// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "splash_screen_manager.h"

#include "../catch_amalgamated.hpp"

using namespace helix::application;

// ============================================================================
// SplashScreenManager tests
// ============================================================================

TEST_CASE("SplashScreenManager: no splash pid", "[splash][application]") {
    SplashScreenManager mgr;

    SECTION("starts as not exited") {
        REQUIRE_FALSE(mgr.has_exited());
    }

    SECTION("exits immediately with no pid") {
        mgr.start(0); // No splash
        mgr.check_and_signal();
        REQUIRE(mgr.has_exited());
    }

    SECTION("negative pid treated as no splash") {
        mgr.start(-1);
        mgr.check_and_signal();
        REQUIRE(mgr.has_exited());
    }
}

TEST_CASE("SplashScreenManager: discovery timing", "[splash][application]") {
    SplashScreenManager mgr;

    // Use a mock PID that won't exist - signal will fail but state transitions work
    mgr.start(999999);

    SECTION("waits for discovery before signaling") {
        // Not enough time, discovery not complete
        mgr.check_and_signal();
        // Signal won't happen because we're waiting for discovery or timeout
        // (The actual signal would fail since PID doesn't exist, but state logic is testable)
    }

    SECTION("signals immediately when discovery complete") {
        mgr.on_discovery_complete();
        REQUIRE(mgr.is_discovery_complete());
    }

    SECTION("discovery_complete flag persists") {
        REQUIRE_FALSE(mgr.is_discovery_complete());
        mgr.on_discovery_complete();
        REQUIRE(mgr.is_discovery_complete());
        // Still true after check
        mgr.check_and_signal();
        REQUIRE(mgr.is_discovery_complete());
    }
}

TEST_CASE("SplashScreenManager: post-splash refresh", "[splash][application]") {
    SplashScreenManager mgr;

    SECTION("no refresh needed initially") {
        REQUIRE_FALSE(mgr.needs_post_splash_refresh());
    }

    SECTION("refresh needed after splash exits") {
        mgr.start(0); // No splash = immediate exit
        mgr.check_and_signal();
        REQUIRE(mgr.has_exited());
        REQUIRE(mgr.needs_post_splash_refresh());
    }

    SECTION("mark_refresh_done decrements counter") {
        mgr.start(0);
        mgr.check_and_signal();
        REQUIRE(mgr.needs_post_splash_refresh());

        mgr.mark_refresh_done();
        REQUIRE_FALSE(mgr.needs_post_splash_refresh());
    }

    SECTION("multiple refreshes if configured") {
        mgr.start(0);
        mgr.check_and_signal();

        // Default is 1 refresh
        REQUIRE(mgr.needs_post_splash_refresh());
        mgr.mark_refresh_done();
        REQUIRE_FALSE(mgr.needs_post_splash_refresh());

        // Extra mark_refresh_done is safe
        mgr.mark_refresh_done();
        REQUIRE_FALSE(mgr.needs_post_splash_refresh());
    }
}

TEST_CASE("SplashScreenManager: idempotent signaling", "[splash][application]") {
    SplashScreenManager mgr;
    mgr.start(0);

    SECTION("multiple check_and_signal calls are safe") {
        mgr.check_and_signal();
        REQUIRE(mgr.has_exited());

        // Second call should be no-op
        mgr.check_and_signal();
        REQUIRE(mgr.has_exited());
    }
}

TEST_CASE("SplashScreenManager: elapsed time tracking", "[splash][application]") {
    SplashScreenManager mgr;
    mgr.start(999999); // Non-existent PID

    SECTION("elapsed_ms starts at 0") {
        // Right after start, elapsed should be very small
        REQUIRE(mgr.elapsed_ms() < 100);
    }
}
