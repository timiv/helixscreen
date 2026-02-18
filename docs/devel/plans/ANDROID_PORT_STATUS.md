# Android Port Status

**Branch:** `feature/android-port`
**Worktree:** `.worktrees/android-port/`
**Last updated:** 2026-02-13

## Current State: First Successful APK Build

The Android port compiles, links, and packages into an APK. It has **not been tested on a device or emulator yet**.

**APK:** `android/app/build/outputs/apk/debug/app-debug.apk` (~186MB debug, arm64-v8a + x86_64)

## What's Been Done

### Phase 1-3: Platform Conditionalization
- `HELIX_PLATFORM_ANDROID` and `__ANDROID__` guards in 12 source files
- Network backends (WiFi, Ethernet, USB) return nullptr on Android — OS manages these
- Crash handler uses `android/log.h` instead of `execinfo.h` (no `backtrace()` on Android)
- Platform detection via `platform_info.cpp` with test override support
- Unit tests for platform detection and asset extraction

### Phase 4: Android Build System
- Gradle wrapper (8.9) + AGP 8.5.1 + JDK 21
- `android/app/jni/CMakeLists.txt` — builds all deps from source (SDL2, LVGL, libhv, TinyGL, lv_markdown, spdlog)
- `HelixActivity.java` — minimal SDL2 Activity bridge, passes `-vv` in debug builds
- AndroidManifest: landscape, fullscreen, singleInstance, INTERNET + ACCESS_NETWORK_STATE
- Asset pipeline: Gradle `copyAssets` task bundles ui_xml/, assets/, config/ into APK
- `android_asset_extractor.cpp` — extracts bundled assets to internal storage on first launch, version-gated

### Phase 5: Build Fixes (This Session)
Fixed 6 categories of build errors to get from "compiles nothing" to "APK packages":

