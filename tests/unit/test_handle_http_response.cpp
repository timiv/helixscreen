// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/api/moonraker_api_internal.h"
#include "hv/requests.h"

#include "../catch_amalgamated.hpp"

using namespace moonraker_internal;

// ============================================================================
// handle_http_response() tests
// ============================================================================

TEST_CASE("handle_http_response", "[api][http]") {
    MoonrakerError captured;
    bool error_called = false;
    auto on_error = [&](const MoonrakerError& e) {
        captured = e;
        error_called = true;
    };

    auto reset = [&]() {
        captured = MoonrakerError{};
        error_called = false;
    };

    SECTION("null response -> CONNECTION_LOST") {
        // null HttpResponsePtr should trigger on_error with CONNECTION_LOST
        std::shared_ptr<HttpResponse> null_resp = nullptr;

        bool result = handle_http_response(null_resp, "test_method", on_error);

        REQUIRE_FALSE(result);
        REQUIRE(error_called);
        REQUIRE(captured.type == MoonrakerErrorType::CONNECTION_LOST);
        REQUIRE(captured.method == "test_method");
        REQUIRE(captured.message.find("No response") != std::string::npos);
    }

    SECTION("404 -> FILE_NOT_FOUND") {
        // 404 status should map to FILE_NOT_FOUND
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_NOT_FOUND;

        bool result = handle_http_response(resp, "download_file", on_error);

        REQUIRE_FALSE(result);
        REQUIRE(error_called);
        REQUIRE(captured.type == MoonrakerErrorType::FILE_NOT_FOUND);
        REQUIRE(captured.code == 404);
    }

    SECTION("401 -> PERMISSION_DENIED") {
        // 401 unauthorized maps to PERMISSION_DENIED (closest semantic match)
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_UNAUTHORIZED;

        bool result = handle_http_response(resp, "upload_file", on_error);

        REQUIRE_FALSE(result);
        REQUIRE(error_called);
        REQUIRE(captured.type == MoonrakerErrorType::PERMISSION_DENIED);
        REQUIRE(captured.code == 401);
    }

    SECTION("403 -> PERMISSION_DENIED") {
        // 403 status should map to PERMISSION_DENIED
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_FORBIDDEN;

        bool result = handle_http_response(resp, "api_call", on_error);

        REQUIRE_FALSE(result);
        REQUIRE(error_called);
        REQUIRE(captured.type == MoonrakerErrorType::PERMISSION_DENIED);
        REQUIRE(captured.code == 403);
    }

    SECTION("500 -> UNKNOWN") {
        // 5xx status codes map to UNKNOWN
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR;

        bool result = handle_http_response(resp, "api_call", on_error);

        REQUIRE_FALSE(result);
        REQUIRE(error_called);
        REQUIRE(captured.type == MoonrakerErrorType::UNKNOWN);
        REQUIRE(captured.code == 500);
    }

    SECTION("200 -> success, no error") {
        // Expected success code returns true, no error callback
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_OK;

        bool result = handle_http_response(resp, "download_file", on_error);

        REQUIRE(result);
        REQUIRE_FALSE(error_called);
    }

    SECTION("custom expected code (201)") {
        // Can pass 201 as expected, 201 returns true
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_CREATED;

        bool result = handle_http_response(resp, "upload_file", on_error, 201);

        REQUIRE(result);
        REQUIRE_FALSE(error_called);
    }

    SECTION("custom expected code (206)") {
        // Partial content for range requests
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_PARTIAL_CONTENT;

        bool result = handle_http_response(resp, "download_file_partial", on_error, 206);

        REQUIRE(result);
        REQUIRE_FALSE(error_called);
    }

    SECTION("multiple expected codes (200 or 206)") {
        // For downloads that accept either 200 or 206
        auto resp200 = std::make_shared<HttpResponse>();
        resp200->status_code = HTTP_STATUS_OK;

        auto resp206 = std::make_shared<HttpResponse>();
        resp206->status_code = HTTP_STATUS_PARTIAL_CONTENT;

        // Can use initializer_list for multiple expected codes
        bool result1 = handle_http_response(resp200, "download", on_error, {200, 206});
        REQUIRE(result1);
        REQUIRE_FALSE(error_called);

        reset();
        bool result2 = handle_http_response(resp206, "download", on_error, {200, 206});
        REQUIRE(result2);
        REQUIRE_FALSE(error_called);
    }

    SECTION("multiple expected codes - failure") {
        // 404 is not in {200, 206}
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_NOT_FOUND;

        bool result = handle_http_response(resp, "download", on_error, {200, 206});

        REQUIRE_FALSE(result);
        REQUIRE(error_called);
        REQUIRE(captured.type == MoonrakerErrorType::FILE_NOT_FOUND);
    }

    SECTION("null callback is safe") {
        // Should not crash with null callback
        auto resp = std::make_shared<HttpResponse>();
        resp->status_code = HTTP_STATUS_NOT_FOUND;

        bool result = handle_http_response(resp, "test", nullptr);

        REQUIRE_FALSE(result);
        // No crash = success
    }

    SECTION("null callback with null response") {
        // Double null case
        std::shared_ptr<HttpResponse> null_resp = nullptr;

        bool result = handle_http_response(null_resp, "test", nullptr);

        REQUIRE_FALSE(result);
        // No crash = success
    }
}
