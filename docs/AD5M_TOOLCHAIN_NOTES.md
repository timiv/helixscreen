# AD5M Toolchain & glibc Compatibility Notes

## Project Overview

**HelixScreen** is a touchscreen UI for Klipper 3D printers, built with LVGL 9.4.
It uses declarative XML layouts with reactive data binding. The UI communicates
with printers via Moonraker's WebSocket API.

### Build System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         Makefile                                 │
│  - Entry point, platform detection, high-level targets          │
└───────────────────────────┬─────────────────────────────────────┘
                            │ includes
        ┌───────────────────┼───────────────────┐
        ▼                   ▼                   ▼
┌───────────────┐   ┌───────────────┐   ┌───────────────┐
│  mk/cross.mk  │   │  mk/deps.mk   │   │  mk/rules.mk  │
│ Platform cfg  │   │ Submodule     │   │ Compile/link  │
│ (pi, ad5m,    │   │ builds        │   │ rules         │
│  native)      │   │ (libhv, wpa,  │   │               │
│               │   │  tinygl)      │   │               │
└───────────────┘   └───────────────┘   └───────────────┘
        │
        ▼
┌───────────────────────────────────────────────────────────────┐
│  docker/Dockerfile.{pi,ad5m}                                   │
│  Cross-compilation toolchains in Docker containers            │
│  - Pi: aarch64-linux-gnu (64-bit ARM, Raspberry Pi 3/4/5)    │
│  - AD5M: arm-none-linux-gnueabihf (32-bit ARM, Cortex-A7)    │
└───────────────────────────────────────────────────────────────┘
```

**Key submodules** (in `lib/`):
- `lvgl` - UI framework
- `libhv` - Async networking (WebSocket client for Moonraker)
- `spdlog` - Logging
- `wpa_supplicant` - WiFi control (client library only)
- `tinygl` - Software OpenGL for 3D G-code preview

### Build Targets

| Target | Command | Output | Description |
|--------|---------|--------|-------------|
| Native (macOS/Linux) | `make -j` | `build/bin/helix-screen` | SDL2 simulator |
| Raspberry Pi | `make pi-docker` | `build/pi/bin/helix-screen` | aarch64, framebuffer |
| Adventurer 5M | `make ad5m-docker` | `build/ad5m/bin/helix-screen` | armv7-a, framebuffer |

### Build System Improvements Needed

1. **Thelio sync problem** (URGENT): The worktree on thelio is manually rsynced,
   causing constant out-of-sync issues. Should be a proper git checkout with
   submodules, or use a sync tool like Unison/Syncthing.

2. **Docker image caching**: The toolchain Docker images should be pushed to a
   registry so they don't need rebuilding. Currently rebuilding from scratch
   takes 15-30 minutes.

3. **Unified cross-compile interface**: Currently `make pi-docker` and
   `make ad5m-docker` use different code paths. Could be unified with
   `make PLATFORM=ad5m` that auto-detects if Docker is needed.

4. **Dependency version pinning**: Submodules are pinned but system deps
   (SDL2, fmt) are not. Consider vcpkg or Conan for reproducible builds.

5. **Incremental Docker builds**: Currently Docker mounts the source and builds
   fresh. Could use BuildKit cache mounts for faster incremental builds.

---

## Target Hardware Specs
- **CPU**: Cortex-A7 (armv7-a hard-float)
- **Display**: 800x480 framebuffer
- **RAM**: 110MB total
- **C Library**: glibc 2.25
- **Key Constraint**: Cannot use symbols newer than GLIBC_2.25

## Problem Summary

Modern ARM toolchains ship with sysroots containing glibc 2.28+ symbols. When we compile
with these toolchains, the resulting binary requires newer glibc symbols than AD5M provides.

**Symbols causing issues** (from `objdump -T` / `readelf --dyn-syms`):
- `stat@GLIBC_2.33` / `fstat@GLIBC_2.33` / `lstat@GLIBC_2.33` - stat family
- `pow@GLIBC_2.29` - math functions
- `fcntl@GLIBC_2.28` - file control

---

## ⚠️ CRITICAL: Thelio Sync Issues

**THIS IS THE #1 TIME WASTER. READ THIS BEFORE DOING ANYTHING.**

The `helixscreen-memory-opt` worktree on thelio is NOT a proper git checkout - it was
manually rsynced. Files get out of sync constantly.

### The Problem
- Local changes on macOS don't automatically appear on thelio
- rsync is unreliable (partial transfers, permission errors from Docker-owned files)
- Submodules are often empty or stale on thelio
- Build errors are frequently caused by outdated files, NOT actual code issues

### Before Building on Thelio, ALWAYS:

```bash
# 1. Kill any stuck Docker containers
ssh thelio.local "docker ps -q | xargs -r docker kill"

