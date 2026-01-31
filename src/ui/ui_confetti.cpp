// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_confetti.h"

#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"
#include "lvgl/src/xml/lv_xml_parser.h"
#include "lvgl/src/xml/lv_xml_widget.h"
#include "lvgl/src/xml/parsers/lv_xml_obj_parser.h"
#include "theme_manager.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace {

// Physics constants
constexpr float GRAVITY = 400.0f;         // Pixels per second squared
constexpr float AIR_RESISTANCE = 0.98f;   // Velocity multiplier per frame
constexpr float ROTATION_SPEED = 8.0f;    // Radians per second
constexpr float FADE_START = 0.7f;        // Start fading at 70% lifetime
constexpr float PARTICLE_LIFETIME = 3.0f; // Seconds

// Particle shapes
enum class Shape { RECT, SQUARE, CIRCLE };

// Celebration colors (vibrant rainbow)
const std::vector<lv_color_t> CONFETTI_COLORS = {
    lv_color_hex(0xFF6B6B), // Red
    lv_color_hex(0xFFE66D), // Yellow
    lv_color_hex(0x4ECDC4), // Teal
    lv_color_hex(0x45B7D1), // Blue
    lv_color_hex(0x96E6A1), // Green
    lv_color_hex(0xDDA0DD), // Plum
    lv_color_hex(0xF7DC6F), // Gold
    lv_color_hex(0xAED6F1), // Light blue
};

struct Particle {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float rotation;       // Current rotation angle
    float rotation_speed; // Rotation velocity
    float life;           // Remaining lifetime (0-1)
    float size;           // Base size
    lv_color_t color;
    Shape shape;
};

struct ConfettiData {
    lv_obj_t* canvas = nullptr;
    lv_draw_buf_t* draw_buf = nullptr;
    lv_timer_t* timer = nullptr;
    std::vector<Particle> particles;
    int32_t width = 0;
    int32_t height = 0;
    std::mt19937 rng;
    uint32_t last_tick = 0;
};

static std::unordered_map<lv_obj_t*, ConfettiData*> s_registry;

ConfettiData* get_data(lv_obj_t* obj) {
    auto it = s_registry.find(obj);
    return (it != s_registry.end()) ? it->second : nullptr;
}

float random_float(std::mt19937& rng, float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

int random_int(std::mt19937& rng, int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void draw_particle(lv_layer_t* layer, const Particle& p) {
    if (p.life <= 0)
        return;

    // Calculate opacity based on lifetime
    lv_opa_t opa = LV_OPA_COVER;
    if (p.life < FADE_START) {
        opa = (lv_opa_t)(255 * (p.life / FADE_START));
    }

    lv_draw_fill_dsc_t fill_dsc;
    lv_draw_fill_dsc_init(&fill_dsc);
    fill_dsc.color = p.color;
    fill_dsc.opa = opa;

    // Size varies slightly with rotation for visual interest
    float size = p.size * (0.8f + 0.2f * fabsf(cosf(p.rotation)));
    int32_t half = (int32_t)(size / 2);

    // Draw based on shape
    switch (p.shape) {
    case Shape::RECT: {
        // Elongated rectangle
        int32_t w = (int32_t)(size * 1.5f);
        int32_t h = (int32_t)(size * 0.6f);
        // Simple rotation approximation via width/height swap
        if (fabsf(sinf(p.rotation)) > 0.7f)
            std::swap(w, h);
        lv_area_t area = {(int32_t)p.x - w / 2, (int32_t)p.y - h / 2, (int32_t)p.x + w / 2,
                          (int32_t)p.y + h / 2};
        fill_dsc.radius = 2;
        lv_draw_fill(layer, &fill_dsc, &area);
        break;
    }
    case Shape::SQUARE: {
        lv_area_t area = {(int32_t)p.x - half, (int32_t)p.y - half, (int32_t)p.x + half,
                          (int32_t)p.y + half};
        fill_dsc.radius = 1;
        lv_draw_fill(layer, &fill_dsc, &area);
        break;
    }
    case Shape::CIRCLE: {
        lv_area_t area = {(int32_t)p.x - half, (int32_t)p.y - half, (int32_t)p.x + half,
                          (int32_t)p.y + half};
        fill_dsc.radius = LV_RADIUS_CIRCLE;
        lv_draw_fill(layer, &fill_dsc, &area);
        break;
    }
    }
}

void update_and_render(ConfettiData* data) {
    if (!data || !data->canvas || !data->draw_buf)
        return;

    // Calculate delta time
    uint32_t now = lv_tick_get();
    float dt = (now - data->last_tick) / 1000.0f;
    data->last_tick = now;

    // Clamp dt to avoid physics explosions on lag
    dt = std::min(dt, 0.1f);

    // Update particles
    bool any_alive = false;
    for (auto& p : data->particles) {
        if (p.life <= 0)
            continue;

        // Physics update
        p.vy += GRAVITY * dt;
        p.vx *= AIR_RESISTANCE;
        p.vy *= AIR_RESISTANCE;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.rotation += p.rotation_speed * dt;
        p.life -= dt / PARTICLE_LIFETIME;

        // Bounce off bottom with energy loss
        if (p.y > data->height - p.size) {
            p.y = data->height - p.size;
            p.vy = -p.vy * 0.3f;
            p.vx *= 0.8f;
        }

        if (p.life > 0)
            any_alive = true;
    }

    // Clear canvas
    lv_canvas_fill_bg(data->canvas, lv_color_black(), LV_OPA_TRANSP);

    // Render particles
    lv_layer_t layer;
    lv_canvas_init_layer(data->canvas, &layer);

    for (const auto& p : data->particles) {
        draw_particle(&layer, p);
    }

    lv_canvas_finish_layer(data->canvas, &layer);
    lv_obj_invalidate(data->canvas);

    // Stop timer if all particles dead
    if (!any_alive && data->timer) {
        lv_timer_delete(data->timer);
        data->timer = nullptr;
        data->particles.clear();
        spdlog::debug("[Confetti] Animation complete");
    }
}

void timer_cb(lv_timer_t* timer) {
    lv_obj_t* obj = static_cast<lv_obj_t*>(lv_timer_get_user_data(timer));
    ConfettiData* data = get_data(obj);
    if (data) {
        update_and_render(data);
    }
}

void delete_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    ConfettiData* data = get_data(obj);
    if (data) {
        if (data->timer) {
            lv_timer_delete(data->timer);
        }
        if (data->draw_buf) {
            lv_draw_buf_destroy(data->draw_buf);
        }
        s_registry.erase(obj);
        delete data;
        spdlog::debug("[Confetti] Destroyed");
    }
}

lv_obj_t* confetti_create_internal(lv_obj_t* parent) {
    // Get parent dimensions
    int32_t width = lv_obj_get_width(parent);
    int32_t height = lv_obj_get_height(parent);

    // Create canvas
    lv_draw_buf_t* draw_buf =
        lv_draw_buf_create(width, height, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    if (!draw_buf) {
        spdlog::error("[Confetti] Failed to create draw buffer");
        return nullptr;
    }

    lv_obj_t* canvas = lv_canvas_create(parent);
    lv_canvas_set_draw_buf(canvas, draw_buf);
    lv_obj_set_size(canvas, width, height);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);

    // Make click-through
    lv_obj_remove_flag(canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_IGNORE_LAYOUT);

    // Clear to transparent
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_TRANSP);

    // Create data
    ConfettiData* data = new ConfettiData();
    data->canvas = canvas;
    data->draw_buf = draw_buf;
    data->width = width;
    data->height = height;
    data->rng = std::mt19937(std::random_device{}());
    data->last_tick = lv_tick_get();

    s_registry[canvas] = data;

    // Register cleanup
    lv_obj_add_event_cb(canvas, delete_cb, LV_EVENT_DELETE, nullptr);

    spdlog::debug("[Confetti] Created {}x{} canvas", width, height);
    return canvas;
}

