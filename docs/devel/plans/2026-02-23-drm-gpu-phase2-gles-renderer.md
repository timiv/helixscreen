# Phase 2: G-code OpenGL ES 2.0 Renderer

**Date:** 2026-02-23
**Status:** Plan
**Prerequisite:** Phase 1 complete (DRM+EGL display backend merged)

## Goal

Replace the software TinyGL renderer with GPU-accelerated OpenGL ES 2.0 rendering for the G-code 3D viewer. Remove the TinyGL submodule entirely. Non-GPU platforms keep the 2D layer preview as fallback.

## Design Decisions (Resolved)

### 1. Shared EGL Context (Yes)

**Decision:** Share LVGL's EGL context via `eglCreateContext()` with `share_context` parameter.

**How it works:**
- LVGL's DRM+EGL driver stores its context in `lv_drm_ctx_t` (accessed via `lv_display_get_driver_data()`)
- The `lv_drm_ctx_t` contains an `lv_egl_ctx_t*` with `EGLDisplay`, `EGLContext`, `EGLSurface`
- We add ~20 lines of getter functions to LVGL (minimal patch) to expose these
- Our renderer calls `eglCreateContext(display, config, lvgl_context, attribs)` to create a shared context
- Shared contexts share texture/buffer namespaces — FBO rendered by our context is directly usable

**Pros:**
- Single GPU resource pool (no duplicate context overhead)
- Texture sharing is automatic (no `EGLImage` export/import dance)
- Simpler init — no separate GBM surface or DRM setup
- ~200 lines less code than separate context

**Cons:**
- Must save/restore GL state carefully (mitigated by FBO isolation)
- Tied to LVGL's EGL lifecycle (acceptable — renderer only exists when display does)

### 2. FBO Render Target (Yes)

**Decision:** Render to an offscreen FBO, blit the result into LVGL as an image.

**How it works:**
1. Create FBO with color renderbuffer (RGBA8) + depth renderbuffer (DEPTH16)
2. Bind FBO, render G-code geometry with GLSL shaders
3. `glReadPixels()` the result into an `lv_draw_buf_t`
4. Draw into LVGL via `lv_draw_image()` (same pattern as current TinyGL renderer)

**Why not render directly to display buffer:**
- LVGL uses partial refresh — direct rendering would conflict
- FBO gives clean isolation from LVGL's draw pipeline
- FBO can be cached (skip re-render if camera/data unchanged)
- Resolution can differ from widget size (interaction mode = 50% res)

### 3. Non-GPU Fallback: 2D Layer Preview

**Decision:** Keep the existing 2D layer renderer (`gcode_layer_renderer.cpp`) as the only rendering path on non-GPU platforms.

Platforms without GPU (AD5M, K1, CC1) never see the 3D option. The `ENABLE_GLES_3D` build flag gates compilation. The existing `HELIX_GCODE_MODE` env var controls runtime selection where both are available.

## Architecture

### LVGL EGL Access (Patch)

Add getter functions to LVGL's DRM EGL driver (submitted as patch):

```c
// In lv_linux_drm_egl.h (new public header or added to existing)
EGLDisplay lv_linux_drm_egl_get_display(lv_display_t *disp);
EGLContext lv_linux_drm_egl_get_context(lv_display_t *disp);
EGLConfig  lv_linux_drm_egl_get_config(lv_display_t *disp);
```

Implementation: cast `lv_display_get_driver_data()` to `lv_drm_ctx_t*`, return `ctx->egl_ctx->display` etc. ~20 lines total.

### Renderer Class

```
GCodeGLESRenderer (new)
├── EGL shared context (created once)
├── FBO (color + depth renderbuffers)
├── GLSL program (vertex + fragment shader)
├── VBO/IBO per geometry set (full + coarse LOD)
├── Uniform state (MVP matrix, lighting, colors)
└── lv_draw_buf_t output (same as TinyGL path)
```

**Public API matches `GCodeTinyGLRenderer`** — same `render()`, `set_viewport_size()`, `set_interaction_mode()`, `set_filament_color()`, etc. The UI wrapper (`ui_gcode_viewer.cpp`) switches renderer class based on build flag.

### GLSL Shaders