# 2. Clean up Docker-owned build artifacts (they block rsync)
ssh thelio.local "cd ~/Code/Printing/helixscreen-memory-opt && sudo rm -rf build/"

# 3. Sync ALL essential directories (run from local macOS):
rsync -avz --exclude='.git' --exclude='build' --exclude='node_modules' --exclude='.venv' \
  /Users/pbrown/Code/Printing/helixscreen-memory-opt/Makefile \
  /Users/pbrown/Code/Printing/helixscreen-memory-opt/lv_conf.h \
  /Users/pbrown/Code/Printing/helixscreen-memory-opt/package.json \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/mk/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/mk/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/src/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/src/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/include/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/include/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/scripts/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/scripts/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/docker/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/docker/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/ui_xml/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/ui_xml/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/tools/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/tools/

rsync -avz /Users/pbrown/Code/Printing/helixscreen-memory-opt/config/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/config/

# 4. Sync submodules (these are often empty!)
rsync -avz --exclude='*.o' --exclude='*.a' --exclude='*.d' \
  /Users/pbrown/Code/Printing/helixscreen-memory-opt/lib/tinygl/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/lib/tinygl/

# lib/lvgl, lib/spdlog, lib/libhv are usually OK but verify:
ssh thelio.local "ls ~/Code/Printing/helixscreen-memory-opt/lib/lvgl/src/*.c | head -3"
ssh thelio.local "ls ~/Code/Printing/helixscreen-memory-opt/lib/spdlog/include/*.h | head -3"
```

### Common rsync Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `Permission denied` on build/ | Docker created files as root | `sudo rm -rf build/` |
| `rsync: connection unexpectedly closed` | Large transfer, SSH timeout | Sync directories individually |
| `--delete` removes mk/ | Wrong rsync syntax | Never use `--delete` with multiple sources |
| `No such file or directory: mk/rules.mk` | Previous rsync deleted it | Re-sync mk/ directory |

### Better Long-Term Solution (TODO)
Set up proper git worktree on thelio instead of rsync:
```bash
# On thelio:
cd ~/Code/Printing
git clone --branch memory-opt git@github.com:yourrepo/helixscreen.git helixscreen-memory-opt
git submodule update --init --recursive
```

---

## Approaches Researched

### 1. ARM GCC 10.3-2021.07 (Current Docker Toolchain)
- **URL**: https://developer.arm.com/downloads/-/gnu-a
- **Sysroot glibc**: 2.33
- **Status**: ❌ NOT COMPATIBLE with dynamic linking
- **Problem**: Produces binaries requiring GLIBC_2.33 symbols
- **Workaround**: Use `-static` flag for fully static build

### 2. Bootlin Toolchains
- **URL**: https://toolchains.bootlin.com/
- **Status**: ❌ NOT SUITABLE
- **Problem**: All current builds use glibc 2.39+ (way too new)
- **Note**: Bootlin rebuilds toolchains, doesn't keep old glibc versions

### 3. Linaro GCC 7.5-2019.12
- **URL**: https://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/arm-linux-gnueabihf/
- **Sysroot glibc**: 2.25 (exact match for AD5M!)
- **Status**: ⚠️ PARTIAL - glibc matches but C++17 support limited
- **Problem**: GCC 7.5 has `std::experimental::filesystem`, not `std::filesystem`
- **Code using std::filesystem**:
  - `src/gcode_file_modifier.cpp`
  - `src/gcode_ops_detector.cpp`
- **Workaround needed**: Either refactor to use experimental, or wrap with `#if __GNUC__ >= 8`

