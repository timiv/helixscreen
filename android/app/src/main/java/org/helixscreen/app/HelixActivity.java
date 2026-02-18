package org.helixscreen.app;

import org.libsdl.app.SDLActivity;

/**
 * HelixScreen Android activity.
 * Extends SDLActivity to provide the SDL2 + native code bridge.
 */
public class HelixActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[]{
            "SDL2",
            "main"
        };
    }

    @Override
    protected String[] getArguments() {
        // Pass debug verbosity in debug builds
        if (BuildConfig.DEBUG) {
            return new String[]{"-vv"};
        }
        return new String[]{};
    }
}
