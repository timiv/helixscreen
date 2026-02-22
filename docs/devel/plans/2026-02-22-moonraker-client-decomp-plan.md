# MoonrakerClient Decomposition Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Split the 2,553-line MoonrakerClient monolith into three focused classes with one clear responsibility each.

**Architecture:** Composition-based extraction. `MoonrakerRequestTracker` owns pending request lifecycle (registration, timeout, response routing, cleanup). `MoonrakerDiscoverySequence` owns the multi-step async printer discovery flow. `MoonrakerClient` retains WebSocket lifecycle, subscriptions, and events — delegating to the two extracted members.

**Tech Stack:** C++17, libhv WebSocketClient, nlohmann JSON (`"hv/json.hpp"`), spdlog

---

## Task 1: Extract MoonrakerRequestTracker — Header

**Files:**
- Create: `include/moonraker_request_tracker.h`

**Step 1: Write the header**

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "json_fwd.h"
#include "moonraker_error.h"
#include "moonraker_events.h"
#include "moonraker_request.h"

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace hv {
class WebSocketClient;
}

namespace helix {
using RequestId = uint64_t;
constexpr RequestId INVALID_REQUEST_ID = 0;
}

namespace helix {
using ::json;

/**
 * @brief Owns pending JSON-RPC request lifecycle
 *
 * Handles request ID generation, registration, timeout checking,
 * response routing, and disconnect cleanup. Uses two-phase lock
 * pattern: copy callbacks under lock, invoke outside lock.
 */
class MoonrakerRequestTracker {
  public:
    MoonrakerRequestTracker();

    /**
     * @brief Send JSON-RPC request and register for response tracking
     *
     * Builds JSON-RPC envelope, sends via WebSocket, registers pending request.
     *
     * @param ws WebSocket client to send through
     * @param method RPC method name
     * @param params JSON parameters (can be null)
     * @param success_cb Success callback
     * @param error_cb Error callback (optional)
     * @param timeout_ms Timeout override (0 = use default)
     * @param silent Suppress RPC_ERROR events
     * @return Request ID, or INVALID_REQUEST_ID on error
     */
    RequestId send(hv::WebSocketClient& ws, const std::string& method, const json& params,
                   std::function<void(json)> success_cb,
                   std::function<void(const MoonrakerError&)> error_cb,
                   uint32_t timeout_ms = 0, bool silent = false);

    /**
     * @brief Send fire-and-forget JSON-RPC (no callbacks, no tracking)
     */
    int send_fire_and_forget(hv::WebSocketClient& ws, const std::string& method,
                             const json& params);

    /**
     * @brief Route an incoming JSON-RPC response to its pending request
     *
     * @param msg Parsed JSON message containing "id" field
     * @param emit_event Function to emit transport events
     * @return true if message was a tracked response, false if not a response or unknown ID
     */
    bool route_response(const json& msg,
                        std::function<void(MoonrakerEventType, const std::string&, bool,
                                           const std::string&)> emit_event);

    /**
     * @brief Cancel a pending request (no callbacks invoked)
     */
    bool cancel(RequestId id);

    /**
     * @brief Check for timed-out requests and invoke error callbacks
     */
    void check_timeouts(std::function<void(MoonrakerEventType, const std::string&, bool,
                                           const std::string&)> emit_event);

    /**
     * @brief Cancel all pending requests, invoking error callbacks
     *
     * Called on disconnect.
     */
    void cleanup_all();

    /** @brief Set default request timeout */
    void set_default_timeout(uint32_t timeout_ms) { default_request_timeout_ms_ = timeout_ms; }

    /** @brief Get default request timeout */
    uint32_t get_default_timeout() const { return default_request_timeout_ms_; }

  private:
    std::map<uint64_t, PendingRequest> pending_requests_;
    std::mutex requests_mutex_;
    std::atomic_uint64_t request_id_{0};
    uint32_t default_request_timeout_ms_{30000};
};

} // namespace helix
```

**Step 2: Verify it compiles**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | head -30`
Expected: Clean compile (header only, nothing includes it yet)

**Step 3: Commit**

```bash
git add include/moonraker_request_tracker.h
git commit -m "refactor(moonraker): add MoonrakerRequestTracker header"
```

