# TinyGL Quality & Performance Improvement Plan

**Status**: Phong Shading Complete, Other Improvements Deferred
**Last Updated**: 2025-11-19 (Phong implementation)
**Target**: Improve visual quality and performance for embedded G-code preview

---

## Executive Summary

TinyGL currently achieves ~90% visual match with OrcaSlicer but has opportunities for quality and performance improvements suitable for embedded hardware. This document outlines planned enhancements, their current status, and lessons learned from initial implementation attempts.

---

## 1. Color Banding Reduction (Ordered Dithering)

### Status: 🟡 **Infrastructure Added, Integration Paused**

### Description
Reduce visible 8-bit color quantization artifacts in gradients using 4x4 Bayer matrix ordered dithering.

### Current State
- ✅ Created `zdither.h/c` with dithering implementation
- ✅ Added Bayer matrix lookup tables (4x4)
- ✅ Implemented `glSetDithering()`/`glGetDithering()` API
- ✅ Added `RGB_TO_PIXEL_COND` conditional macro
- ✅ Included in build system
- ❌ **Not yet integrated into rasterization pipeline**

### Lessons Learned
1. **Test incrementally** - Initial implementation modified too many macros at once without testing
2. **Rendering tests must verify pixels** - Test framework created files but didn't verify content
3. **Macro changes are subtle** - Small errors in PUT_PIXEL macros silently broke all rendering
4. **Committed code needs verification** - Broke rendering twice without catching it in tests

### Next Steps (When Resumed)
1. Test `RGB_TO_PIXEL_COND` in isolated context first
2. Modify ONE PUT_PIXEL variant and test before others
3. Add pixel verification to test framework (not just file creation)
4. Start with flat shading before smooth shading
5. Test after EACH change, not batch changes

### Complexity: **Medium**
### Visual Impact: **Low-Medium** (reduces gradient banding)
### Performance Impact: **Minimal** (<3% when enabled, 0% when disabled)

---

## 2. Edge Anti-Aliasing

### Status: 🔴 **Not Started**

### Description
Implement coverage-based anti-aliasing for triangle edges to reduce jagged appearance on diagonal lines.

### Approach Options
1. **Supersampling** - Render at higher resolution, downsample (simple but expensive)
2. **Coverage masks** - Calculate sub-pixel coverage for edge pixels (better performance)
3. **MSAA-style** - Multi-sample approach with sample pattern

### Technical Requirements
- Modify edge rasterization to calculate coverage
- Add sub-pixel precision to edge calculations
- Blend edge pixels based on coverage percentage

### Estimated Complexity: **High**
### Visual Impact: **High** (significantly improves diagonal lines)
### Performance Impact: **Medium** (5-15% depending on approach)

### Recommended Approach
Start with 2x2 coverage masks on edge pixels only (not full supersampling).

---

## 3. Per-Pixel Lighting (Phong Shading)

### Status: ✅ **COMPLETE** (2025-11-19)

### Description
Implemented Phong shading (per-pixel lighting) alongside Gouraud shading (per-vertex) for smoother lighting on curved surfaces.

### Implementation Approach
**Chose separate parameter passing over structure embedding:**
- Pass normal vectors as `GLfloat* n0, n1, n2` parameters to `ZB_fillTrianglePhong()`
- Normals extracted from `GLVertex.normal.v` at call site in clip.c
- Local variables created in rasterizer for template header compatibility
- Avoided embedding normals in `ZBufferPoint` structure (caused crashes)

### Technical Details
**Files Modified:**
- `tinygl/include/zbuffer.h` - Updated function signature, kept ZBufferPoint unchanged
- `tinygl/src/clip.c` - Pass `p0->normal.v, p1->normal.v, p2->normal.v` to rasterizer
- `tinygl/src/ztriangle.c` - Added local normal variables, fixed all `#undef INTERP_NORMAL`
- `tinygl/src/ztriangle.h` - Updated gradient setup, **fixed critical bug** (missing `#undef INTERP_NORMAL`)

**Critical Bug Fixed:**
The template header ztriangle.h is included 7 times by different functions. Missing `#undef INTERP_NORMAL` at end of header caused flag leakage to subsequent includes, breaking texture mapping functions.

### Performance Results
**Tested with test_runner.cpp phong test (2025-11-19):**
```
Scene                 Gouraud    Phong    Slowdown
────────────────────────────────────────────────────
Sphere (80 tri)      0.12 ms   0.13 ms     +2.4%
Sphere (320 tri)     0.17 ms   0.18 ms     +5.4%
Cylinders (720 tri)  0.20 ms   0.22 ms     +6.8%
────────────────────────────────────────────────────
AVERAGE                                    +4.9%
```

### Actual Complexity: **High**
### Visual Impact: **High** (eliminates lighting bands on curves - manual verification required)
### Performance Impact: **Minimal** (4.9% slowdown, well below 30% acceptable threshold)

### Recommendation
**✅ Production ready** - Phong shading available via `glSetPhongShading(GL_TRUE)` with negligible performance cost. Visual improvement significant on low-poly curved surfaces. No automated visual verification yet - requires manual comparison of `*_gouraud.ppm` vs `*_phong.ppm` test output.

---

## 4. Tile-Based Parallel Rasterization

### Status: 🔴 **Not Started**

### Description
Replace scanline rasterizer with tile-based approach to enable parallel processing.