### 4. Fully Static Build (`-static` flag)
- **Status**: 🔄 IN PROGRESS (blocked by sync issues)
- **Approach**: Link everything statically, no runtime glibc dependency
- **Trade-offs**:
  - ✅ Guaranteed compatibility - no glibc version issues
  - ✅ Can use modern toolchain (GCC 10+)
  - ❌ Larger binary (~5-8MB vs ~2MB dynamic)
  - ❌ No glibc NSS support (but we don't need it)
  - ⚠️ Warning about getaddrinfo() (see Issue 4 below)

### 5. Musl-based Toolchain (Not Yet Tried)
- **Potential approach**: Use musl libc instead of glibc
- **Pros**: Static linking is natural, smaller binaries
- **Cons**: May have compatibility issues with some C++ features

---

## Build Issues Encountered & Fixes

### Issue 1: wpa_supplicant LTO Conflict
**Symptom**: Undefined references to `wpa_ctrl_*` functions despite libwpa_client.a being linked
```
arm-none-linux-gnueabihf-ar: plugin needed to handle lto object
undefined reference to `wpa_ctrl_request'
undefined reference to `wpa_ctrl_open'
```

**Root Cause**: wpa_supplicant was compiled with LTO flags inherited from our build.
LTO-compiled .a files contain GIMPLE IR (intermediate representation), not machine code.
When linking with our LTO-compiled objects, the linker couldn't process the mixed formats.

**Fix** (in `mk/deps.mk`):
```makefile
# Strip LTO flags for wpa_supplicant build
WPA_CFLAGS := $(filter-out -flto -ffunction-sections -fdata-sections,$(TARGET_CFLAGS))
# Use env -u to unset inherited CFLAGS
$(Q)env -u CFLAGS CC="$(CC)" EXTRA_CFLAGS="$(WPA_CFLAGS)" $(MAKE) -C ...
```
**Status**: ✅ FIXED

### Issue 2: helix-splash DisplayBackend::create_auto() Undefined
**Symptom**:
```
undefined reference to `DisplayBackend::create_auto()'
```

**Root Cause**: With LTO enabled, the linker aggressively strips symbols from static
libraries that it doesn't see as directly referenced. `create_auto()` is called
internally through `create()`, but the linker only sees the entry point in `main()`.

**Fix** (in `mk/splash.mk`):
```makefile
# Use --whole-archive to force all symbols from display library
$(Q)$(CXX) $(SPLASH_OBJ) -Wl,--whole-archive $(DISPLAY_LIB) -Wl,--no-whole-archive ...
```
**Status**: ✅ FIXED

### Issue 3: glibc Symbol Version Mismatch
**Symptom** (on AD5M device):
```
/lib/libc.so.6: version `GLIBC_2.33' not found (required by ./helix-screen)
/lib/libc.so.6: version `GLIBC_2.29' not found (required by ./helix-screen)
```

**Root Cause**: Toolchain sysroot has glibc 2.33, AD5M system has glibc 2.25

**Fix Options**:
1. Use older toolchain with matching sysroot (Linaro 7.5) - requires code changes
2. Use fully static build - current approach
3. Use symbol versioning tricks (complex, fragile)

**Status**: 🔄 Using static linking approach

### Issue 4: SSL Libraries Not Found with ENABLE_SSL=no
**Symptom**:
```
cannot find -lssl
cannot find -lcrypto
```

**Root Cause**: The Makefile had unconditional `-lssl -lcrypto` on line 349 even when
`ENABLE_SSL=no` was set. The conditional was only around `LDFLAGS +=`, not the base definition.

**Fix** (in `Makefile`, cross-compile section ~line 350):
```makefile
# BEFORE (broken):
LDFLAGS := ... -lssl -lcrypto -ldl -lm -lpthread

# AFTER (fixed):
LDFLAGS := ... -ldl -lm -lpthread
ifeq ($(ENABLE_SSL),yes)
    LDFLAGS += -lssl -lcrypto
endif
```
**Status**: ✅ FIXED locally, but thelio had old Makefile (sync issue!)

### Issue 5: getaddrinfo Static Linking Warning
**Symptom** (during link):
```
warning: Using 'getaddrinfo' in statically linked applications requires at runtime
the shared libraries from the glibc version used for linking
```

**Root Cause**: libhv's `hsocket.c` uses `getaddrinfo()` for DNS resolution.
With static linking, glibc's NSS (Name Service Switch) still needs dynamic loading
for DNS resolution to work properly.

**Impact**: May affect Moonraker hostname resolution if not using IP address directly.

**Workarounds**:
1. Always use IP addresses instead of hostnames (e.g., `192.168.1.x` not `printer.local`)
2. Or: Configure `/etc/hosts` on AD5M with static mappings
3. Or: Use musl-based toolchain which handles this better

**Status**: ⚠️ WARNING only - may work fine if using IP addresses

### Issue 6: TinyGL Submodule Empty on Thelio
**Symptom**:
```
make[2]: *** [Makefile:38: clean] Error 2
make[1]: *** [mk/rules.mk:100: build/ad5m/lib/libTinyGL.a] Error 2
```

**Root Cause**: The lib/tinygl/src/ directory was empty on thelio because submodule
wasn't properly synced.

**Fix**: Sync TinyGL explicitly:
```bash
rsync -avz --exclude='*.o' --exclude='*.a' --exclude='*.d' \
  /Users/pbrown/Code/Printing/helixscreen-memory-opt/lib/tinygl/ \
  thelio.local:~/Code/Printing/helixscreen-memory-opt/lib/tinygl/
```
**Status**: ✅ FIXED by syncing

---

## Current Strategy

**Approach**: Fully static build with ARM GCC 10.3

**Config** (in `mk/cross.mk`):
```makefile
TARGET_LDFLAGS := -Wl,--gc-sections -flto -static
ENABLE_SSL := no
```

**Build Flags**:
- `PLATFORM_TARGET=ad5m` - Select AD5M cross-compile target
- `ENABLE_SSL=no` - Disable SSL (not needed for local Moonraker)
- `SKIP_FONTS=1` - Skip font generation (npm not in Docker)
- `SKIP_OPTIONAL_DEPS=1` - Skip optional dependency check

**Expected outcome**:
- Binary size: ~5-8MB (larger than dynamic)
- glibc dependency: NONE (fully self-contained)
- Compatibility: Should work on any Linux armv7-a system

---

## Files Modified for AD5M Static Build

| File | Change | Status |
|------|--------|--------|
| `mk/deps.mk` | wpa_supplicant LTO fix | ✅ Done |
| `mk/cross.mk` | `-static` flag, `ENABLE_SSL=no` | ✅ Done |
| `mk/splash.mk` | `--whole-archive` for display lib | ✅ Done |
| `Makefile` | Conditional SSL libs | ✅ Done |
| `docker/Dockerfile.ad5m` | ARM GCC 10.3 toolchain | ✅ Done |
| `scripts/check-deps.sh` | Respect `ENABLE_SSL=no` | ✅ Done |

---

## Build Command (After Syncing to Thelio!)

```bash
# On thelio:
cd ~/Code/Printing/helixscreen-memory-opt

# Clean previous build artifacts (important!)
sudo rm -rf build/ad5m

# Build
docker run --rm -v $(pwd):/src -w /src helixscreen/toolchain-ad5m \
  make PLATFORM_TARGET=ad5m SKIP_OPTIONAL_DEPS=1 ENABLE_SSL=no SKIP_FONTS=1 -j$(nproc)

# Verify results
file build/ad5m/bin/helix-screen       # Should show "statically linked"
ldd build/ad5m/bin/helix-screen        # Should show "not a dynamic executable"
ls -lh build/ad5m/bin/                 # Check file sizes
```

---

## Next Steps for Fresh Session

1. [ ] **FIRST**: Follow the "Before Building on Thelio" sync checklist above
2. [ ] Run clean build on thelio
3. [ ] Verify binary is statically linked
4. [ ] Check binary size
5. [ ] Copy to AD5M and test: `scp build/ad5m/bin/helix-screen root@192.168.1.x:/tmp/`
6. [ ] Test on AD5M: `ssh root@192.168.1.x "/tmp/helix-screen --test"`

---

## Commands Reference

```bash
# Check glibc symbol requirements
objdump -T binary | grep GLIBC_
readelf --dyn-syms binary | grep GLIBC

# Check if binary is static
file binary                    # Should show "statically linked"
ldd binary                     # Should show "not a dynamic executable"

# Build AD5M in Docker
docker build -t helixscreen/toolchain-ad5m -f docker/Dockerfile.ad5m docker/
docker run --rm -v $(pwd):/src helixscreen/toolchain-ad5m make PLATFORM_TARGET=ad5m clean
docker run --rm -v $(pwd):/src helixscreen/toolchain-ad5m make PLATFORM_TARGET=ad5m -j

# Check toolchain sysroot glibc version
ls /opt/arm-toolchain/*/libc/lib/ | grep libc-

# Verify files on thelio match local
ssh thelio.local "md5sum ~/Code/Printing/helixscreen-memory-opt/Makefile"
md5sum /Users/pbrown/Code/Printing/helixscreen-memory-opt/Makefile
```

---

## Session History / Time Spent

| Date | Hours | What Happened |
|------|-------|---------------|
| Dec 2-3 | ~4h | Initial glibc investigation, tried Linaro toolchain |
| Dec 4 | ~3h | Dockerfile rewrite for static linking, wpa_supplicant LTO fix |
| Dec 5 | ~4h | **Mostly sync issues**: Makefile not synced, scripts not synced, TinyGL empty, mk/ deleted by bad rsync |

**Total time lost to sync issues: ~3-4 hours**

The actual code fixes (LTO, static linking, SSL conditional) took maybe 1 hour total.
The rest was fighting file synchronization between macOS and thelio.