---

## Task 2: Extract MoonrakerRequestTracker — Implementation

**Files:**
- Create: `src/printer/moonraker_request_tracker.cpp`
- Reference: `src/api/moonraker_client.cpp` lines 871-998 (send_jsonrpc overloads, cancel_request), 1752-1854 (check_request_timeouts, cleanup_pending_requests)

**Step 1: Write the implementation**

Move the following logic from `moonraker_client.cpp` into the new file:

- `send()` — combines the JSON-RPC envelope building from `send_jsonrpc(method, params, success_cb, error_cb, timeout_ms, silent)` (lines 903-980) with the request registration. Uses `ws.send()` instead of `this->send()`.
- `send_fire_and_forget()` — combines the two simple `send_jsonrpc` overloads (lines 871-895). Generates an ID, builds JSON-RPC, sends, no tracking.
- `route_response()` — extracts the `j.contains("id")` block from the `onmessage` handler (lines 387-469). Returns `true` if message was handled.
- `cancel()` — `cancel_request()` (lines 982-998)
- `check_timeouts()` — `check_request_timeouts()` (lines 1752-1809)
- `cleanup_all()` — `cleanup_pending_requests()` (lines 1811-1854)

Key details to preserve:
- Two-phase lock pattern: copy callbacks under lock, invoke outside
- `AbortManager::instance().is_handling_shutdown()` check in route_response for toast suppression
- `LOG_ERROR_INTERNAL` macro usage (from `ui_error_reporting.h`)
- `request_id_.fetch_add(1) + 1` pattern (IDs start at 1)
- Error callback invocation on send failure

**Step 2: Verify it compiles**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | tail -20`
Expected: Clean compile (Makefile auto-discovers new .cpp)

**Step 3: Commit**

```bash
git add src/printer/moonraker_request_tracker.cpp
git commit -m "refactor(moonraker): implement MoonrakerRequestTracker"
```

---

## Task 3: Wire MoonrakerRequestTracker into MoonrakerClient

**Files:**
- Modify: `include/moonraker_client.h`
- Modify: `src/api/moonraker_client.cpp`

**Step 1: Add tracker member to header**

In `moonraker_client.h`:
- Add `#include "moonraker_request_tracker.h"`
- Remove `RequestId` and `INVALID_REQUEST_ID` definitions (now in tracker header) — but keep them if other files include moonraker_client.h for those types. Actually: the types are defined in tracker header within `namespace helix`, and moonraker_client.h already has `namespace helix`. Keep a `using` or just include the tracker header which brings them in.
- Add `MoonrakerRequestTracker tracker_;` as private member
- Remove from private section: `pending_requests_`, `requests_mutex_`, `request_id_`, `default_request_timeout_ms_`
- Remove private method declarations: `check_request_timeouts()`, `cleanup_pending_requests()`
- Update `process_timeouts()` inline body to call `tracker_.check_timeouts(...)`
- Update `set_default_request_timeout()` to call `tracker_.set_default_timeout()`

**Step 2: Update implementation to delegate**

In `moonraker_client.cpp`:
- `send_jsonrpc(method)` → `tracker_.send_fire_and_forget(*this, method, json{})`
- `send_jsonrpc(method, params)` → `tracker_.send_fire_and_forget(*this, method, params)`
- `send_jsonrpc(method, params, cb)` → `tracker_.send(*this, method, params, cb, nullptr)`
- `send_jsonrpc(method, params, success_cb, error_cb, timeout_ms, silent)` → `tracker_.send(*this, method, params, success_cb, error_cb, timeout_ms, silent)`
- `cancel_request(id)` → `tracker_.cancel(id)`
- `disconnect()`: replace `cleanup_pending_requests()` call with `tracker_.cleanup_all()`
- `onmessage` lambda: replace the `j.contains("id")` block with:
  ```cpp
  if (tracker_.route_response(j, [this](auto type, auto& msg, auto err, auto& details) {
      emit_event(type, msg, err, details);
  })) {
      // Response handled, but still check for notifications below
      // (a message can't be both, so this is just for clarity)
  }
  ```
- Remove: `check_request_timeouts()` and `cleanup_pending_requests()` method implementations
- `configure_timeouts()`: delegate `default_request_timeout_ms_` to `tracker_.set_default_timeout()`
- Constructor: remove `request_id_(0)` from init list