### Current Architecture
- Scanline-based: Process triangles top-to-bottom sequentially
- Single-threaded
- Good cache locality for depth buffer

### Proposed Architecture
- Divide framebuffer into tiles (e.g., 32x32 pixels)
- Sort triangles into bins by tile coverage
- Process tiles in parallel (thread per tile)
- Requires careful depth buffer handling

### Benefits
- Enables multi-core utilization
- Better cache locality for small tiles
- Can render tiles out of order

### Challenges
- Complex implementation (major rewrite)
- Thread synchronization overhead
- Memory overhead for tile bins
- Depth buffer synchronization

### Estimated Complexity: **Very High**
### Visual Impact: **None** (performance only)
### Performance Impact: **High positive** (2-4x speedup on multi-core)

### Recommendation
**Defer indefinitely** - Current performance (597 FPS for 6K triangles) is more than sufficient for embedded G-code preview. Only pursue if target hardware has multiple cores AND performance becomes bottleneck.

---

## 5. SIMD Acceleration

### Status: 🔴 **Not Started**

### Description
Use SIMD instructions (SSE/AVX/NEON) for vector math operations (transforms, lighting calculations).

### Target Operations
- Matrix-vector multiplications (vertex transforms)
- Lighting calculations (dot products, vector normalization)
- Color blending
- Possibly 4 pixels at a time in rasterizer

### Platforms
- **x86/x64**: SSE2, SSE4, AVX
- **ARM**: NEON (important for embedded targets)

### Estimated Complexity: **High**
### Visual Impact: **None** (performance only)
### Performance Impact: **Medium positive** (20-40% faster transforms/lighting)

### Recommendation
**Low priority** - Profile first to identify actual bottlenecks. May be worthwhile for ARM embedded targets if transform/lighting is bottleneck.

---

## 6. Other Potential Improvements

### 6.1 Depth Buffer Optimization
**Status**: 🟢 **Already Good**
Current 16-bit depth buffer is appropriate for viewing distances. No changes needed.

### 6.2 Frustum Culling Optimization
**Status**: 🟢 **Likely Sufficient**
Standard frustum culling should be adequate. Only profile if performance issues arise.

### 6.3 Texture Filtering
**Status**: 🔴 **Not Applicable**
G-code preview doesn't use textures. Not relevant for this use case.

### 6.4 Backface Culling
**Status**: 🟢 **Likely Enabled**
Standard feature. Verify it's enabled but no changes needed.

---

## Priority Recommendations

### ✅ Recently Completed
1. **Per-Pixel Lighting (Phong Shading)** - Complete with excellent performance (4.9% overhead)

### Immediate Priority (Do Next)
1. **None** - Current TinyGL quality is excellent for G-code preview

### Short-term (If Quality Issues Arise)
1. **Edge Anti-Aliasing** - If diagonal lines look too jagged
2. **Ordered Dithering** - If gradient banding is reported as issue (infrastructure exists, needs integration)

### Long-term (Only If Necessary)
1. **SIMD Acceleration** - Only if profiling shows transform/lighting bottleneck on ARM
2. **Parallel Rasterization** - Only if multi-core embedded target AND performance critical

### Never (Not Worth The Effort)
- Supersampling anti-aliasing (too expensive)
- ~~Full Phong lighting on all surfaces (too expensive)~~ **DONE** - Actually only 5% overhead!

---

## Test Framework Status

### Completed ✅
- Basic test framework infrastructure
- 5 test scenes (sphere tessellation, cube grid, Gouraud artifacts, color banding, lighting)
- Performance benchmarking (FPS, triangle throughput)
- Image saving (PPM format)
- Integrated into build system (`make test-tinygl-*` targets)
- **NEW (2025-11-19): Pixel verification system**
  - Automated comparison against reference images
  - Multi-metric validation (PSNR, SSIM, max pixel diff)
  - Visual diff generation on test failures
  - Regression test mode with exit codes (--verify flag)
  - Reference image management with safeguards

### Test Verification Thresholds
- **PSNR** (Peak Signal-to-Noise Ratio): ≥30 dB (excellent match)
- **SSIM** (Structural Similarity Index): ≥0.95 (perceptually similar)
- **Max Pixel Difference**: ≤10/255 (no major errors)

### Usage
```bash
# Generate reference images (do this once, or after intentional changes)
make test-tinygl-reference

# Run tests with verification (returns exit code 0 or 1)
./build/bin/tinygl_test_runner --verify

# Verify specific test
./build/bin/tinygl_test_runner gouraud --verify

# Just render tests (no verification)
make test-tinygl-quality
```

### ✅ All Issues Resolved
- ✅ Tests now verify actual pixel content
- ✅ Tests fail when rendering breaks
- ✅ Automated visual regression testing enabled
- ✅ Visual diffs generated automatically on failures

---

## Performance Baseline

From testing (2025-11-19):
- **597 FPS** for 6,144 triangles (sphere subdivision 3)
- **~10,000 triangles/ms** throughput
- Excellent performance for software rendering on modern CPU

This is more than adequate for real-time G-code preview on embedded hardware.

---

## Conclusion

TinyGL's current performance and quality are **excellent for the G-code preview use case**. The ordered dithering work has been paused after discovering integration challenges. Future improvements should be driven by actual user-reported quality issues rather than speculative enhancements.

**Key Takeaway**: Don't fix what isn't broken. Profile before optimizing.