void* confetti_xml_create(lv_xml_parser_state_t* state, const char** /*attrs*/) {
    lv_obj_t* parent = static_cast<lv_obj_t*>(lv_xml_state_get_parent(state));
    return confetti_create_internal(parent);
}

void confetti_apply(lv_xml_parser_state_t* state, const char** attrs) {
    lv_xml_obj_apply(state, attrs);
}

} // namespace

lv_obj_t* ui_confetti_create(lv_obj_t* parent) {
    return confetti_create_internal(parent);
}

void ui_confetti_burst(lv_obj_t* confetti, int count) {
    ConfettiData* data = get_data(confetti);
    if (!data) {
        spdlog::warn("[Confetti] burst called on invalid object");
        return;
    }

    // Spawn point (top center)
    float spawn_x = data->width / 2.0f;
    float spawn_y = data->height * 0.2f;

    // Reserve space
    data->particles.reserve(data->particles.size() + count);

    // Spawn particles
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = spawn_x + random_float(data->rng, -50, 50);
        p.y = spawn_y + random_float(data->rng, -30, 30);

        // Burst velocity - fan out and up
        float angle = random_float(data->rng, -2.5f, -0.6f); // Mostly upward
        float speed = random_float(data->rng, 200, 500);
        p.vx = cosf(angle) * speed * random_float(data->rng, 0.5f, 1.5f);
        p.vy = sinf(angle) * speed;

        p.rotation = random_float(data->rng, 0, 6.28f);
        p.rotation_speed = random_float(data->rng, -ROTATION_SPEED, ROTATION_SPEED);
        p.life = 1.0f;
        p.size = random_float(data->rng, 6, 14);
        p.color = CONFETTI_COLORS[random_int(data->rng, 0, CONFETTI_COLORS.size() - 1)];
        p.shape = static_cast<Shape>(random_int(data->rng, 0, 2));

        data->particles.push_back(p);
    }

    // Start animation timer if not running
    if (!data->timer) {
        data->last_tick = lv_tick_get();
        data->timer = lv_timer_create(timer_cb, 16, confetti); // ~60 FPS
        spdlog::debug("[Confetti] Burst {} particles", count);
    }
}

void ui_confetti_clear(lv_obj_t* confetti) {
    ConfettiData* data = get_data(confetti);
    if (!data)
        return;

    if (data->timer) {
        lv_timer_delete(data->timer);
        data->timer = nullptr;
    }
    data->particles.clear();
    lv_canvas_fill_bg(data->canvas, lv_color_black(), LV_OPA_TRANSP);
    lv_obj_invalidate(data->canvas);
}

void ui_confetti_init() {
    lv_xml_register_widget("ui_confetti", confetti_xml_create, confetti_apply);
    spdlog::debug("[Confetti] Registered ui_confetti widget");
}