**Step 3: Handle the `RequestId`/`INVALID_REQUEST_ID` type forwarding**

Since many files include `moonraker_client.h` expecting `helix::RequestId` and `helix::INVALID_REQUEST_ID`, ensure the tracker header is transitively included. The `#include "moonraker_request_tracker.h"` in moonraker_client.h handles this.

**Step 4: Build and verify**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | tail -30`
Expected: Clean compile

**Step 5: Run tests**

Run: `make test -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp && /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp/build/bin/helix-tests "[moonraker]" 2>&1 | tail -30`
Expected: All moonraker-tagged tests pass

**Step 6: Commit**

```bash
git add include/moonraker_client.h src/api/moonraker_client.cpp include/moonraker_request_tracker.h
git commit -m "refactor(moonraker): wire MoonrakerRequestTracker into MoonrakerClient"
```

---

## Task 4: Extract MoonrakerDiscoverySequence — Header

**Files:**
- Create: `include/moonraker_discovery_sequence.h`

**Step 1: Write the header**

```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "json_fwd.h"
#include "printer_discovery.h"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

namespace helix {

class MoonrakerClient; // Forward declaration

/**
 * @brief Owns the multi-step async printer discovery flow
 *
 * Discovery timeline:
 * 1. server.connection.identify → identified
 * 2. printer.objects.list → parse_objects() → on_hardware_discovered
 * 3. server.info → Moonraker version, klippy_state
 * 4. printer.info → hostname, software_version
 * 5. MCU queries → firmware versions
 * 6. printer.objects.subscribe → initial state dispatched
 * 7. on_discovery_complete
 */
class MoonrakerDiscoverySequence {
  public:
    explicit MoonrakerDiscoverySequence(MoonrakerClient& client);

    /**
     * @brief Start the discovery sequence
     *
     * @param on_complete Called when discovery finishes successfully
     * @param on_error Called if discovery fails (e.g., Klippy not connected)
     */
    void start(std::function<void()> on_complete,
               std::function<void(const std::string& reason)> on_error = nullptr);

    /**
     * @brief Parse Klipper object list into typed hardware vectors
     * @param objects JSON array of object name strings
     */
    void parse_objects(const json& objects);

    /**
     * @brief Parse bed mesh callback dispatch
     * @param bed_mesh JSON from bed_mesh subscription
     */
    void parse_bed_mesh(const json& bed_mesh);

    /** @brief Reset identification state (e.g., on disconnect) */
    void reset_identified() { identified_.store(false); }

    /** @brief Check if identified to Moonraker */
    bool is_identified() const { return identified_.load(); }

    /** @brief Clear all cached discovery data */
    void clear_cache();

    /** @brief Get discovered hardware data */
    [[nodiscard]] const PrinterDiscovery& hardware() const { return hardware_; }

    /** @brief Mutable hardware access (for dispatch_status_update kinematics) */
    PrinterDiscovery& hardware() { return hardware_; }

    /** @brief Set callback for early hardware discovery phase */
    void set_on_hardware_discovered(std::function<void(const PrinterDiscovery&)> cb) {
        on_hardware_discovered_ = std::move(cb);
    }

    /** @brief Set callback for discovery completion */
    void set_on_discovery_complete(std::function<void(const PrinterDiscovery&)> cb) {
        on_discovery_complete_ = std::move(cb);
    }

    /** @brief Set callback for bed mesh updates */
    void set_bed_mesh_callback(std::function<void(const json&)> cb) {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        bed_mesh_callback_ = std::move(cb);
    }

    // ======== Mock access (protected members exposed for MoonrakerClientMock) ========

    /** @brief Get hardware vectors (for mock to populate directly) */
    std::vector<std::string>& heaters() { return heaters_; }
    std::vector<std::string>& sensors() { return sensors_; }
    std::vector<std::string>& fans() { return fans_; }
    std::vector<std::string>& leds() { return leds_; }
    std::vector<std::string>& steppers() { return steppers_; }
    std::vector<std::string>& afc_objects() { return afc_objects_; }
    std::vector<std::string>& filament_sensors() { return filament_sensors_; }

