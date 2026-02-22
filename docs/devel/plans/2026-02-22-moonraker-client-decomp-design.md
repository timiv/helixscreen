# MoonrakerClient Decomposition Design

> **Created**: 2026-02-22
> **Status**: Approved
> **Approach**: Three-way split (internal restructuring, no consumer changes)
> **Goal**: Readability/maintainability — each file has one clear responsibility

## Problem

`MoonrakerClient` is a 2,553-line monolith (699-line header + 1,854-line impl) mixing:
- WebSocket connection lifecycle and reconnection
- JSON-RPC request/response protocol with timeout tracking
- Multi-step async printer discovery orchestration
- Subscription and notification dispatch
- Event system and modal suppression

The `connect()` method alone is 424 lines. The discovery sequence (`continue_discovery()` + `complete_discovery_subscription()`) is 514 lines of deeply nested async callbacks.

## Design

### Component 1: `MoonrakerDiscoverySequence` (~600 lines)

Owns the entire multi-step async printer discovery flow.

**What moves:**
- `discover_printer()` — entry point, `server.connection.identify`
- `continue_discovery()` — steps 1-5: object list, server info, printer info, config query, MCU queries
- `complete_discovery_subscription()` — builds subscription JSON, subscribes to all discovered objects
- `parse_objects()` — categorizes Klipper objects into typed vectors (heaters, sensors, fans, etc.)
- `parse_bed_mesh()` — callback dispatch
- Hardware vectors: `heaters_`, `sensors_`, `fans_`, `leds_`, `steppers_`, `afc_objects_`, `filament_sensors_`
- `hardware_` (`PrinterDiscovery` member)
- Discovery callbacks: `on_hardware_discovered_`, `on_discovery_complete_`
- `identified_` atomic

**Interface:**
- Constructor takes `MoonrakerClient&` for sending JSON-RPC requests
- `start(on_connected_cb, on_discovery_complete_cb)` replaces `discover_printer()`
- `hardware()` accessor for discovered hardware data
- Client calls `discovery_.start()` from `onopen` callback

**Files:** `include/moonraker_discovery_sequence.h`, `src/printer/moonraker_discovery_sequence.cpp`

### Component 2: `MoonrakerRequestTracker` (~250 lines)

Owns pending request lifecycle — registration, timeout checking, response routing, cleanup.

**What moves:**
- `pending_requests_` map + `requests_mutex_`
- `request_id_` atomic
- Request timeout configuration
- Request registration (ID generation, map insertion, timeout setup)
- `cancel_request()`
- `check_request_timeouts()`
- `cleanup_pending_requests()`
- Response routing: `route_response(json)` — match response ID to pending callback

**Interface:**
- `send(WebSocketClient&, method, params, success_cb, error_cb, timeout_ms, silent)` — builds JSON-RPC, sends, registers pending
- `route_response(const json& msg)` → `bool` — returns true if message was a request response
- `cancel(request_id)` — abort specific request
- `check_timeouts(event_emitter)` — scan for timed-out requests
- `cleanup_all()` — called on disconnect, invokes error callbacks for all pending

**Files:** `include/moonraker_request_tracker.h`, `src/printer/moonraker_request_tracker.cpp`

### Component 3: MoonrakerClient Residual (~1,000 lines)

What stays:
- WebSocket lifecycle: `connect()`, `disconnect()`, `force_reconnect()`
- Connection state machine: `set_connection_state()`, atomics, reconnection logic
- Subscription management: `register_notify_update()`, `unsubscribe_notify_update()`, `register_method_callback()`, `dispatch_status_update()`
- Event system: `emit_event()`, `register_event_handler()`, modal suppression
- Utility: `gcode_script()`, `get_gcode_store()`, callback setters

The `connect()` method shrinks because:
- Request routing delegates to `tracker_.route_response(msg)`
- Discovery kickoff becomes `discovery_.start()`

## Key Decisions

1. **Composition** — Client owns `MoonrakerDiscoverySequence discovery_` and `MoonrakerRequestTracker tracker_` as members
2. **No consumer changes** — All consumers go through MoonrakerAPI, not MoonrakerClient directly. Internal restructuring only.
3. **Back-reference pattern** — Discovery takes `MoonrakerClient&` to call `send_jsonrpc()`. Same pattern as domain APIs.
4. **MockMoonrakerClient** — Needs adaptation for new discovery interface. Mock is test-only, small scope.
5. **Two-phase lock pattern preserved** — Both extracted classes maintain the "copy under lock, invoke outside lock" callback pattern.

## Files Changed

| File | Change |
|------|--------|
| `include/moonraker_discovery_sequence.h` | NEW |
| `src/printer/moonraker_discovery_sequence.cpp` | NEW |
| `include/moonraker_request_tracker.h` | NEW |
| `src/printer/moonraker_request_tracker.cpp` | NEW |
| `include/moonraker_client.h` | Shrink: remove moved members, add composition members |
| `src/printer/moonraker_client.cpp` | Shrink: delegate to discovery_ and tracker_ |
| `include/moonraker_mock_client.h` | Adapt to new discovery interface |
| `src/printer/moonraker_mock_client.cpp` | Adapt to new discovery interface |

## Metrics

| Metric | Before | After (Target) |
|--------|--------|----------------|
| moonraker_client.h lines | 699 | ~400 |
| moonraker_client.cpp lines | 1,854 | ~1,000 |
| connect() lines | 424 | ~300 |
| Responsibility areas | 5+ mixed | 1 per file |
| Files | 2 | 6 (2 existing + 4 new) |
