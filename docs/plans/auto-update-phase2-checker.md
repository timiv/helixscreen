# Phase 2: Update Checker Service

**Status**: COMPLETE
**Depends on**: Phase 1 (SSL Enablement) - COMPLETE
**Estimated complexity**: Medium (2 new files, follows existing patterns)

## Goal

Create a service that checks GitHub releases API for updates and provides update status to the UI.

**Note**: LVGL subjects for UI binding are deferred to Phase 3 (Settings UI Integration). The current implementation uses callback-based API which is sufficient for initial integration.

## Files to Create

1. `src/system/update_checker.h` - Interface
2. `src/system/update_checker.cpp` - Implementation

## API Design

```cpp
// src/system/update_checker.h

class UpdateChecker {
public:
    struct ReleaseInfo {
        std::string version;        // e.g., "1.2.3"
        std::string tag_name;       // e.g., "v1.2.3"
        std::string download_url;   // Asset URL for binary
        std::string release_notes;  // Body markdown
        std::string published_at;   // ISO 8601 timestamp
    };

    enum class Status {
        Idle,            // No check in progress
        Checking,        // HTTP request pending
        UpdateAvailable, // New version found
        UpToDate,        // Already on latest
        Error            // Check failed (network, parse, etc.)
    };

    // Singleton access
    static UpdateChecker& instance();

    // Check for updates (runs in background thread)
    // Callback invoked on LVGL thread via ui_queue_update
    using Callback = std::function<void(Status, std::optional<ReleaseInfo>)>;
    void check_for_updates(Callback callback = nullptr);

    // Getters (thread-safe)
    Status get_status() const;
    std::optional<ReleaseInfo> get_cached_update() const;
    bool has_update_available() const;
    std::string get_error_message() const;

    // Cache control
    void clear_cache();  // Force fresh check on next call

    // LVGL subjects for UI binding
    lv_subject_t* status_subject();      // int: Status enum
    lv_subject_t* version_subject();     // string: new version or empty
    lv_subject_t* checking_subject();    // int: 1 if checking, 0 otherwise

    // Lifecycle
    void init();
    void shutdown();

private:
    UpdateChecker() = default;
    ~UpdateChecker();

    void do_check();  // Worker thread entry
    void report_result(Status status, std::optional<ReleaseInfo> info, const std::string& error);

    // State
    std::atomic<Status> status_{Status::Idle};
    std::optional<ReleaseInfo> cached_info_;
    std::string error_message_;
    mutable std::mutex mutex_;

    // Rate limiting
    std::chrono::steady_clock::time_point last_check_time_{};
    static constexpr std::chrono::hours MIN_CHECK_INTERVAL{1};

    // Threading
    std::thread worker_thread_;
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> shutting_down_{false};
    Callback pending_callback_;

    // LVGL subjects
    lv_subject_t status_subject_;
    lv_subject_t version_subject_;
    lv_subject_t checking_subject_;
    bool subjects_initialized_{false};
};
```

## Implementation Details

### HTTP Request (follows moonraker_api_rest.cpp pattern)

```cpp
void UpdateChecker::do_check() {
    // Thread entry point
    auto req = std::make_shared<HttpRequest>();
    req->method = HTTP_GET;
    req->url = "https://api.github.com/repos/prestonbrown/helixscreen/releases/latest";
    req->timeout = 30;
    req->headers["User-Agent"] = "HelixScreen/" HELIX_VERSION;
    req->headers["Accept"] = "application/vnd.github.v3+json";

    auto resp = requests::request(req);

    if (cancelled_.load() || shutting_down_.load()) {
        return;  // Bail out cleanly
    }

    if (!resp || resp->status_code != 200) {
        report_result(Status::Error, std::nullopt,
                      resp ? "HTTP " + std::to_string(resp->status_code) : "No response");
        return;
    }

    // Parse JSON
    try {
        auto j = json::parse(resp->body);
        ReleaseInfo info;
        info.tag_name = j.value("tag_name", "");
        info.release_notes = j.value("body", "");
        info.published_at = j.value("published_at", "");

        // Strip 'v' prefix for comparison
        info.version = info.tag_name;
        if (!info.version.empty() && (info.version[0] == 'v' || info.version[0] == 'V')) {
            info.version = info.version.substr(1);
        }

        // Find binary asset URL (look for .tar.gz or platform-specific)
        if (j.contains("assets") && j["assets"].is_array()) {
            for (const auto& asset : j["assets"]) {
                std::string name = asset.value("name", "");
                if (name.find(".tar.gz") != std::string::npos) {
                    info.download_url = asset.value("browser_download_url", "");
                    break;
                }
            }
        }

        // Compare versions
        auto current = helix::version::parse_version(HELIX_VERSION);
        auto latest = helix::version::parse_version(info.version);

        if (!current || !latest) {
            report_result(Status::Error, std::nullopt, "Failed to parse version");
            return;
        }

        if (*latest > *current) {
            report_result(Status::UpdateAvailable, info, "");
        } else {
            report_result(Status::UpToDate, std::nullopt, "");
        }

    } catch (const json::exception& e) {
        report_result(Status::Error, std::nullopt, std::string("JSON parse error: ") + e.what());
    }
}
```