  private:
    void continue_discovery(std::function<void()> on_complete,
                            std::function<void(const std::string& reason)> on_error);
    void complete_discovery_subscription(std::function<void()> on_complete);

    MoonrakerClient& client_;

    // Hardware vectors
    std::vector<std::string> heaters_;
    std::vector<std::string> sensors_;
    std::vector<std::string> fans_;
    std::vector<std::string> leds_;
    std::vector<std::string> steppers_;
    std::vector<std::string> afc_objects_;
    std::vector<std::string> filament_sensors_;

    PrinterDiscovery hardware_;
    std::atomic<bool> identified_{false};

    // Callbacks
    std::function<void(const PrinterDiscovery&)> on_hardware_discovered_;
    std::function<void(const PrinterDiscovery&)> on_discovery_complete_;
    std::function<void(const json&)> bed_mesh_callback_;
    std::mutex callbacks_mutex_;
};

} // namespace helix
```

**Step 2: Verify it compiles**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | head -30`
Expected: Clean compile

**Step 3: Commit**

```bash
git add include/moonraker_discovery_sequence.h
git commit -m "refactor(moonraker): add MoonrakerDiscoverySequence header"
```

---

## Task 5: Extract MoonrakerDiscoverySequence — Implementation

**Files:**
- Create: `src/printer/moonraker_discovery_sequence.cpp`
- Reference: `src/api/moonraker_client.cpp` lines 1040-1749 (discover_printer, continue_discovery, complete_discovery_subscription, parse_objects, parse_bed_mesh, clear_discovery_cache)

**Step 1: Write the implementation**

Move the following from `moonraker_client.cpp`:

- `start()` — from `discover_printer()` (lines 1040-1085). Calls `client_.send_jsonrpc()` for identify.
- `continue_discovery()` — (lines 1087-1469). Calls `client_.send_jsonrpc()` for objects.list, server.info, printer.info, MCU queries.
- `complete_discovery_subscription()` — (lines 1471-1603). Builds subscription JSON, calls `client_.send_jsonrpc()`, dispatches initial state via `client_.dispatch_status_update()`.
- `parse_objects()` — (lines 1605-1733). Categorizes Klipper objects.
- `parse_bed_mesh()` — (lines 1735-1749). Forwards to bed_mesh_callback_.
- `clear_cache()` — from `clear_discovery_cache()` (lines around 133-135). Resets all vectors and hardware_.

Key dependencies the implementation needs from MoonrakerClient:
- `client_.send_jsonrpc()` — for sending requests
- `client_.dispatch_status_update()` — for initial state in complete_discovery_subscription
- `client_.get_connection_generation()` — will need a new accessor or pass generation as parameter
- `HELIX_VERSION` from `helix_version.h`
- `PrinterState::instance()` — for setting hostname, software_version, etc.
- Various spdlog calls

Note: `connection_generation_` is used in `continue_discovery()` to detect stale callbacks. We'll need to either:
(a) Add `uint64_t connection_generation() const` accessor to MoonrakerClient, or
(b) Pass the generation into `start()`.

Option (a) is simpler. Add `uint64_t connection_generation() const { return connection_generation_.load(); }` to MoonrakerClient header.

**Step 2: Build**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | tail -30`
Expected: Clean compile

**Step 3: Commit**

```bash
git add src/printer/moonraker_discovery_sequence.cpp
git commit -m "refactor(moonraker): implement MoonrakerDiscoverySequence"
```

---

## Task 6: Wire MoonrakerDiscoverySequence into MoonrakerClient

**Files:**
- Modify: `include/moonraker_client.h`
- Modify: `src/api/moonraker_client.cpp`

**Step 1: Add discovery member to header**

In `moonraker_client.h`:
- Add `#include "moonraker_discovery_sequence.h"`
- Add `MoonrakerDiscoverySequence discovery_;` as private member
- Add `uint64_t connection_generation() const { return connection_generation_.load(); }` public accessor
- Remove from protected: `heaters_`, `sensors_`, `fans_`, `leds_`, `steppers_`, `afc_objects_`, `filament_sensors_`, `hardware_`, `on_hardware_discovered_`, `on_discovery_complete_`, `bed_mesh_callback_`
- Remove from private: `continue_discovery()`, `complete_discovery_subscription()` declarations
- Update public methods to delegate:
  - `discover_printer()` → `discovery_.start()`
  - `parse_objects()` → `discovery_.parse_objects()`
  - `parse_bed_mesh()` → `discovery_.parse_bed_mesh()`
  - `hardware()` → `discovery_.hardware()`
  - `is_identified()` → `discovery_.is_identified()`
  - `reset_identified()` → `discovery_.reset_identified()`
  - `set_on_hardware_discovered()` → `discovery_.set_on_hardware_discovered()`
  - `set_on_discovery_complete()` → `discovery_.set_on_discovery_complete()`
  - `set_bed_mesh_callback()` → `discovery_.set_bed_mesh_callback()`
  - `clear_discovery_cache()` → `discovery_.clear_cache()`

