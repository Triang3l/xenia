/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_PIPELINE_STATE_H_
#define XENIA_GPU_PIPELINE_STATE_H_

#include <array>
#include <cstdint>

#include "xenia/gpu/register_file.h"
#include "xenia/gpu/registers.h"
#include "xenia/gpu/xenos.h"

namespace xe {
namespace gpu {

// Helpers for graphics pipeline state, including normalization of state.
//
// Normalization serves multiple purposes:
// - Expansion of state variables that override other variables, such as
//   RB_DEPTHCONTROL::Z_ENABLE = 0 making the Z test behave like
//   Z_WRITE_ENABLE = 0 and ZFUNC = ALWAYS regardless of their values. This
//   makes it possible to check just one variable instead of two.
// - Detecting opportunities where a potential fast path may be taken in the
//   host GPU hardware / driver or the emulation logic (or where it can be
//   simplified). This includes the opposite of state variable expansion -
//   combining granular state into broader state variables: in the example
//   above, Z_WRITE_ENABLE = 0 and ZFUNC = ALWAYS make the Z test a no-op, so
//   Z_ENABLE can be set to 0 if this configuration is detected.
// - Setting unused variables (or variable bits) to constant values to reduce
//   the number of state combinations in hardware-accelerated emulation
//   implementations, and to avoid host GPU context rolls when a state change
//   would be irrelevant.

struct ScissorState {
  std::array<uint_fast16_t, 2> tl{};
  std::array<uint_fast16_t, 2> br{};

  // Initialize to empty.
  explicit ScissorState() = default;

  explicit ScissorState(const RegisterFile& regs);

  // Empty scissor is always placed at {0, 0} so that any code using
  // ScissorState for estimating the potentially covered region of the surface,
  // such as for the height to get the EDRAM range needed, based on the
  // bottom-right extent will trivially get the smallest estimate if nothing
  // will be drawn anyway.
  bool IsEmpty() const;
};

struct PrimitiveState {
  enum class AssembledPrimitiveType {
    kInvalid,
    kPoint,
    kLine,
    kTriangleTwoFaced,
    kRectangle,
  };

  // Can be packed in uint32_t and zero-initialized.
  enum class TessellationDomain {
    kNone = 0,

    kLinePrimitive,
    kLinePatch,

    kTrianglePrimitive,
    kTrianglePatch,

    kQuadPrimitive,
    kQuadPatch,

    kCount,
  };

  xenos::PrimitiveType primitive_type;

  bool major_mode_explicit;

  TessellationDomain tessellation_domain;
  xenos::TessellationMode tessellation_mode;
  // For non-adaptive modes, `tessellation_max_level` is the level.
  // To convert to the Direct3D 11 tessellation factors, add 1.
  // https://www.slideshare.net/blackdevilvikas/next-generation-graphics-programming-on-xbox-360
  float tessellation_max_level;
  float tessellation_min_level;

  AssembledPrimitiveType assembled_primitive_type;

  // If only one of the two faces is rasterized, the values for the rasterized
  // face will be used for the unused face too. This simplifies targeting host
  // GPU APIs that don't support per-face state (they can unconditionally use
  // the state for the front face).

  bool cull_front;
  bool cull_back;

  bool front_clockwise;

  xenos::PolygonType polygon_type_front;
  xenos::PolygonType polygon_type_back;

  // Slope scales for 1/16 subpixels
  // (see xenos::kPolygonOffsetScaleSubpixelUnit).
  float polygon_offset_front_subpixel_slope_scale;
  float polygon_offset_front_offset;
  float polygon_offset_back_subpixel_slope_scale;
  float polygon_offset_back_offset;

  bool all_primitives_killed_or_culled;

  explicit PrimitiveState(const RegisterFile& regs, bool scissor_is_empty);