### Thread Safety (follows NetworkTester pattern)

```cpp
void UpdateChecker::report_result(Status status, std::optional<ReleaseInfo> info,
                                   const std::string& error) {
    // Must dispatch to LVGL thread!
    ui_queue_update([this, status, info, error]() {
        std::lock_guard<std::mutex> lock(mutex_);

        status_ = status;
        error_message_ = error;
        cached_info_ = info;
        last_check_time_ = std::chrono::steady_clock::now();

        // Update LVGL subjects
        if (subjects_initialized_) {
            lv_subject_set_int(&status_subject_, static_cast<int>(status));
            lv_subject_set_int(&checking_subject_, 0);

            if (info) {
                // Note: Subject strings need careful lifetime management
                // Using a static buffer or member string that outlives the subject
                lv_subject_copy_string(&version_subject_, info->version.c_str());
            } else {
                lv_subject_copy_string(&version_subject_, "");
            }
        }

        // Invoke user callback
        if (pending_callback_) {
            pending_callback_(status, info);
            pending_callback_ = nullptr;
        }
    });
}
```

### Rate Limiting

```cpp
void UpdateChecker::check_for_updates(Callback callback) {
    if (shutting_down_.load()) {
        return;
    }

    // Rate limit check
    auto now = std::chrono::steady_clock::now();
    if (status_ != Status::Idle && status_ != Status::Error) {
        auto elapsed = now - last_check_time_;
        if (elapsed < MIN_CHECK_INTERVAL && cached_info_.has_value()) {
            // Return cached result
            spdlog::debug("[UpdateChecker] Using cached result (checked {} mins ago)",
                          std::chrono::duration_cast<std::chrono::minutes>(elapsed).count());
            if (callback) {
                ui_queue_update([this, callback]() {
                    callback(status_.load(), cached_info_);
                });
            }
            return;
        }
    }

    // Join previous thread if needed
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    pending_callback_ = callback;
    status_ = Status::Checking;
    cancelled_ = false;

    if (subjects_initialized_) {
        ui_queue_update([this]() {
            lv_subject_set_int(&checking_subject_, 1);
        });
    }

    worker_thread_ = std::thread(&UpdateChecker::do_check, this);
}
```

## Build Integration

Add to `mk/sources.mk`:
```makefile
SYSTEM_SOURCES += src/system/update_checker.cpp
```

## Testing Strategy

1. **Unit test**: Mock HTTP response, verify version comparison logic
2. **Integration test**: Real HTTP call in dev environment with `-vv` logging
3. **Manual test**: Deploy to AD5M, verify check completes with real GitHub API

### Test File: `tests/update_checker_test.cpp`

```cpp
TEST_CASE("UpdateChecker version comparison", "[update_checker]") {
    // Test that version parsing and comparison works
    auto v1 = helix::version::parse_version("1.0.0");
    auto v2 = helix::version::parse_version("1.0.1");
    REQUIRE(v1.has_value());
    REQUIRE(v2.has_value());
    REQUIRE(*v2 > *v1);
}

TEST_CASE("UpdateChecker parses GitHub release JSON", "[update_checker]") {
    // Mock JSON response
    const char* json_str = R"({
        "tag_name": "v1.2.3",
        "body": "Release notes here",
        "published_at": "2025-01-15T10:00:00Z",
        "assets": [{
            "name": "helixscreen-1.2.3.tar.gz",
            "browser_download_url": "https://github.com/.../helixscreen-1.2.3.tar.gz"
        }]
    })";

    auto j = json::parse(json_str);
    // ... verify parsing
}
```

## Check Triggers (Phase 3 Scope)

For this phase, provide manual API only. Automatic triggers will be added in Phase 3:

1. **Startup check**: 5 seconds after UI ready (background timer)
2. **Settings button**: "Check for Updates" button
3. **Daily timer**: If idle for 24 hours (not printing)

## Error Handling

| Scenario | Behavior |
|----------|----------|
| No network | Status::Error, error_message set, retry on next manual check |
| GitHub rate limit (403) | Status::Error, respect Retry-After header if present |
| Invalid JSON | Status::Error, log parse error |
| Version parse failure | Status::Error, log details |
| Request timeout | Status::Error, 30s timeout |

## Checklist

- [x] Create `include/system/update_checker.h`
- [x] Create `src/system/update_checker.cpp`
- [x] Add to `mk/sources.mk`
- [x] Create `tests/unit/test_update_checker.cpp` (14 test cases, 111 assertions)
- [x] Verify builds locally (`make -j`)
- [ ] Verify AD5M build (`make ad5m-docker`)
- [x] Test with `-vv` logging on desktop
- [ ] Deploy to AD5M and verify real GitHub API call works
- [x] Document in this file when complete

## Dependencies

- `hv/requests.h` - HTTP client (already available)
- `hv/json.hpp` - JSON parsing (already available)
- `version.h` - Version comparison (already available)
- `ui_update_queue.h` - Thread-safe UI updates (already available)

## Notes

- GitHub API has rate limits: 60 requests/hour unauthenticated. Our 1-hour cache respects this.
- Release asset naming will need to be defined when we add release builds (Phase 4+)
- The service is stateless across restarts - no persistent cache file needed initially