| Issue | Fix |
|-------|-----|
| `lvgl_assert_handler.h` not found | Added `include/` to lvgl + lv_markdown include paths |
| `lvgl/lvgl.h` not found | Added `lib/` to main include paths (matches Makefile's `-isystem lib`) |
| `json.hpp` not found | Added `lib/libhv/cpputil` to include paths |
| `drivers/sdl/lv_sdl_window.h` not found | Added `lib/lvgl/src` to include paths |
| `stb_image.h` not found | Added `lib/tinygl/include-demo` to include paths |
| `HELIX_VERSION` undefined | Added version parsing from VERSION.txt in CMake |
| LVGL SDL driver compiled out | Added `HELIX_DISPLAY_SDL` define + SDL2 link to lvgl target |
| `aligned_alloc` unavailable on API 21 | Bumped minSdkVersion to 28 |
| `-std=c++17` applied to C files | Generator expression: `$<$<COMPILE_LANGUAGE:CXX>:-std=c++17>` |
| TinyGL missing zdither.c | Build from source globs instead of its CMakeLists.txt |

Also merged `main` into the branch to pick up libhv fixes.

## Build Instructions

```bash
# Prerequisites
# - Android Studio (for JBR JDK 21)
# - Android SDK: platforms;android-34, build-tools;34.0.0, platform-tools
# - NDK r29: /usr/local/Caskroom/android-ndk/29/AndroidNDK14206865.app/Contents/NDK
# - android/local.properties with sdk.dir and ndk.dir

# Build APK
cd .worktrees/android-port/android
export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"
export ANDROID_SDK_ROOT="$HOME/Library/Android/sdk"
export ANDROID_NDK_HOME="/usr/local/Caskroom/android-ndk/29/AndroidNDK14206865.app/Contents/NDK"
export PATH="/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:$JAVA_HOME/bin:$PATH"
./gradlew assembleDebug

# Install on device/emulator
adb install app/build/outputs/apk/debug/app-debug.apk

# Verify macOS build still works (from worktree root)
cd .worktrees/android-port
make -j && make test-run
```

## Architecture

```
HelixActivity.java (SDLActivity subclass)
  → loads libSDL2.so + libmain.so
  → SDL2 calls main() in main.cpp
  → android_extract_assets_if_needed()
    → copies APK assets/ → internal storage
    → sets HELIX_DATA_DIR env var
  → Application::run() — same as desktop
  → SDL2 display backend (software rendering)
```

**Key design decisions:**
- SDL2 handles JNI bridge, window/input, audio — minimal Java code
- Same LVGL rendering path as desktop (SDL2 backend)
- Assets extracted to writable storage so LVGL XML loader works unchanged
- Network backends (WiFi/Ethernet/USB) gracefully return nullptr — Android manages networking

## What's Left To Do

### Phase 6: First Launch on Device/Emulator (Next)

1. **Test on Android emulator** — create an API 28+ x86_64 AVD, install APK, see what happens
2. **Test on physical device** — `adb install`, check logcat for crashes
3. **Asset extraction debugging** — the current extractor is MVP; it copies from a source dir but the APK extraction path via SDL_RWops is still TODO
4. **Fix runtime crashes** — expect issues with:
   - Asset paths not resolving correctly
   - Missing filesystem paths that exist on Linux/macOS
   - SDL2 display initialization differences on Android
   - Touch input mapping

### Phase 7: Runtime Fixes

Expected issues to address:

- **Asset extraction**: Current code uses filesystem copy from a source directory. On Android, assets are inside the APK (a zip file). Need SDL_RWops or AssetManager JNI to read them. The `copyAssets` Gradle task puts files in `src/main/assets/` which SDL2 can read via `SDL_RWFromFile()` with the `"assets/"` prefix — but the extraction code may need updating.
- **Data directory paths**: Settings, printer_database.json, logs — need to write to `SDL_AndroidGetInternalStoragePath()`, not `~/helixscreen/config/`
- **Display resolution**: Fixed landscape but need to handle different screen sizes/densities
- **Sound**: SDL2_mixer or native audio — current PWM/M300 backends won't work
- **Crash reporting**: Backtrace unavailable, need Android-specific crash collection
- **Permissions**: Runtime permission requests if any new ones needed

### Phase 8: Polish & Optimization

- **APK size reduction**: 186MB debug is huge. Release build + strip + split ABIs will help significantly
- **ProGuard/R8**: Minification for Java layer
- **App icon**: Currently generic Android icon
- **Splash screen**: Android 12+ splash screen API
- **Screen rotation**: Currently locked to landscape
- **Back button handling**: Android back gesture/button → navigation
- **Notifications**: Print status notifications via Android notification system
- **mDNS on Android**: Printer discovery — Android has NsdManager, not avahi

### Future Considerations

- **Google Play distribution**: Signing, app bundle format, listing
- **Tablet optimization**: Different layouts for tablet vs phone
- **Android Auto / Wear OS**: Probably not relevant
- **OTA updates**: Can't use the Linux update system — Play Store handles updates
- **Background service**: Keep WebSocket alive when app is backgrounded

## SDK/Tool Versions

| Component | Version |
|-----------|---------|
| Gradle | 8.9 |
| Android Gradle Plugin | 8.5.1 |
| Compile SDK | 34 (Android 14) |
| Min SDK | 28 (Android 9) |
| Target SDK | 34 (Android 14) |
| NDK | r29 (29.0.14206865) |
| JDK | 21 (Android Studio JBR) |
| C++ STL | c++_shared |
| ABIs | arm64-v8a, x86_64 |

## Files Changed (vs main)

```
android/                              # Entire Android build system (new)
  app/build.gradle                    # App config, SDK versions, CMake
  app/jni/CMakeLists.txt              # Native build — all libraries
  app/src/main/AndroidManifest.xml    # Permissions, activity config
  app/src/main/java/.../HelixActivity.java  # SDL2 Activity bridge
  app/src/main/res/                   # Android resources (icons, strings)
  build.gradle                        # Root Gradle config
  gradle/                             # Gradle wrapper
  settings.gradle                     # Module settings
  .gitignore                          # Ignore build outputs + extracted assets

include/android_asset_extractor.h     # Asset extraction API
include/platform_info.h               # Platform detection API

src/application/android_asset_extractor.cpp  # Asset extraction impl
src/application/application.cpp       # #ifdef __ANDROID__ for asset extraction
src/application/data_root_resolver.cpp # Path resolution
src/system/crash_handler.cpp          # Android crash logging
src/system/platform_capabilities.cpp  # Platform detection
src/system/platform_info.cpp          # is_android_platform()
src/api/wifi_backend.cpp              # Returns nullptr on Android
src/api/ethernet_backend.cpp          # Returns nullptr on Android
src/api/usb_backend.cpp               # Returns nullptr on Android

tests/unit/test_android_asset_extractor.cpp  # Asset extraction tests
tests/unit/test_android_platform.cpp  # Platform detection tests
```