**Step 2: Update implementation**

In `moonraker_client.cpp`:
- Remove implementations: `discover_printer()`, `continue_discovery()`, `complete_discovery_subscription()`, `parse_objects()`, `parse_bed_mesh()`, `clear_discovery_cache()`
- These are now one-line delegations defined inline in the header, or the bodies are in the discovery sequence impl.
- Constructor: initialize `discovery_(*this)` in init list
- `disconnect()`: add `discovery_.reset_identified()` and `discovery_.clear_cache()`
- `dispatch_status_update()`: access `discovery_.hardware()` instead of `hardware_` for kinematics
- `onopen` callback: call `discovery_.start()` where `discover_printer()` was called

**Step 3: Build**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | tail -30`
Expected: Clean compile

**Step 4: Run tests**

Run: `make test -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp && /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp/build/bin/helix-tests "[moonraker]" 2>&1 | tail -30`
Expected: All tests pass

**Step 5: Commit**

```bash
git add include/moonraker_client.h src/api/moonraker_client.cpp include/moonraker_discovery_sequence.h src/printer/moonraker_discovery_sequence.cpp
git commit -m "refactor(moonraker): wire MoonrakerDiscoverySequence into MoonrakerClient"
```

---

## Task 7: Adapt MoonrakerClientMock

**Files:**
- Modify: `include/moonraker_client_mock.h`
- Modify: `src/api/moonraker_client_mock.cpp`
- Possibly modify: `src/api/moonraker_client_mock_server.cpp` (discover_printer override)

**Step 1: Update mock to use discovery accessors**

The mock directly accesses protected members like `heaters_`, `sensors_`, etc. These now live on `discovery_`. Update the mock to use the public accessors on the discovery sequence:

- `heaters_` → `discovery_.heaters()` — but `discovery_` is private on MoonrakerClient. Options:
  (a) Make `discovery_` protected (simplest, follows existing pattern where mock accesses protected members)
  (b) Add forwarding accessors to MoonrakerClient

Option (a) is best since the mock is the only subclass and this is an established pattern.

Change `discovery_` from private to protected in `moonraker_client.h`.

In mock:
- `heaters_` → `discovery_.heaters()`
- `sensors_` → `discovery_.sensors()`
- `fans_` → `discovery_.fans()`
- `leds_` → `discovery_.leds()`
- `steppers_` → `discovery_.steppers()`
- `afc_objects_` → `discovery_.afc_objects()`
- `filament_sensors_` → `discovery_.filament_sensors()`
- `hardware_` → `discovery_.hardware()`
- `on_hardware_discovered_` → `discovery_.set_on_hardware_discovered()` / call through discovery
- `on_discovery_complete_` → `discovery_.set_on_discovery_complete()` / call through discovery
- `notify_callbacks_` → stays on MoonrakerClient (not moved), so mock still accesses it
- `dispatch_status_update()` → stays on MoonrakerClient, no change

Test helpers like `set_heaters()` need to update: `discovery_.heaters() = std::move(heaters);`

For `rebuild_hardware()` — this calls `parse_objects()` which builds a JSON array and calls the parent's `parse_objects()`. Since `parse_objects()` now delegates to `discovery_.parse_objects()`, this should work transparently.

**Step 2: Build**

Run: `make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | tail -30`
Expected: Clean compile

**Step 3: Run full test suite**

Run: `make test -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp && /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp/build/bin/helix-tests 2>&1 | tail -40`
Expected: All tests pass (full suite to catch any mock-dependent test failures)

**Step 4: Commit**

```bash
git add include/moonraker_client.h include/moonraker_client_mock.h src/api/moonraker_client_mock.cpp src/api/moonraker_client_mock_server.cpp
git commit -m "refactor(moonraker): adapt MoonrakerClientMock for decomposed discovery"
```

---

## Task 8: Adapt Test Mock (tests/mocks/)

**Files:**
- Modify: `tests/mocks/moonraker_client_mock.h`
- Modify: `tests/mocks/moonraker_client_mock.cpp`

**Step 1: Check what the test mock accesses**

The test-only mock in `tests/mocks/` may also reference protected members that moved. Read the files and update any direct member access to use the discovery sequence accessors.

**Step 2: Build tests**

Run: `make test -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp 2>&1 | tail -30`
Expected: Clean compile

**Step 3: Run full test suite**

Run: `/Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp/build/bin/helix-tests 2>&1 | tail -40`
Expected: All tests pass

**Step 4: Commit**

```bash
git add tests/mocks/moonraker_client_mock.h tests/mocks/moonraker_client_mock.cpp
git commit -m "refactor(moonraker): adapt test mock for decomposed client"
```

---

## Task 9: Clean Up and Verify

**Files:**
- Verify: `include/moonraker_client.h` (target ~400 lines)
- Verify: `src/api/moonraker_client.cpp` (target ~1,000 lines)

**Step 1: Line count verification**

```bash
wc -l include/moonraker_client.h src/api/moonraker_client.cpp include/moonraker_request_tracker.h src/printer/moonraker_request_tracker.cpp include/moonraker_discovery_sequence.h src/printer/moonraker_discovery_sequence.cpp
```

Expected: moonraker_client.h ~400, moonraker_client.cpp ~1000 (vs. 699/1854 before)

**Step 2: Full build (clean)**

```bash
make clean -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp && make -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp
```
Expected: Clean compile

**Step 3: Full test suite**

```bash
make test -j16 -C /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp && /Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp/build/bin/helix-tests 2>&1 | tail -40
```
Expected: All tests pass

**Step 4: Smoke test with mock UI**

```bash
/Users/pbrown/Code/Printing/helixscreen/.worktrees/moonraker-client-decomp/build/bin/helix-screen --test -vv 2>&1 | head -50
```
Expected: Starts, discovers mock printer, shows home panel

**Step 5: Update REFACTOR_PLAN.md**

Mark section 2.4 (MoonrakerClient decomposition) as complete in the refactor plan.

**Step 6: Final commit**

```bash
git add docs/devel/plans/REFACTOR_PLAN.md
git commit -m "docs: mark MoonrakerClient decomposition complete in refactor plan"
```

---

## Dependency Order

```
Task 1 (tracker header) → Task 2 (tracker impl) → Task 3 (wire tracker)
Task 4 (discovery header) → Task 5 (discovery impl) → Task 6 (wire discovery)
Task 3 + Task 6 → Task 7 (adapt main mock)
Task 7 → Task 8 (adapt test mock)
Task 8 → Task 9 (verify + clean up)
```

Tasks 1-3 and Tasks 4-5 are independent chains that could run in parallel if desired, but Task 6 depends on Task 3 (tracker must be wired first so `send_jsonrpc` delegation works when discovery calls it).

## Key Risk Areas

1. **`RequestId`/`INVALID_REQUEST_ID` type visibility** — Many files expect these from `moonraker_client.h`. The transitive include via `moonraker_request_tracker.h` should handle this, but watch for compile errors.

2. **Mock protected member access** — The mock heavily uses protected members. Making `discovery_` protected is the cleanest path.

3. **`connection_generation_` access** — Discovery needs this for stale callback detection. A public accessor is the cleanest approach.

4. **`dispatch_status_update()` from discovery** — Discovery's `complete_discovery_subscription()` calls this on the client. Since discovery holds a `MoonrakerClient&`, it can call the public method directly.

5. **Circular include** — `moonraker_discovery_sequence.h` forward-declares `MoonrakerClient` (no include). The `.cpp` file includes `moonraker_client.h`. No circular dependency.
