// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

/**
 * @file thumbnail_load_context.h
 * @brief Async safety context for thumbnail loading operations
 *
 * ThumbnailLoadContext encapsulates the common pattern used across panels
 * when loading thumbnails asynchronously:
 * 1. An `alive` flag to detect if the caller was destroyed
 * 2. A generation counter to detect stale callbacks
 *
 * This eliminates the repeated boilerplate of capturing these values
 * and checking them in every async callback.
 *
 * ## Usage Example
 * ```cpp
 * // In your panel class:
 * std::shared_ptr<std::atomic<bool>> m_alive;
 * std::atomic<uint32_t> thumbnail_gen_{0};
 *
 * void load_thumbnail() {
 *     auto ctx = ThumbnailLoadContext::create(m_alive, &thumbnail_gen_);
 *
 *     get_thumbnail_cache().fetch_for_detail_view(
 *         api_, path, ctx,
 *         [this, ctx](const std::string& lvgl_path) {
 *             if (!ctx.is_valid()) return;  // One-liner safety check
 *             // ... use lvgl_path ...
 *         });
 * }
 * ```
 *
 * @see ThumbnailCache::fetch_for_detail_view
 * @see ThumbnailCache::fetch_for_card_view
 */
struct ThumbnailLoadContext {
    /// Shared flag indicating if the owner object is still alive
    std::shared_ptr<std::atomic<bool>> alive;

    /// Pointer to the owner's generation counter (may be nullptr if not used)
    std::atomic<uint32_t>* generation;

    /// The generation value captured at creation time
    uint32_t captured_gen;

    /**
     * @brief Check if this context is still valid
     *
     * A context is valid if:
     * 1. The alive flag is still true (owner not destroyed)
     * 2. The generation counter hasn't changed (no newer request superseded this one)
     *
     * @return true if the callback should proceed, false if it should abort
     */
    [[nodiscard]] bool is_valid() const {
        // Check alive flag first (most likely failure case during shutdown)
        if (!alive || !alive->load()) {
            return false;
        }
        // If no generation tracking, always valid
        if (generation == nullptr) {
            return true;
        }
        // Check generation hasn't changed
        return captured_gen == generation->load();
    }

    /**
     * @brief Create a context, incrementing the generation counter
     *
     * This is the recommended way to create a context. It atomically
     * increments the generation counter and captures the new value.
     *
     * @param alive_flag Shared alive flag from the calling object
     * @param gen Pointer to generation counter (nullptr if not used)
     * @return A context that can be passed to async callbacks
     */
    static ThumbnailLoadContext create(std::shared_ptr<std::atomic<bool>> alive_flag,
                                       std::atomic<uint32_t>* gen = nullptr) {
        ThumbnailLoadContext ctx;
        ctx.alive = std::move(alive_flag);
        ctx.generation = gen;
        ctx.captured_gen = gen ? ++(*gen) : 0;
        return ctx;
    }

    /**
     * @brief Create a context without incrementing generation
     *
     * Use this when you need to check validity but don't want to
     * invalidate previous callbacks (e.g., for chained operations).
     *
     * @param alive_flag Shared alive flag from the calling object
     * @param gen Pointer to generation counter (nullptr if not used)
     * @return A context that captures current generation without incrementing
     */
    static ThumbnailLoadContext capture(std::shared_ptr<std::atomic<bool>> alive_flag,
                                        std::atomic<uint32_t>* gen = nullptr) {
        ThumbnailLoadContext ctx;
        ctx.alive = std::move(alive_flag);
        ctx.generation = gen;
        ctx.captured_gen = gen ? gen->load() : 0;
        return ctx;
    }
};
