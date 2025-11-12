// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MOONRAKER_ERROR_H
#define MOONRAKER_ERROR_H

#include <string>
#include "hv/json.hpp"

using json = nlohmann::json;

/**
 * @brief Error types for Moonraker operations
 */
enum class MoonrakerErrorType {
    NONE,               // No error
    TIMEOUT,            // Request timed out
    CONNECTION_LOST,    // WebSocket connection lost
    JSON_RPC_ERROR,     // JSON-RPC protocol error from Moonraker
    PARSE_ERROR,        // JSON parsing failed
    VALIDATION_ERROR,   // Response validation failed
    NOT_READY,          // Klipper not in ready state
    FILE_NOT_FOUND,     // Requested file doesn't exist
    PERMISSION_DENIED,  // Operation not allowed
    UNKNOWN             // Unknown error
};

/**
 * @brief Comprehensive error information for Moonraker operations
 */
struct MoonrakerError {
    MoonrakerErrorType type = MoonrakerErrorType::NONE;
    int code = 0;                    // JSON-RPC error code if applicable
    std::string message;             // Human-readable error message
    std::string method;              // Method that caused the error
    json details;                    // Additional error details from Moonraker

    /**
     * @brief Check if there's an error
     */
    bool has_error() const { return type != MoonrakerErrorType::NONE; }

    /**
     * @brief Get string representation of error type
     */
    std::string get_type_string() const {
        switch (type) {
            case MoonrakerErrorType::NONE: return "NONE";
            case MoonrakerErrorType::TIMEOUT: return "TIMEOUT";
            case MoonrakerErrorType::CONNECTION_LOST: return "CONNECTION_LOST";
            case MoonrakerErrorType::JSON_RPC_ERROR: return "JSON_RPC_ERROR";
            case MoonrakerErrorType::PARSE_ERROR: return "PARSE_ERROR";
            case MoonrakerErrorType::VALIDATION_ERROR: return "VALIDATION_ERROR";
            case MoonrakerErrorType::NOT_READY: return "NOT_READY";
            case MoonrakerErrorType::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
            case MoonrakerErrorType::PERMISSION_DENIED: return "PERMISSION_DENIED";
            case MoonrakerErrorType::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }

    /**
     * @brief Get a user-friendly error message
     */
    std::string user_message() const {
        if (type == MoonrakerErrorType::TIMEOUT) {
            return "Request timed out. The printer may be busy.";
        } else if (type == MoonrakerErrorType::CONNECTION_LOST) {
            return "Connection to printer lost.";
        } else if (type == MoonrakerErrorType::NOT_READY) {
            return "Printer is not ready. Please wait for initialization.";
        } else if (type == MoonrakerErrorType::FILE_NOT_FOUND) {
            return "File not found on printer.";
        } else if (type == MoonrakerErrorType::PERMISSION_DENIED) {
            return "Permission denied. Check printer configuration.";
        } else if (!message.empty()) {
            return message;
        } else {
            return "An unknown error occurred.";
        }
    }

    /**
     * @brief Create error from JSON-RPC error response
     */
    static MoonrakerError from_json_rpc(const json& error_obj, const std::string& method_name) {
        MoonrakerError err;
        err.type = MoonrakerErrorType::JSON_RPC_ERROR;
        err.method = method_name;

        if (error_obj.contains("code")) {
            err.code = error_obj["code"].get<int>();
        }

        if (error_obj.contains("message")) {
            err.message = error_obj["message"].get<std::string>();
        }

        if (error_obj.contains("data")) {
            err.details = error_obj["data"];
        }

        // Map specific error codes to types
        if (err.code == -32601) {  // Method not found
            err.type = MoonrakerErrorType::VALIDATION_ERROR;
        } else if (err.message.find("not ready") != std::string::npos) {
            err.type = MoonrakerErrorType::NOT_READY;
        } else if (err.message.find("File not found") != std::string::npos) {
            err.type = MoonrakerErrorType::FILE_NOT_FOUND;
        }

        return err;
    }

    /**
     * @brief Create timeout error
     */
    static MoonrakerError timeout(const std::string& method_name, uint32_t timeout_ms) {
        MoonrakerError err;
        err.type = MoonrakerErrorType::TIMEOUT;
        err.method = method_name;
        err.message = "Request timeout after " + std::to_string(timeout_ms) + "ms";
        return err;
    }

    /**
     * @brief Create connection lost error
     */
    static MoonrakerError connection_lost(const std::string& method_name = "") {
        MoonrakerError err;
        err.type = MoonrakerErrorType::CONNECTION_LOST;
        err.method = method_name;
        err.message = "WebSocket connection lost";
        return err;
    }

    /**
     * @brief Create parse error
     */
    static MoonrakerError parse_error(const std::string& what, const std::string& method_name = "") {
        MoonrakerError err;
        err.type = MoonrakerErrorType::PARSE_ERROR;
        err.method = method_name;
        err.message = "JSON parse error: " + what;
        return err;
    }
};

#endif // MOONRAKER_ERROR_H