**Vertex shader** (Gouraud lighting, matching TinyGL's current look):
```glsl
// gcode_gles.vert
uniform mat4 u_mvp;
uniform mat3 u_normal_matrix;
uniform vec3 u_light_dir[2];
uniform vec3 u_light_color[2];
uniform vec3 u_ambient;
uniform vec4 u_base_color;
uniform float u_specular_intensity;
uniform float u_specular_shininess;

attribute vec3 a_position;
attribute vec3 a_normal;

varying vec4 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    vec3 n = normalize(u_normal_matrix * a_normal);
    vec3 diffuse = u_ambient;
    for (int i = 0; i < 2; i++) {
        float NdotL = max(dot(n, u_light_dir[i]), 0.0);
        diffuse += u_light_color[i] * NdotL;
    }
    // Specular (Blinn-Phong, view-space)
    vec3 view_dir = vec3(0.0, 0.0, 1.0);
    float spec = 0.0;
    for (int i = 0; i < 2; i++) {
        vec3 half_dir = normalize(u_light_dir[i] + view_dir);
        spec += pow(max(dot(n, half_dir), 0.0), u_specular_shininess);
    }
    v_color = vec4(u_base_color.rgb * diffuse + vec3(spec * u_specular_intensity), u_base_color.a);
}
```

**Fragment shader:**
```glsl
// gcode_gles.frag
precision mediump float;
varying vec4 v_color;
uniform float u_ghost_alpha; // 1.0 = solid, <1.0 = ghost layer

void main() {
    // Stipple emulation for ghost mode (screen-door transparency)
    if (u_ghost_alpha < 1.0) {
        ivec2 fc = ivec2(gl_FragCoord.xy);
        if (mod(float(fc.x + fc.y), 2.0) > 0.5) discard;
    }
    gl_FragColor = v_color;
}
```

### Geometry Pipeline

Current TinyGL path: `GeometryBuilder` → `PrebuiltGeometry` (interleaved `pos+normal+color` arrays) → `glBegin/glEnd/glVertex3f` per triangle.

New path: `GeometryBuilder` → `PrebuiltGeometry` → **VBO upload** (one-time `glBufferData`) → `glDrawArrays` per draw call.

The `PrebuiltGeometry` struct already stores flat arrays of positions and normals. The port is straightforward:

1. Create VBO from `PrebuiltGeometry::positions` and `PrebuiltGeometry::normals`
2. One VBO per layer group (for layer range filtering)
3. `glDrawArrays(GL_TRIANGLES, ...)` replaces the per-vertex `glBegin/glEnd` loop
4. Coarse LOD geometry gets its own VBO set

### Render Flow

```
1. ui_gcode_viewer.cpp draw event fires
2. GCodeGLESRenderer::render() called
3. Check cached state — skip if camera/data/layer unchanged
4. eglMakeCurrent(shared_context)
5. glBindFramebuffer(GL_FRAMEBUFFER, fbo_)
6. glViewport(0, 0, render_width, render_height)
7. glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
8. Set uniforms (MVP, lighting, colors)
9. For each visible layer range:
   a. Bind layer VBO
   b. Set u_base_color (filament color or ghost color)
   c. Set u_ghost_alpha (1.0 for printed, <1.0 for ghost)
   d. glDrawArrays(GL_TRIANGLES, ...)
10. glReadPixels() → lv_draw_buf_t
11. lv_draw_image() into LVGL layer
12. eglMakeCurrent(EGL_NO_CONTEXT) — release
```

### Threading Model

**Single-threaded** — all GL calls happen on the LVGL thread during the draw event callback. This matches the current TinyGL approach and avoids all multi-threaded GL complexity.

Geometry building (CPU-heavy) stays on background threads as today. Only VBO upload and rendering happen on the LVGL thread.

## File Changes

| File | Change |
|------|--------|
| `src/rendering/gcode_gles_renderer.cpp` | **NEW** — OpenGL ES 2.0 renderer |
| `include/gcode_gles_renderer.h` | **NEW** — Public API (mirrors TinyGL API) |
| `src/rendering/gcode_gles_shaders.h` | **NEW** — Inline GLSL shader source strings |
| `src/ui/ui_gcode_viewer.cpp` | Switch `#ifdef ENABLE_TINYGL_3D` → `#ifdef ENABLE_GLES_3D` |
| `lib/lvgl/` | Patch: add EGL context getters (~20 lines) |
| `lv_conf.h` | No changes needed (EGL already gated) |
| `Makefile` | Add `ENABLE_GLES_3D` flag, new source files |
| `mk/cross.mk` | Set `ENABLE_GLES_3D=yes` for Pi targets with `ENABLE_OPENGLES=yes` |
| `mk/deps.mk` | Remove TinyGL build rules |
| `lib/tinygl/` | Remove submodule |
| `src/rendering/gcode_tinygl_renderer.cpp` | Remove |
| `include/gcode_tinygl_renderer.h` | Remove |

## Task Breakdown

### Task 1: LVGL EGL Getter Patch

Add public getter functions to access EGL context from `lv_linux_drm_egl.c`.

**Files:** `lib/lvgl/src/drivers/display/drm/lv_linux_drm_egl.c`, new header or existing header
**Output:** Patch file in `patches/`
**Estimate:** ~30 lines

### Task 2: GCodeGLESRenderer — Core Rendering

Create the renderer with:
- EGL shared context creation/teardown
- FBO creation/resize
- GLSL shader compilation + program linking
- Basic `render()` with clear + single-color triangle draw
- `glReadPixels()` → `lv_draw_buf_t` → `lv_draw_image()`

**Test:** Render a solid-color triangle visible in the G-code viewer widget.

### Task 3: Geometry Upload + Drawing

Port geometry from `glBegin/glEnd` to VBOs:
- Upload `PrebuiltGeometry` positions + normals to VBOs
- Per-layer-group VBOs for layer range filtering
- `glDrawArrays(GL_TRIANGLES, ...)`
- Coarse LOD VBO set for interaction mode

**Test:** Full G-code model visible with correct geometry.

### Task 4: Lighting + Materials

Wire up GLSL uniforms:
- Two-point studio lighting (matching TinyGL's `setup_lighting()`)
- Filament color
- Specular highlights (Blinn-Phong)
- Ambient term

**Test:** Visual parity with TinyGL renderer.

### Task 5: Ghost Layers + Print Progress

Port ghost/progress visualization:
- Ghost alpha uniform for stipple discard pattern
- Ghost color dimming
- Layer range filtering (min/max layer)
- Print progress layer tracking

**Test:** Ghost layers render with stipple pattern matching TinyGL output.

### Task 6: Interactive Features

Port remaining features:
- Interaction mode (50% resolution during drag)
- Object highlighting (per-object color override)
- Excluded object dimming
- Frustum culling (can reuse existing CPU-side culling)
- Frame skip optimization (cached render state comparison)
- `pick_object()` — ray-based click detection (CPU-side, no GL needed)

### Task 7: Build System + UI Integration

- Add `ENABLE_GLES_3D` build flag
- Wire into `mk/cross.mk` for Pi targets
- Update `ui_gcode_viewer.cpp` to use `GCodeGLESRenderer`
- Remove `ENABLE_TINYGL_3D` flag and TinyGL build rules
- Remove `lib/tinygl/` submodule

### Task 8: Testing + Deployment

- Build with `make pi-docker`
- Deploy to Pi 3/4/5, BTT CB1
- Compare visual output with TinyGL screenshots
- Measure FPS improvement
- Test non-GPU platform build (no 3D, 2D fallback only)
- Test DRM disabled (fbdev) — 3D viewer should gracefully show 2D

## Performance Expectations

| Metric | TinyGL (CPU) | OpenGL ES (GPU) | Notes |
|--------|-------------|-----------------|-------|
| 3M triangles | 3-4 FPS | 30+ FPS | Main win |
| Interaction mode | 8-10 FPS | 60 FPS | Half-res |
| VBO upload | N/A | ~50ms one-time | Geometry change only |
| Memory | ~20MB (framebuffer + zbuffer) | ~5MB (VBOs) + GPU VRAM | Lower CPU memory |
| `glReadPixels` | N/A | ~2ms at 800x480 | Per-frame cost |

## Risks

- **`glReadPixels` latency:** Synchronous GPU→CPU readback. At 800x480 RGBA this is ~1.5MB per frame. Measured ~2ms on Pi 4 which is acceptable. If too slow, can use PBO (pixel buffer object) for async readback.
- **ES 2.0 on Pi 3:** VideoCore IV only supports ES 2.0. Our shaders must stay ES 2.0 compatible (no `layout` qualifiers, no `in`/`out`, use `attribute`/`varying`).
- **State leaks:** Shared EGL context means any GL state we leave dirty could affect LVGL. Mitigated by: bind FBO before all draws, unbind after, use `eglMakeCurrent(EGL_NO_CONTEXT)` when done.
- **Geometry memory:** Large G-code files produce millions of vertices. VBO upload is one-time but could spike memory during the copy. Same as TinyGL — not a regression.

## Non-Goals (Future Work)

- Per-fragment lighting (Phong shading) — Gouraud is sufficient and matches current look
- Anti-aliasing (MSAA) — adds FBO complexity, not needed at 800x480
- GPU-accelerated bed mesh — already 30+ FPS in software, port later if needed
- Texture mapping on geometry — not needed for G-code visualization
- Vulkan — overkill for 2D UI + simple 3D