  bool ArePrimitivesValid() const {
    return assembled_primitive_type != assembled_primitive_type::kInvalid;
  }
};

// Whether to discard samples after coarse or fine rasterization - specifically,
// after the viz query, but before pixel shading.
// In hardware-accelerated emulation, the distinction between hi-Z / hi-stencil
// and full RB depth / stencil testing may not be made, so SC viz queries with
// all pixels being killed by SC may need to be emulated by using the regular
// depth / stencil testing purely to see if it passes or fails, while disabling
// all observable post-SC effects (such as pixel shader memory exports, depth /
// stencil writes, occlusion query sample count incrementing in RB, color
// writes).
inline bool AreAllPixelsKilledPostRasterization(
    const reg::PA_SC_VIZ_QUERY pa_sc_viz_query) {
  return pa_sc_viz_query.kill_pix_post_hi_z ||
         pa_sc_viz_query.kill_pix_post_detail_mask;
}

struct SurfaceState {
  // Invalid guest configuration will fall back to kNoOperation (to still
  // execute the vertex shader, in case it has something like memory export).
  xenos::EdramMode edram_mode = xenos::EdramMode::kNoOperation;

  uint_fast16_t surface_pitch = 0;

  xenos::MsaaSamples msaa_samples = xenos::MsaaSamples::k1X;

  explicit SurfaceState() = default;

  explicit SurfaceState(const RegisterFile& regs,
                        bool all_primitives_killed_or_culled,
                        bool allow_zero_surface_pitch);
};

struct DepthStencilState {
  // Disabled or identity depth / stencil test is normalized to both at once, so
  // depth and stencil state has correct values even if `z_enable` and / or
  // `stencil_enable` is zero.

  // If only one of the two faces is rasterized, the stencil state for the
  // rasterized face will be used for the unused face too. This simplifies
  // targeting host GPU APIs that don't support per-face state (they can
  // unconditionally use the state for the front face).

  reg::RB_DEPTHCONTROL depth_control;

  reg::RB_STENCILREFMASK stencil_ref_mask_front;
  reg::RB_STENCILREFMASK stencil_ref_mask_back;

  // Initialize to depth and stencil disabled, with individual depth / stencil
  // controls being identity.
  explicit DepthStencilState();

  explicit DepthStencilState(const RegisterFile& regs,
                             const PrimitiveState& primitive_state,
                             xenos::EdramMode edram_mode);

  void DepthStencilEnabled() const;

 private:
  // Normalizes the stencil comparison function, operations and masks for one of
  // the faces in the provided RB_DEPTHCONTROL - specifically, for the one
  // configured via its front face fields. Other bits of
  // `depth_control_with_stencil_func_ops` are not read and not modified.
  static void NormalizeFaceStencil(
      reg::RB_DEPTHCONTROL& depth_control_with_stencil_func_ops,
      reg::RB_STENCILREFMASK& stencil_ref_mask, xenos::CompareFunction z_func);
};

struct ColorWriteState {
  // Component write mask for each target. Zero if the target is not written to,
  // otherwise components that don't exist in the format will be set to 1 (so
  // that a != 0b1111 comparison can be used to check if write masking is needed
  // at all).
  uint_fast16_t component_masks;

  // For disabled render targets and components, will be set to identity.
  std::array<reg::RB_BLENDCONTROL, xenos::kMaxColorRenderTargets>
      blend_controls;

  // Initialize to all color targets disabled and blending to indentity.
  explicit ColorWriteState();

  explicit ColorWriteState(const RegisterFile& regs,
                           xenos::EdramMode edram_mode,
                           uint_fast8_t pixel_shader_color_exports);

  uint_fast8_t GetTargetComponentMask(unsigned target_index) const;

  bool IsBlendFactorConstantUsed() const;

 private:
  static xenos::BlendFactor NormalizeColorBlendFactor(xenos::BlendFactor factor,
                                                      bool format_has_alpha);
  static xenos::BlendFactor NormalizeAlphaBlendFactor(
      xenos::BlendFactor factor);
};

}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_PIPELINE_STATE_H_
