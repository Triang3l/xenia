/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/pipeline_state.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "xenia/base/assert.h"
#include "xenia/base/logging.h"

namespace xe {
namespace gpu {

ScissorState::ScissorState(const RegisterFile& regs) {
  const auto pa_sc_window_scissor_tl = regs.Get<reg::PA_SC_WINDOW_SCISSOR_TL>();
  const auto pa_sc_window_scissor_br = regs.Get<reg::PA_SC_WINDOW_SCISSOR_BR>();
  int_fast16_t tl_x = pa_sc_window_scissor_tl.tl_x;
  int_fast16_t tl_y = pa_sc_window_scissor_tl.tl_y;
  int_fast16_t br_x = pa_sc_window_scissor_br.br_x;
  int_fast16_t br_y = pa_sc_window_scissor_br.br_y;

  if (!pa_sc_window_scissor_tl.window_offset_disable) {
    const auto pa_sc_window_offset = regs.Get<reg::PA_SC_WINDOW_OFFSET>();
    tl_x += pa_sc_window_offset.window_x_offset;
    tl_y += pa_sc_window_offset.window_y_offset;
    br_x += pa_sc_window_offset.window_x_offset;
    br_y += pa_sc_window_offset.window_y_offset;
  }

  const auto pa_sc_screen_scissor_tl = regs.Get<reg::PA_SC_SCREEN_SCISSOR_TL>();
  const auto pa_sc_screen_scissor_br = regs.Get<reg::PA_SC_SCREEN_SCISSOR_BR>();
  tl_x = std::max(int_fast16_t(pa_sc_screen_scissor_tl.tl_x), tl_x);
  tl_y = std::max(int_fast16_t(pa_sc_screen_scissor_tl.tl_y), tl_y);
  br_x = std::min(int_fast16_t(pa_sc_screen_scissor_br.br_x), br_x);
  br_y = std::min(int_fast16_t(pa_sc_screen_scissor_br.br_y), br_y);

  // Make non-negative because host GPU APIs may be requiring that, and clamp to
  // the guest rasterizer fixed-point coordinate range.
  tl_x = std::min(int_fast16_t(xenos::kTexture2DCubeMaxWidthHeight),
                  std::max(int_fast16_t(0), tl_x));
  tl_y = std::min(int_fast16_t(xenos::kTexture2DCubeMaxWidthHeight),
                  std::max(int_fast16_t(0), tl_y));
  br_x = std::min(int_fast16_t(xenos::kTexture2DCubeMaxWidthHeight),
                  std::max(tl_x, br_x));
  br_y = std::min(int_fast16_t(xenos::kTexture2DCubeMaxWidthHeight),
                  std::max(tl_y, br_y));

  // Place an empty scissor rectangle at {0, 0} so that code using the
  // bottom-right extent for potentially covered region estimation can trivially
  // get the smallest value for any empty scissor rectangle.
  if (tl_x >= br_x || tl_y >= br_y) {
    tl_x = 0;
    tl_y = 0;
    br_x = 0;
    br_y = 0;
  }

  tl[0] = uint_fast16_t(tl_x);
  tl[1] = uint_fast16_t(tl_y);
  br[0] = uint_fast16_t(br_x);
  br[1] = uint_fast16_t(br_y);
}

bool ScissorState::IsEmpty() const { return tl[0] >= br[0] || tl[1] >= br[1]; }

PrimitiveState::PrimitiveState(const RegisterFile& regs) {
  const auto vgt_draw_initiator = regs.Get<reg::VGT_DRAW_INITIATOR>();

  primitive_type = vgt_draw_initiator.prim_type;

  // See "THIS REGISTER IS IGNORED IN MAJOR MODE 0 FOR PRIM TYPES 0 THRU 21 !!"
  // in the R6xx/R7xx 3D Register Reference Guide.
  // On the R6xx, 21 is POLYGON, and 22 is 2D_COPY_RECT_LIST_V0.
  // On the Xenos, 15 is POLYGON, and 16 is 2D_COPY_RECT_LIST_V0 (when
  // tessellation is disabled).
  major_mode_explicit =
      uint32_t(primitive_type) > 15 ||
      vgt_draw_initiator.major_mode == xenos::MajorMode::kExplicit;

  const bool tessellation_enabled =
      major_mode_explicit &&
      regs.Get<reg::VGT_OUTPUT_PATH_CNTL>().path_select ==
          xenos::VGTOutputPath::kTessellationEnable;

  assembled_primitive_type = AssembledPrimitiveType::kInvalid;
  if (tessellation_enabled) {
    switch (primitive_type) {
      case xenos::PrimitiveType::kLinePatch:
        assembled_primitive_type = AssembledPrimitiveType::kLine;
        break;
      case xenos::PrimitiveType::kTrianglePatch:
      case xenos::PrimitiveType::kQuadPatch:
        assembled_primitive_type = AssembledPrimitiveType::kTriangleTwoFaced;
        break;
      default:
        break;
    }
  } else {
    switch (primitive_type) {
      case xenos::PrimitiveType::k2DCopyRectListV0:
      case xenos::PrimitiveType::k2DCopyRectListV1:
      case xenos::PrimitiveType::k2DCopyRectListV2:
      case xenos::PrimitiveType::k2DCopyRectListV3:
      case xenos::PrimitiveType::k2DFillRectList:
        assembled_primitive_type = AssembledPrimitiveType::kRectangle;
        break;
      case xenos::PrimitiveType::k2DLineStrip:
        assembled_primitive_type = AssembledPrimitiveType::kLine;
        break;
      case xenos::PrimitiveType::k2DTriStrip:
        assembled_primitive_type = AssembledPrimitiveType::kTriangleTwoFaced;
        break;
      default:
        break;
    }
  }
  if (assembled_primitive_type == AssembledPrimitiveType::kInvalid) {
    switch (primitive_type) {
      case xenos::PrimitiveType::kPointList:
        assembled_primitive_type = AssembledPrimitiveType::kPoint;
        break;
      case xenos::PrimitiveType::kLineList:
      case xenos::PrimitiveType::kLineStrip:
      case xenos::PrimitiveType::kLineLoop:
        assembled_primitive_type = AssembledPrimitiveType::kLine;
        break;
      case xenos::PrimitiveType::kTriangleList:
      case xenos::PrimitiveType::kTriangleFan:
      case xenos::PrimitiveType::kTriangleStrip:
      case xenos::PrimitiveType::kTriangleListWithWFlags:
      case xenos::PrimitiveType::kQuadList:
      case xenos::PrimitiveType::kQuadStrip:
      case xenos::PrimitiveType::kPolygon:
        assembled_primitive_type = AssembledPrimitiveType::kTriangleTwoFaced;
        break;
      case xenos::PrimitiveType::kRectangleList:
        assembled_primitive_type = AssembledPrimitiveType::kRectangle;
        break;
      default:
        break;
    }
  }

  tessellation_domain = TessellationDomain::kNone;
  // Fall back to continuous if invalid, it's good as the default mode for
  // simplicity because it's directly supported by Direct3D 11 tessellation.
  tessellation_mode = xenos::TessellationMode::kContinuous;
  if (tessellation_enabled) {
    const bool is_patch_primitive_type =
        primitive_type >= xenos::PrimitiveType::kLinePatch;

    switch (assembled_primitive_type) {
      case AssembledPrimitiveType::kLine:
        tessellation_domain = is_patch_primitive_type
                                  ? TessellationDomain::kLinePatch
                                  : TessellationDomain::kLinePrimitive;
        break;
      case AssembledPrimitiveType::kTriangleTwoFaced:
        if (primitive_type == xenos::PrimitiveType::kQuadList ||
            primitive_type == xenos::PrimitiveType::kQuadStrip) {
          tessellation_domain = is_patch_primitive_type
                                    ? TessellationDomain::kQuadPatch
                                    : TessellationDomain::kQuadPrimitive;
        } else if (primitive_type == xenos::PrimitiveType::kPolygon) {
          assembled_primitive_type = AssembledPrimitiveType::kInvalid;
        } else {
          tessellation_domain = is_patch_primitive_type
                                    ? TessellationDomain::kTrianglePatch
                                    : TessellationDomain::kTrianglePrimitive;
        }
        break;
      default:
        // Primitive type not supporting tessellation, make invalid.
        assembled_primitive_type = AssembledPrimitiveType::kInvalid;
        break;
    }

    // See the Radeon R6xx/R7xx 3D Register Reference Guide for the explanation
    // of the mode and the level registers.

    const xenos::TessellationMode guest_tessellation_mode =
        regs.Get<reg::VGT_HOS_CNTL>().tess_mode;
    if (is_patch_primitive_type) {
      if (guest_tessellation_mode > xenos::TessellationMode::kAdaptive) {
        XELOGW(
            "Invalid tessellation mode {} used, report the game to Xenia "
            "developers",
            unsigned(guest_tessellation_mode));
      } else {
        tessellation_mode = guest_tessellation_mode;
      }
    } else {
      // Adaptive tessellation is not supported, the index buffer contains the
      // vertex indices, not the tessellation factors. However, continuous
      // behaves like adaptive with all factors being VGT_HOS_MAX_TESS_LEVEL.
      tessellation_mode =
          guest_tessellation_mode == xenos::TessellationMode::kDiscrete
              ? xenos::TessellationMode::kDiscrete
              : xenos::TessellationMode::kContinuous;
    }

    tessellation_max_level = std::min(
        tessellation_mode == xenos::TessellationMode::kDiscrete ? 14.0f : 15.0f,
        std::fmax(1.0f, regs.Get<float>(XE_GPU_REG_VGT_HOS_MAX_TESS_LEVEL)));
    if (tessellation_mode == xenos::TessellationMode::kAdaptive) {
      tessellation_min_level = std::min(
          tessellation_max_level,
          std::fmax(1.0f, regs.Get<float>(XE_GPU_REG_VGT_HOS_MIN_TESS_LEVEL)));
    } else {
      // Constant level from the VGT_HOS_MAX_TESS_LEVEL register for the
      // non-adaptive modes.
      tessellation_min_level = tessellation_max_level;
    }
  } else {
    tessellation_max_level = 1.0f;
    tessellation_min_level = 1.0f;
  }

  const auto pa_su_sc_mode_cntl = regs.Get<reg::PA_SU_SC_MODE_CNTL>();

  if (assembled_primitive_type == AssembledPrimitiveType::kTriangleTwoFaced) {
    cull_front = pa_su_sc_mode_cntl.cull_front;
    cull_back = pa_su_sc_mode_cntl.cull_back;

    front_clockwise = pa_su_sc_mode_cntl.face;

    // 4541096E uses poly_mode 2 for triangles, which is "reserved" on R6xx and
    // not defined on Adreno 2xx, but polymode_front/back_ptype are 0 (points)
    // in this case in 4541096E, which should not be respected for non-kDualMode
    // as the title wants to draw filled triangles.
    if (pa_su_sc_mode_cntl.poly_mode == xenos::PolygonModeEnable::kDualMode) {
      // Fall back to triangles for all invalid polygon types for safety.
      polygon_type_front = std::min(pa_su_sc_mode_cntl.polymode_front_ptype,
                                    xenos::PolygonType::kTriangles);
      polygon_type_back = std::min(pa_su_sc_mode_cntl.polymode_back_ptype,
                                   xenos::PolygonType::kTriangles);
    } else {
      polygon_type_front = xenos::PolygonType::kTriangles;
      polygon_type_back = xenos::PolygonType::kTriangles;
    }

    if (pa_su_sc_mode_cntl.poly_offset_front_enable) {
      polygon_offset_front_subpixel_slope_scale =
          regs.Get<float>(XE_GPU_REG_PA_SU_POLY_OFFSET_FRONT_SCALE);
      polygon_offset_front_offset =
          regs.Get<float>(PA_SU_POLY_OFFSET_FRONT_OFFSET);
    } else {
      polygon_offset_front_subpixel_slope_scale = 0.0f;
      polygon_offset_front_offset = 0.0f;
    }
    if (pa_su_sc_mode_cntl.poly_offset_back_enable) {
      polygon_offset_back_subpixel_slope_scale =
          regs.Get<float>(XE_GPU_REG_PA_SU_POLY_OFFSET_BACK_SCALE);
      polygon_offset_back_offset =
          regs.Get<float>(PA_SU_POLY_OFFSET_BACK_OFFSET);
    } else {
      polygon_offset_back_subpixel_slope_scale = 0.0f;
      polygon_offset_back_offset = 0.0f;
    }

    if (cull_back) {
      // If back faces (or both faces, for consistency) are culled, ignore the
      // back face control and use the front face control for it instead to
      // avoid unnecessary host state changes.
      polygon_type_back = polygon_type_front;
      polygon_offset_back_subpixel_slope_scale =
          polygon_offset_front_subpixel_slope_scale;
      polygon_offset_back_offset = polygon_offset_front_offset;
    } else if (cull_front) {
      polygon_type_front = polygon_type_back;
      polygon_offset_front_subpixel_slope_scale =
          polygon_offset_back_subpixel_slope_scale;
      polygon_offset_front_offset = polygon_offset_back_offset;
    }
  } else {
    // Non-triangle primitives are always front-facing.

    // According to the R6xx/R7xx 3D Register Reference Guide,
    // CULL_FRONT/BACK = 1 is "Cull front/back-facing triangles". Therefore, the
    // culling state applies only to AssembledPrimitiveType::kTriangleTwoFaced
    // primitives (this is also consistent with specifications of PC GPU APIs
    // like OpenGL, Vulkan and Direct3D 11.3).
    cull_front = false;
    cull_back = false;

    // Doesn't matter - set to false for consistency.
    front_clockwise = false;

    // According to the R6xx/R7xx 3D Register Reference Guide,
    // POLYMODE_FRONT/BACK_PTYPE "Specifies how to render front-facing
    // polygons", so it doesn't apply to non-polygon primitives (this is also
    // consistent with PC GPU API specifications).
    switch (assembled_primitive_type) {
      case AssembledPrimitiveType::kPoint:
        polygon_type_front = xenos::PolygonType::kPoints;
        break;
      case AssembledPrimitiveType::kLine:
        polygon_type_front = xenos::PolygonType::kLines;
        break;
      default:
        polygon_type_front = xenos::PolygonType::kTriangles;
        break;
    }
    polygon_type_back = polygon_type_front;

    if (pa_su_sc_mode_cntl.poly_offset_para_enable) {
      polygon_offset_front_subpixel_slope_scale =
          regs.Get<float>(XE_GPU_REG_PA_SU_POLY_OFFSET_FRONT_SCALE);
      polygon_offset_front_offset =
          regs.Get<float>(PA_SU_POLY_OFFSET_FRONT_OFFSET);
    } else {
      polygon_offset_front_subpixel_slope_scale = 0.0f;
      polygon_offset_front_offset = 0.0f;
    }
    polygon_offset_back_subpixel_slope_scale =
        polygon_offset_front_subpixel_slope_scale;
    polygon_offset_back_offset = polygon_offset_front_offset;
  }

  all_primitives_killed_or_culled =
      (cull_front && cull_back) || scissor_is_empty ||
      regs.Get<reg::SQ_PROGRAM_CNTL>().vs_export_mode ==
          xenos::VertexShaderExportMode::kMultipass;
}

SurfaceState::SurfaceState(const RegisterFile& regs,
                           const bool all_primitives_killed_or_culled,
                           const bool allow_zero_surface_pitch) {
  if (all_primitives_killed_or_culled) {
    // Nothing invokes the pixel shader and the render backend.
    return;
  }

  const xenos::EdramMode guest_edram_mode =
      regs.Get<reg::RB_MODECONTROL>().edram_mode;

  if (guest_edram_mode != xenos::EdramMode::kNoOperation &&
      guest_edram_mode != xenos::EdramMode::kColorDepth &&
      guest_edram_mode != xenos::EdramMode::kDepthOnly &&
      guest_edram_mode != xenos::EdramMode::kCopy) {
    XELOGW("Unknown EDRAM mode {} used, report the game to Xenia developers",
           uint32_t(guest_edram_mode));
    return;
  }

  if (guest_edram_mode == xenos::EdramMode::kNoOperation) {
    return;
  }

  const auto rb_surface_info = regs.Get<reg::RB_SURFACE_INFO>();

  if (rb_surface_info.msaa_samples > xenos::EdramSamples::k4X) {
    XELOGW(
        "Invalid MSAA sample count {} used, report the game to Xenia "
        "developers",
        1u << unsigned(rb_surface_info.msaa_samples));
    return;
  }

  if (rb_surface_info.surface_pitch == 0 && !allow_zero_surface_pitch) {
    // TODO(Triang3l): Treat as target-independent rasterization when using host
    // render targets maybe. For drawing without them, no special handling is
    // needed except for skipping the rasterizer ordered view barrier logic.
    // In resolving, zero source pitch is also trivial to handle.
    XELOGW(
        "Zero EDRAM surface pitch is used, which is currently not supported by "
        "Xenia, report the game to Xenia developers");
    return;
  }

  edram_mode = guest_edram_mode;

  // On the host, the same host depth testing may need to be used for emulating
  // both SC hi-Z viz queries and the full RB depth / stencil testing, so if the
  // SC kills all pixels, just disable color writes rather than disabling the RB
  // completely in the emulation logic.
  if (AreAllPixelsKilledPostRasterization(regs.Get<reg::PA_SC_VIZ_QUERY>())) {
    if (edram_mode == xenos::EdramMode::kColorDepth) {
      edram_mode = xenos::EdramMode::kDepthOnly;
    } else if (edram_mode == xenos::EdramMode::kCopy) {
      edram_mode = xenos::EdramMode::kNoOperation;
    }
  }

  surface_pitch = rb_surface_info.surface_pitch;
  msaa_samples = rb_surface_info.msaa_samples;
}

DepthStencilState::DepthStencilState() {
  // Initialize to both Z and stencil tests disabled, with the more granular
  // control normalized to the identity state equivalent to the disabled state -
  // always passing and not writing anything. The "keep" stencil function is 0,
  // so there's no need to set it explicitly.
  depth_control = reg::RB_DEPTHCONTROL{};
  depth_control.zfunc = xenos::CompareFunction::kAlways;
  depth_control.stencilfunc = xenos::CompareFunction::kAlways;
  depth_control.stencilfunc_bf = xenos::CompareFunction::kAlways;

  stencil_ref_mask_front = reg::RB_STENCILREFMASK{};
  stencil_ref_mask_back = reg::RB_STENCILREFMASK{};
}

DepthStencilState::DepthStencilState(const RegisterFile& regs,
                                     const PrimitiveState& primitive_state,
                                     const xenos::EdramMode edram_mode) {
  if (edram_mode != xenos::EdramMode::kColorDepth &&
      edram_mode != xenos::EdramMode::kDepthOnly) {
    depth_control.value = kIdentityDepthControl;
    stencil_ref_mask_front.value = kIdentityStencilRefMask;
    stencil_ref_mask_back.value = kIdentityStencilRefMask;
    return;
  }

  // Initialize RB_DEPTHCONTROL to the guest value, and zero unused bits.
  depth_control = regs.Get<reg::RB_DEPTHCONTROL>();
  depth_control._pad_3 = 0;

  // Don't write anything to the depth / stencil target if drawing purely for a
  // hi-Z / hi-stencil viz query. On the host, the same host depth testing may
  // need to be used for emulating both SC hi-Z / hi-stencil viz queries and the
  // full RB depth / stencil testing.
  // TODO(Triang3l): Should hi-Z viz queries work in
  // xenos::EdramMode::kNoOperation with read-only depth testing actually
  // performed? Does hi-Z even respect RB_DEPTHCONTROL, because if it doesn't,
  // should Xenia even expect RB_DEPTHCONTROL to be valid and up-to-date with
  // pixel killing in SC enabled?
  if (AreAllPixelsKilledPostRasterization(regs.Get<reg::PA_SC_VIZ_QUERY>())) {
    depth_control.z_write_enable = 0;
    depth_control.stencilfail = xenos::StencilOp::kKeep;
    depth_control.stencilzpass = xenos::StencilOp::kKeep;
    depth_control.stencilzfail = xenos::StencilOp::kKeep;
    depth_control.stencilfail_bf = xenos::StencilOp::kKeep;
    depth_control.stencilzpass_bf = xenos::StencilOp::kKeep;
    depth_control.stencilzfail_bf = xenos::StencilOp::kKeep;
  }

  // Normalize disabled and identity depth test control to both disabled and
  // identity at once. Also, disable depth writing for the "equal" depth
  // comparison function because the written depth may only be the same as the
  // destination depth in this case.
  if (depth_control.z_enable) {
    if (depth_control.zfunc == xenos::CompareFunction::kEqual) {
      depth_control.z_write_enable = 0;
    } else if (!depth_control.z_write_enable &&
               depth_control.zfunc == xenos::CompareFunction::kAlways) {
      depth_control.z_enable = 0;
    }
  } else {
    depth_control.z_write_enable = 0;
    depth_control.zfunc = xenos::CompareFunction::kAlways;
  }

  // Normalize the stencil test control.

  if (depth_control.stencil_enable) {
    stencil_ref_mask_front = regs.Get<reg::RB_STENCILREFMASK>();

    if (depth_control.backface_enable) {
      if (primitive_state.assembled_primitive_type !=
              AssembledPrimitiveType::kTriangleTwoFaced ||
          primitive_state.cull_back) {
        // Ignore the back face control if back faces (or both faces, for
        // consistency) are culled, or if the primitive type has only front
        // faces.
        depth_control.backface_enable = 0;
      } else if (primitive_state.cull_front) {
        // Ignore the front face control if front faces are culled.
        depth_control.backface_enable = 0;
        depth_control.stencilfunc = depth_control.stencilfunc_bf;
        depth_control.stencilfail = depth_control.stencilfail_bf;
        depth_control.stencilzpass = depth_control.stencilzpass_bf;
        depth_control.stencilzfail = depth_control.stencilzfail_bf;
        stencil_ref_mask_front =
            regs.Get<reg::RB_STENCILREFMASK>(XE_GPU_REG_RB_STENCILREFMASK_BF);
      }
    }

    NormalizeFaceStencil(depth_control, stencil_ref_mask_front,
                         depth_control.zfunc);

    if (depth_control.backface_enable) {
      reg::RB_DEPTHCONTROL stencil_func_ops_back_as_front{};
      stencil_func_ops_back_as_front.stencilfunc = depth_control.stencilfunc_bf;
      stencil_func_ops_back_as_front.stencilfail = depth_control.stencilfail_bf;
      stencil_func_ops_back_as_front.stencilzpass =
          depth_control.stencilzpass_bf;
      stencil_func_ops_back_as_front.stencilzfail =
          depth_control.stencilzfail_bf;
      NormalizeFaceStencil(stencil_func_ops_back_as_front,
                           stencil_ref_mask_back, depth_control.zfunc);
      depth_control.stencilfunc_bf = stencil_func_ops_back_as_front.stencilfunc;
      depth_control.stencilfail_bf = stencil_func_ops_back_as_front.stencilfail;
      depth_control.stencilzpass_bf =
          stencil_func_ops_back_as_front.stencilzpass;
      depth_control.stencilzfail_bf =
          stencil_func_ops_back_as_front.stencilzfail;
    } else {
      // Replicate the front face stencil control to the back face.
      depth_control.stencilfunc_bf = depth_control.stencilfunc;
      depth_control.stencilfail_bf = depth_control.stencilfail;
      depth_control.stencilzpass_bf = depth_control.stencilzpass;
      depth_control.stencilzfail_bf = depth_control.stencilzfail;
      stencil_ref_mask_back = stencil_ref_mask_front;
    }

    // Disable the stencil masks or the stencil test completely if not needed.
    const bool stencil_is_identity_front =
        depth_control.stencilfunc == xenos::CompareFunction::kAlways &&
        depth_control.stencilfail == xenos::StencilOp::kKeep &&
        depth_control.stencilzpass == xenos::StencilOp::kKeep &&
        depth_control.stencilzfail == xenos::StencilOp::kKeep;
    const bool stencil_is_identity_back =
        depth_control.stencilfunc_bf == xenos::CompareFunction::kAlways &&
        depth_control.stencilfail_bf == xenos::StencilOp::kKeep &&
        depth_control.stencilzpass_bf == xenos::StencilOp::kKeep &&
        depth_control.stencilzfail_bf == xenos::StencilOp::kKeep;
    if (stencil_is_identity_front) {
      stencil_ref_mask_front = RB_STENCILREFMASK{};
    }
    if (stencil_is_identity_back) {
      stencil_ref_mask_back = RB_STENCILREFMASK{};
    }
    if (stencil_is_identity_front && stencil_is_identity_back) {
      depth_control.stencil_enable = 0;
      depth_control.backface_enable = 0;
    } else {
      // Normalize `backface_enable` if not disabling stencil.
      if (depth_control.backface_enable &&
          depth_control.stencilfunc_bf == depth_control.stencilfunc &&
          depth_control.stencilfail_bf == depth_control.stencilfail &&
          depth_control.stencilzpass_bf == depth_control.stencilzpass &&
          depth_control.stencilzfail_bf == depth_control.stencilzfail &&
          stencil_ref_mask_back.stencilref ==
              stencil_ref_mask_front.stencilref &&
          stencil_ref_mask_back.stencilmask ==
              stencil_ref_mask_front.stencilmask &&
          stencil_ref_mask_back.stencilwritemask ==
              stencil_ref_mask_front.stencilwritemask) {
        depth_control.backface_enable = 0;
      }
    }
  } else {
    // Normalize disabled and no-operation stencil.
    depth_control.backface_enable = 0;
    depth_control.stencilfunc = xenos::CompareFunction::kAlways;
    depth_control.stencilfail = xenos::StencilOp::kKeep;
    depth_control.stencilzpass = xenos::StencilOp::kKeep;
    depth_control.stencilzfail = xenos::StencilOp::kKeep;
    depth_control.stencilfunc_bf = xenos::CompareFunction::kAlways;
    depth_control.stencilfail_bf = xenos::StencilOp::kKeep;
    depth_control.stencilzpass_bf = xenos::StencilOp::kKeep;
    depth_control.stencilzfail_bf = xenos::StencilOp::kKeep;
  }
}

void DepthStencilState::DepthStencilEnabled() {
  return depth_control.z_enable || depth_control.stencil_enable;
}

void DepthStencilState::NormalizeFaceStencil(
    reg::RB_DEPTHCONTROL& depth_control_with_stencil_func_ops,
    reg::RB_STENCILREFMASK& stencil_ref_mask,
    const xenos::CompareFunction z_func) {
  // Clear unused bits.
  stencil_ref_mask._pad_24 = 0;

  // Stencil test is `(ref & mask) func (stencil & mask)`.

  // Setting to "always" may contribute towards disabling the stencil test if
  // it ends up being no-operation, also normalization in general may help
  // reduce host pipeline state object permutations.
  if (!stencil_ref_mask.stencilmask) {
    // `0 func 0`: always equal.
    depth_control_with_stencil_func_ops.stencilfunc =
        uint32_t(depth_control_with_stencil_func_ops.stencilfunc) &
                uint32_t(xenos::CompareFunction::kEqual)
            ? xenos::CompareFunction::kAlways
            : xenos::CompareFunction::kNever;
  } else if (!(stencil_ref_mask.stencilref & stencil_ref_mask.stencilmask)) {
    // `0 func (stencil & mask)`.
    if (depth_control_with_stencil_func_ops.stencilfunc ==
        xenos::CompareFunction::kLessEqual) {
      depth_control_with_stencil_func_ops.stencilfunc =
          xenos::CompareFunction::kAlways;
    } else if (depth_control_with_stencil_func_ops.stencilfunc ==
               xenos::CompareFunction::kGreater) {
      depth_control_with_stencil_func_ops.stencilfunc =
          xenos::CompareFunction::kNever;
    }
  }

  if (z_func == xenos::CompareFunction::kAlways) {
    // Depth test never fails.
    depth_control_with_stencil_func_ops.stencilzfail =
        depth_control_with_stencil_func_ops.stencilzpass;
  } else if (z_func == xenos::CompareFunction::kNever) {
    // Depth test never passes.
    depth_control_with_stencil_func_ops.stencilzpass =
        depth_control_with_stencil_func_ops.stencilzfail;
  }

  if (depth_control_with_stencil_func_ops.stencilfunc ==
      xenos::CompareFunction::kAlways) {
    // Stencil test never fails.
    depth_control_with_stencil_func_ops.stencilfail =
        depth_control_with_stencil_func_ops.stencilpass;
    // No comparison needed, provide a stronger hint that the stencil doesn't
    // need to be read.
    stencil_ref_mask.stencilmask = 0;
  } else if (depth_control_with_stencil_func_ops.stencilfunc ==
             xenos::CompareFunction::kNever) {
    // Stencil test never passes.
    depth_control_with_stencil_func_ops.stencilpass =
        depth_control_with_stencil_func_ops.stencilfail;
    // No comparison needed, provide a stronger hint that the stencil doesn't
    // need to be read.
    stencil_ref_mask.stencilmask = 0;
  }

  // Normalize the "keep" operation and a zero stencil write mask.
  if (!stencil_ref_mask.stencilwritemask) {
    // Set all operations to "keep".
    depth_control_with_stencil_func_ops.stencilfail = xenos::StencilOp::kKeep;
    depth_control_with_stencil_func_ops.stencilzpass = xenos::StencilOp::kKeep;
    depth_control_with_stencil_func_ops.stencilzfail = xenos::StencilOp::kKeep;
  } else if (depth_control_with_stencil_func_ops.stencilfail ==
                 xenos::StencilOp::kKeep &&
             depth_control_with_stencil_func_ops.stencilzpass ==
                 xenos::StencilOp::kKeep &&
             depth_control_with_stencil_func_ops.stencilzfail ==
                 xenos::StencilOp::kKeep) {
    stencil_ref_mask.stencilwritemask = 0;
  }

  // Drop unused bits of the reference so they don't cause a host pipeline state
  // change (context roll) if changed alone.
  if (depth_control_with_stencil_func_ops.stencilfail ==
          xenos::CompareFunction::kReplace ||
      depth_control_with_stencil_func_ops.stencilzpass ==
          xenos::CompareFunction::kReplace ||
      depth_control_with_stencil_func_ops.stencilzfail ==
          xenos::CompareFunction::kReplace) {
    stencil_ref_mask.stencilref &=
        stencil_ref_mask.stencilmask | stencil_ref_mask.stencilwritemask;
  } else {
    stencil_ref_mask.stencilref &= stencil_ref_mask.stencilmask;
  }
}

ColorWriteState::ColorWriteState() {
  color_mask = 0b0;

  reg::RB_BLENDCONTROL identity_blend_control{};
  identity_blend_control.color_srcblend = xenos::BlendFactor::kOne;
  identity_blend_control.alpha_srcblend = xenos::BlendFactor::kOne;
  blend_controls.fill(identity_blend_control);
}

ColorWriteState::ColorWriteState(const RegisterFile& regs,
                                 const xenos::EdramMode edram_mode,
                                 const uint_fast8_t pixel_shader_color_exports)
    : ColorWriteState() {
  if (edram_mode != xenos::EdramMode::kColorDepth) {
    return;
  }

  const uint32_t rb_color_mask = regs.Get<uint32_t>(XE_GPU_REG_RB_COLOR_MASK);

  for (unsigned target_index = 0; target_index < xenos::kMaxColorRenderTargets;
       ++target_index) {
    // It appears that if the pixel shader doesn't export a color output, the
    // corresponding render target is not written to. 58410955 clears the top
    // tile with RB_COLOR_MASK enabled for all 4 targets (and all having the
    // same base address in the EDRAM), but with the pixel shader exporting to
    // only to one color target. In 4D5307E6, many surfaces are drawn with
    // RB_COLOR_MASK enabled for 2 targets, but with oC0 or oC1 selected
    // conditionally.
    //
    // This possibly conceptually matches the PC Direct3D 9 behavior:
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx9-graphics-reference-asm-ps-registers-output-color
    // "For ps_3_0, output registers oC# and oD# can be written any number of
    // times. The output of the pixel shader comes from the contents of the
    // output registers at the end of shader execution. If a write to an output
    // register does not happen, perhaps due to flow control or if the shader
    // just did not write it, the corresponding rendertarget is also not
    // updated. If a subset of the channels in an output register are written,
    // then undefined values will be written to the remaining channels."
    // (control flow on the PC means static control flow like bool constants and
    // loops: "For ps_2_x and ps_3_0, you cannot write to oC# and oD# registers
    // within dynamic flow control or predication (writes to oC# inside static
    // flow control is fine).")
    //
    // TODO(Triang3l): Recheck the actual RB_COLOR_MASK for those draws and
    // verify this comment.
    if (!(pixel_shader_color_exports & (uint_fast8_t(1) << target_index))) {
      continue;
    }

    uint_fast8_t target_mask = (rb_color_mask >> (4 * target_index)) & 0b1111;
    if (!target_mask) {
      continue;
    }

    const xenos::ColorRenderTargetFormat target_format =
        regs::Get<reg::RB_COLOR_INFO>(
            reg::RB_COLOR_INFO::rt_register_indices[target_index]);
    const uint_fast8_t target_format_mask =
        xenos::kColorRenderTargetFormatComponentMasks[unsigned(target_format)];
    if (!target_format_mask) {
      XELOGW(
          "Unknown color render target format {} used, report the game to "
          "Xenia developers",
          unsigned(target_format));
      continue;
    }

    // If only components not existing in the format need to be written, disable
    // the target.
    target_mask &= target_format_mask;
    if (!target_mask) {
      continue;
    }

    // Treat missing components as written, so write masking can be disabled if
    // needed without checking the format.
    target_mask |= ~target_format_mask & 0b1111;

    component_masks |= uint_fast16_t(target_mask) << (4 * target_index);

    reg::RB_BLENDCONTROL& target_blend_control = blend_controls[target_index];
    const auto guest_blend_control = regs.Get<reg::RB_BLENDCONTROL>(
        reg::RB_BLENDCONTROL::rt_register_indices[target_index]);

    const bool target_format_has_alpha = (target_format_mask & 0b1000) != 0;

    if (target_mask & 0b0111) {
      if (guest_blend_control.color_comb_fcn > xenos::BlendOp::kRevSubtract) {
        XELOGW(
            "Unknown color blend combination function {} used for target {}, "
            "report the game to Xenia developers",
            uint32_t(guest_blend_control.color_comb_fcn), target_index);
      } else {
        target_blend_control.color_comb_fcn =
            guest_blend_control.color_comb_fcn;
        if (guest_blend_control.color_comb_fcn != xenos::BlendOp::kMin &&
            guest_blend_control.color_comb_fcn != xenos::BlendOp::kMax) {
          target_blend_control.color_srcblend = NormalizeColorBlendFactor(
              guest_blend_control.color_srcblend, target_format_has_alpha);
          target_blend_control.color_destblend = NormalizeColorBlendFactor(
              guest_blend_control.color_destblend, target_format_has_alpha);
        }
      }
    }

    if (target_format_has_alpha && (target_mask & 0b1000)) {
      if (guest_blend_control.alpha_comb_fcn > xenos::BlendOp::kRevSubtract) {
        XELOGW(
            "Unknown alpha blend combination function {} used for target {}, "
            "report the game to Xenia developers",
            uint32_t(guest_blend_control.alpha_comb_fcn), target_index);
      } else {
        target_blend_control.alpha_comb_fcn =
            guest_blend_control.alpha_comb_fcn;
        if (guest_blend_control.alpha_comb_fcn != xenos::BlendOp::kMin &&
            guest_blend_control.alpha_comb_fcn != xenos::BlendOp::kMax) {
          target_blend_control.alpha_srcblend =
              NormalizeAlphaBlendFactor(guest_blend_control.alpha_srcblend);
          target_blend_control.alpha_destblend =
              NormalizeAlphaBlendFactor(guest_blend_control.alpha_destblend);
        }
      }
    }
  }
}

uint_fast8_t ColorWriteState::GetTargetComponentMask(
    const unsigned target_index) const {
  assert_true(target_index < xenos::kMaxColorRenderTargets);
  return (component_masks >> (4 * target_index)) & 0b1111;
}

bool ColorWriteState::IsBlendFactorConstantUsed() const {
  for (const reg::RB_BLENDCONTROL target_blend_control : blend_controls) {
    if (unsigned(target_blend_control.color_srcblend) >> 2 == 0b011 ||
        unsigned(target_blend_control.color_destblend) >> 2 == 0b011 ||
        unsigned(target_blend_control.alpha_srcblend) >> 2 == 0b011 ||
        unsigned(target_blend_control.alpha_destblend) >> 2 == 0b011) {
      return true;
    }
  }
  return false;
}

xenos::BlendFactor ColorWriteState::NormalizeColorBlendFactor(
    const xenos::BlendFactor factor, const bool format_has_alpha) {
  const unsigned factor_bits = unsigned(factor);

  if (factor_bits >> 4) {
    // TODO(Triang3l): Research whether the source alpha exists for the formats
    // without alpha on the Xenos. If yes, the blend factor can be smaller than
    // zero.
    return xenos::BlendFactor::kSrcAlphaSaturate;
  }

  if (!format_has_alpha) {
    if (factor == xenos::BlendFactor::kDstAlpha) {
      return xenos::BlendFactor::kOne;
    }
    if (factor == xenos::BlendFactor::kOneMinusDstAlpha) {
      return xenos::BlendFactor::kZero;
    }
  }

  // Zero and one don't have separate color and alpha factors in host GPU APIs.
  if (factor_bits >> 2 == 0) {
    return factor_bits & 1 ? xenos::BlendFactor::kOne
                           : xenos::BlendFactor::kZero;
  }

  return factor;
}

xenos::BlendFactor ColorWriteState::NormalizeAlphaBlendFactor(
    const xenos::BlendFactor factor) {
  const unsigned factor_bits = unsigned(factor);

  if (factor_bits >> 4) {
    // kSrcAlphaSaturate is (f, f, f, 1).
    return xenos::BlendFactor::kOne;
  }

  // Zero and one don't have separate color and alpha factors in host GPU APIs.
  if (factor_bits >> 2 == 0) {
    return factor_bits & 1 ? xenos::BlendFactor::kOne
                           : xenos::BlendFactor::kZero;
  }

  // Use the AAAA factor for alpha regardless of whether it's set to RGBA or
  // AAAA by the guest to reduce host pipeline state combinations.
  return xenos::BlendFactor(factor_bits | 0b10);
}

}  // namespace gpu
}  // namespace xe
