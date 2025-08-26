/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/d3d12/d3d12_pipeline_state.h"

#include <cstring>

#include "xenia/base/assert.h"
#include "xenia/base/math.h"
#include "xenia/gpu/registers.h"

namespace xe {
namespace gpu {
namespace d3d12 {

D3D12_COMPARISON_FUNC D3D12SerializedPipelineState::ComparisonFuncToD3D12(
    const xenos::CompareFunction func) {
  return D3D12_COMPARISON_FUNC(UINT(D3D12_COMPARISON_FUNC_NEVER) + UINT(func));
}

D3D12_STENCIL_OP D3D12SerializedPipelineState::StencilFace::OpToD3D12(
    const xenos::StencilOp op) {
  return D3D12_STENCIL_OP(UINT(D3D12_STENCIL_OP_KEEP) + UINT(op));
}

void D3D12SerializedPipelineState::StencilFace::ToD3D12(
    D3D12_DEPTH_STENCILOP_DESC& stencil_op_desc) const {
  stencil_op_desc.StencilFailOp = OpToD3D12(fail_op);
  stencil_op_desc.StencilDepthFailOp = OpToD3D12(depth_fail_op);
  stencil_op_desc.StencilPassOp = OpToD3D12(pass_op);
  stencil_op_desc.StencilFunc = ComparisonFuncToD3D12(func);
}

void D3D12SerializedPipelineState::StencilFace::ToD3D12(
    D3D12_DEPTH_STENCILOP_DESC1& stencil_op_desc) const {
  stencil_op_desc.StencilFailOp = OpToD3D12(fail_op);
  stencil_op_desc.StencilDepthFailOp = OpToD3D12(depth_fail_op);
  stencil_op_desc.StencilPassOp = OpToD3D12(pass_op);
  stencil_op_desc.StencilFunc = ComparisonFuncToD3D12(func);
  stencil_op_desc.StencilReadMask = read_mask;
  stencil_op_desc.StencilWriteMask = write_mask;
}

const D3D12SerializedPipelineState::ColorTarget::BlendFactor
    D3D12SerializedPipelineState::ColorTarget::kBlendFactorFromXenos[1 << 5] = {
        // See the xenos::BlendFactor bit layout.
        // Indexing with `| 0b10` is explicitly supported for obtaining the
        // factor for alpha regardless of whether it's a color or an alpha one
        // on the Xenos for simplicity because Direct3D 12 doesn't allow color
        // factors for alpha (except for D3D12_BLEND_[INV_]BLEND_FACTOR, because
        // the separate ALPHA_FACTOR was added after the initial release of
        // Direct3D 12).

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kZero,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kOne,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kZero,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kOne,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcColor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kInvSrcColor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlpha,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kInvSrcAlpha,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kDestColor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kInvDestColor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kDestAlpha,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kInvDestAlpha,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kBlendFactor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kInvBlendFactor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kAlphaFactor,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kInvAlphaFactor,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,

        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
        D3D12SerializedPipelineState::ColorTarget::BlendFactor::kSrcAlphaSat,
};

D3D12_BLEND D3D12SerializedPipelineState::ColorTarget::BlendFactorToD3D12(
    const BlendFactor blend_factor) {
  switch (blend_factor) {
    case BlendFactor::kZero:
      return D3D12_BLEND_ZERO;
    case BlendFactor::kOne:
      return D3D12_BLEND_ONE;
    case BlendFactor::kSrcColor:
      return D3D12_BLEND_SRC_COLOR;
    case BlendFactor::kInvSrcColor:
      return D3D12_BLEND_INV_SRC_COLOR;
    case BlendFactor::kSrcAlpha:
      return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::kInvSrcAlpha:
      return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::kDestAlpha:
      return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::kInvDestAlpha:
      return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendFactor::kDestColor:
      return D3D12_BLEND_DEST_COLOR;
    case BlendFactor::kInvDestColor:
      return D3D12_BLEND_INV_DEST_COLOR;
    case BlendFactor::kSrcAlphaSat:
      return D3D12_BLEND_SRC_ALPHA_SAT;
    case BlendFactor::kBlendFactor:
      return D3D12_BLEND_BLEND_FACTOR;
    case BlendFactor::kInvBlendFactor:
      return D3D12_BLEND_INV_BLEND_FACTOR;
    case BlendFactor::kAlphaFactor:
      return D3D12_BLEND_ALPHA_FACTOR;
    case BlendFactor::kInvAlphaFactor:
      return D3D12_BLEND_INV_ALPHA_FACTOR;
    default:
      assert_unhandled_case(blend_factor);
      return D3D12_BLEND_ZERO;
  }
}

const D3D12SerializedPipelineState::ColorTarget::BlendOp
    D3D12SerializedPipelineState::ColorTarget::kBlendOpFromXenos[1 << 3] = {
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kAdd,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kSubtract,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kMin,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kMax,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kRevSubtract,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kAdd,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kAdd,
        D3D12SerializedPipelineState::ColorTarget::BlendOp::kAdd,
};

D3D12_BLEND_OP D3D12SerializedPipelineState::ColorTarget::BlendOpToD3D12(
    const BlendOp blend_op) {
  switch (blend_op) {
    case BlendOp::kAdd:
      return D3D12_BLEND_OP_ADD;
    case BlendOp::kSubtract:
      return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::kRevSubtract:
      return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::kMin:
      return D3D12_BLEND_OP_MIN;
    case BlendOp::kMax:
      return D3D12_BLEND_OP_MAX;
    default:
      assert_unhandled_case(blend_op);
      return D3D12_BLEND_OP_ADD;
  }
}

void D3D12SerializedPipelineState::ColorTarget::BlendToD3D12(
    D3D12_RENDER_TARGET_BLEND_DESC& blend_desc_out) const {
  if (write_mask & 0b0111) {
    blend_desc_out.SrcBlend = BlendFactorToD3D12(src_blend_color);
    blend_desc_out.DestBlend = BlendFactorToD3D12(dest_blend_color);
    blend_desc_out.BlendOp = BlendOpToD3D12(blend_op_color);
  } else {
    blend_desc_out.SrcBlend = D3D12_BLEND_ONE;
    blend_desc_out.DestBlend = D3D12_BLEND_ZERO;
    blend_desc_out.BlendOp = D3D12_BLEND_OP_ADD;
  }
  if (write_mask & 0b1000) {
    blend_desc_out.SrcBlendAlpha = BlendFactorToD3D12(src_blend_alpha);
    blend_desc_out.DestBlendAlpha = BlendFactorToD3D12(dest_blend_alpha);
    blend_desc_out.BlendOpAlpha = BlendOpToD3D12(blend_op_alpha);
  } else {
    blend_desc_out.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend_desc_out.DestBlendAlpha = D3D12_BLEND_ZERO;
    blend_desc_out.BlendOpAlpha = D3D12_BLEND_OP_ADD;
  }
  blend_desc_out.BlendEnable =
      blend_desc_out.SrcBlend != D3D12_BLEND_ONE ||
      blend_desc_out.DestBlend != D3D12_BLEND_ZERO ||
      blend_desc_out.BlendOp != D3D12_BLEND_OP_ADD ||
      blend_desc_out.SrcBlendAlpha != D3D12_BLEND_ONE ||
      blend_desc_out.DestBlendAlpha != D3D12_BLEND_ZERO ||
      blend_desc_out.BlendOpAlpha != D3D12_BLEND_OP_ADD;
  blend_desc_out.LogicOpEnable = FALSE;
  blend_desc_out.LogicOp = D3D12_LOGIC_OP_COPY;
  blend_desc_out.RenderTargetWriteMask = write_mask;
}

D3D12SerializedPipelineState::D3D12SerializedPipelineState() {
  std::memset(this, 0, sizeof(*this));
}

bool D3D12SerializedPipelineState::IsValid() const {
  if (hull_shader != hw::HostHullShader::kNone) {
    if (hull_shader >= hw::HostHullShader::kCount) {
      return false;
    }
    if (geometry_shader != hw::HostGeometryShader::kNone) {
      return false;
    }
  } else {
    if (geometry_shader >= hw::HostGeometryShader::kCount) {
      return false;
    }
  }

  if (cull_front && cull_back) {
    return false;
  }

  if (msaa_samples >= hw::HostMSAASamples::kCount) {
    return false;
  }

  if (primitive_topology_type >= PrimitiveTopologyType::kCount) {
    return false;
  }

  if (depth_stencil_view_format >= DepthStencilView::kCount) {
    return false;
  }

  for (unsigned color_target_index = 0;
       color_target_index < xenos::kMaxColorRenderTargets;
       ++color_target_index) {
    const ColorTarget& color_target = color_targets[color_target_index];
    if (!color_target.enable) {
      if (color_target.write_mask) {
        return false;
      }
      continue;
    }
    if (!xenos::kColorRenderTargetFormatComponentMask[unsigned(
            color_target.format)]) {
      return false;
    }
    if (color_target.write_mask & 0b0111) {
      if (color_target.src_blend_color >= ColorTarget::BlendFactor::kCount ||
          color_target.dest_blend_color >= ColorTarget::BlendFactor::kCount ||
          color_target.blend_op_color >= ColorTarget::BlendOp::kCount) {
        return false;
      }
    }
    if (color_target.write_mask & 0b1000) {
      if (color_target.src_blend_alpha >= ColorTarget::BlendFactor::kCount ||
          ColorTarget::IsColorOnlyBlendFactor(color_target.src_blend_alpha) ||
          color_target.dest_blend_alpha >= ColorTarget::BlendFactor::kCount ||
          ColorTarget::IsColorOnlyBlendFactor(color_target.dest_blend_alpha) ||
          color_target.blend_op_alpha >= ColorTarget::BlendOp::kCount) {
        return false;
      }
    }
  }

  return true;
}

bool D3D12SerializedPipelineState::IsUsableWithConfig(
    const hw::HostConfig& host_config) const {
  if (msaa_samples != hw::GetHostMSAASamples(
                          hw::HostMSAASamplesToGuest(msaa_samples),
                          !TargetIndependentRasterizationUsed(), host_config)) {
    return false;
  }

  if (stencil_enable && !host_config.RBHLESeparateStencilRefMaskUsed() &&
      (stencil_front_face.read_mask != stencil_back_face.read_mask ||
       stencil_front_face.write_mask != stencil_back_face.write_mask)) {
    return false;
  }

  if (!host_config.RBHLEConstantAlphaColorBlendFactorUsed()) {
    for (unsigned color_target_index = 0;
         color_target_index < xenos::kMaxColorRenderTargets;
         ++color_target_index) {
      const ColorTarget& color_target = color_targets[color_target_index];
      if (!color_target.enable) {
        continue;
      }
      if (color_target.src_blend_color ==
              ColorTarget::BlendFactor::kAlphaFactor ||
          color_target.src_blend_color ==
              ColorTarget::BlendFactor::kInvAlphaFactor ||
          color_target.dest_blend_color ==
              ColorTarget::BlendFactor::kAlphaFactor ||
          color_target.dest_blend_color ==
              ColorTarget::BlendFactor::kInvAlphaFactor) {
        return false;
      }
    }
  }

  return true;
}

bool D3D12SerializedPipelineState::TargetIndependentRasterizationUsed() const {
  if (depth_stencil_view_format != DepthStencilView::kNone) {
    return false;
  }
  for (unsigned color_target_index = 0;
       color_target_index < xenos::kMaxColorRenderTargets;
       ++color_target_index) {
    if (color_targets[color_target_index].enable) {
      return false;
    }
  }
  return true;
}

void D3D12SerializedPipelineState::RasterizerAndSampleDescToD3D12(
    D3D12_RASTERIZER_DESC& rasterizer_desc_out,
    DXGI_SAMPLE_DESC& sample_desc_out) const {
  rasterizer_desc_out.FillMode =
      fill_mode_wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
  if (cull_back) {
    assert_false(cull_front);
    rasterizer_desc_out.CullMode = D3D12_CULL_MODE_BACK;
  } else if (cull_front) {
    rasterizer_desc_out.CullMode = D3D12_CULL_MODE_FRONT;
  } else {
    rasterizer_desc_out.CullMode = D3D12_CULL_MODE_NONE;
  }
  rasterizer_desc_out.FrontCounterClockwise = front_counter_clockwise;
  rasterizer_desc_out.DepthBias = depth_bias;
  rasterizer_desc_out.DepthBiasClamp = 0.0f;
  rasterizer_desc_out.SlopeScaledDepthBias = slope_scaled_depth_bias;
  rasterizer_desc_out.DepthClipEnable = depth_clip_enable;
  // TODO(Triang3l): Anti-aliased line rendering (MultisampleEnable and / or
  // AntialiasedLineEnable) with PA_SC_LINE_CNTL::EXPAND_LINE_WIDTH. However, a
  // geometry shader may be needed to support PA_SU_LINE_CNTL::LINE_WIDTH
  // instead.
  rasterizer_desc_out.MultisampleEnable = FALSE;
  rasterizer_desc_out.AntialiasedLineEnable = FALSE;
  rasterizer_desc_out.ConservativeRaster =
      D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
  sample_desc_out.Quality = 1;
  // 2 is not a valid ForcedSampleCount in Direct3D 12, target-independent
  // rasterization with host 2x MSAA can be supported only via the
  // SupportedSampleCountsWithNoOutputs functionality.
  if (TargetIndependentRasterizationUsed() &&
      msaa_samples != hw::HostMSAASamples::k2xGuest2xHost) {
    rasterizer_desc_out.ForcedSampleCount =
        msaa_samples == hw::HostMSAASamples::k1x ? 1 : 4;
    sample_desc_out.Count = 1;
  } else {
    rasterizer_desc_out.ForcedSampleCount = 0;
    switch (msaa_samples) {
      case hw::HostMSAASamples::k1x:
        sample_desc_out.Count = 1;
        break;
      case hw::HostMSAASamples::k2xGuest2xHost:
        sample_desc_out.Count = 2;
        break;
      case hw::HostMSAASamples::k2xGuest4xHost:
      case hw::HostMSAASamples::k4x:
        sample_desc_out.Count = 4;
        break;
      default:
        assert_unhandled_case(msaa_samples);
        sample_desc_out.Count = 1;
        break;
    }
  }
}

UINT D3D12SerializedPipelineState::SampleMaskToD3D12(
    const hw::HostMSAASamples msaa_samples) {
  if (msaa_samples == hw::HostMSAASamples::k2xGuest4xHost) {
    return hw::kGuestToHostMSAASampleMask2xTo4x;
  }
  // TODO(Triang3l): Emulate PA_SC_AA_MASK.
  return ~UINT(0);
}

DXGI_FORMAT D3D12SerializedPipelineState::DepthStencilViewToD3D12(
    const DepthStencilView depth_stencil_view_format) {
  switch (depth_stencil_view_format) {
    case DepthStencilView::kNone:
      return DXGI_FORMAT_UNKNOWN;
    case DepthStencilView::kD24UNormS8UInt:
      return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case DepthStencilView::kD32FloatS8X24UInt:
      return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    default:
      assert_unhandled_case(depth_stencil_view_format);
      return DXGI_FORMAT_UNKNOWN;
  }
}

bool D3D12SerializedPipelineState::IsDepthTestEnabled() const {
  return depth_write_mask || depth_func != xenos::CompareFunction::kAlways;
}

void D3D12SerializedPipelineState::DepthStencilToD3D12(
    D3D12_DEPTH_STENCIL_DESC& depth_stencil_desc_out) const {
  depth_stencil_desc_out.DepthEnable = IsDepthTestEnabled();
  depth_stencil_desc_out.DepthWriteMask = depth_write_mask
                                              ? D3D12_DEPTH_WRITE_MASK_ALL
                                              : D3D12_DEPTH_WRITE_MASK_ZERO;
  depth_stencil_desc_out.DepthFunc = ComparisonFuncToD3D12(depth_func);
  depth_stencil_desc_out.StencilEnable = stencil_enable;
  depth_stencil_desc_out.StencilReadMask = stencil_front_face.read_mask;
  depth_stencil_desc_out.StencilWriteMask = stencil_front_face.write_mask;
  stencil_front_face.ToD3D12(depth_stencil_desc_out.FrontFace);
  stencil_back_face.ToD3D12(depth_stencil_desc_out.BackFace);
}

void D3D12SerializedPipelineState::DepthStencilToD3D12(
    D3D12_DEPTH_STENCIL_DESC2& depth_stencil_desc_out) const {
  depth_stencil_desc_out.DepthEnable = IsDepthTestEnabled();
  depth_stencil_desc_out.DepthWriteMask = depth_write_mask
                                              ? D3D12_DEPTH_WRITE_MASK_ALL
                                              : D3D12_DEPTH_WRITE_MASK_ZERO;
  depth_stencil_desc_out.DepthFunc = ComparisonFuncToD3D12(depth_func);
  depth_stencil_desc_out.StencilEnable = stencil_enable;
  stencil_front_face.ToD3D12(depth_stencil_desc_out.FrontFace);
  stencil_back_face.ToD3D12(depth_stencil_desc_out.BackFace);
  depth_stencil_desc_out.DepthBoundsTestEnable = FALSE;
}

void D3D12SerializedPipelineState::Get(
    D3D12SerializedPipelineState& pipeline_state_out, const RegisterFile& regs,
    const uint64_t vertex_shader_hash,
    const uint64_t vertex_shader_modification, const uint64_t pixel_shader_hash,
    const uint64_t pixel_shader_modification,
    const PrimitiveState& primitive_state, const SurfaceState& surface_state,
    const DepthStencilState& depth_stencil_state,
    const ColorWriteState& color_write_state,
    const hw::HostRenderTargetBindingTracker::Bindings& render_target_bindings,
    const hw::HostConfig& host_config) {
  std::memset(&pipeline_state_out, 0, sizeof(pipeline_state_out));

  pipeline_state_out.vertex_shader_hash = vertex_shader_hash;
  pipeline_state_out.vertex_shader_modification = vertex_shader_modification;
  pipeline_state_out.pixel_shader_hash = pixel_shader_hash;
  pipeline_state_out.pixel_shader_modification = pixel_shader_modification;

  pipeline_state_out.host_hull_shader = hw::GetHostHullShader(
      primitive_state.tessellation_domain, primitive_state.tessellation_mode);

  // TODO(Triang3l): Host geometry shader.

  // Point fill mode is not supported by Direct3D 12, use the line fill mode, as
  // the developer likely intended the primitive not to be filled for debugging
  // purposes.
  pipeline_state_out.fill_mode_wireframe =
      primitive_state.polygon_type_front != xenos::PolygonMode::kTriangles ||
      primitive_state.polygon_type_back != xenos::PolygonMode::kTriangles;

  // Culling both faces is not supported by Direct3D 12, needs to be handled
  // externally.
  if (primitive_state.cull_back != pipeline_state_out.cull_front) {
    (pipeline_state_out.cull_back ? pipeline_state_out.cull_back
                                  : pipeline_state_out.cull_front) = true;
  }
  pipeline_state_out.front_counter_clockwise = !primitive_state.front_clockwise;

  pipeline_state_out.depth_clip_enable =
      !regs.Get<reg::PA_CL_CLIP_CNTL>().clip_disable;

  msaa_samples =
      hw::GetHostMSAASamples(surface_state.msaa_samples,
                             render_target_bindings.AnyBound(), host_config);

  switch (primitive_state.assembled_primitive_state) {
    case PrimitiveState::AssembledPrimitiveType::kPoint:
      pipeline_state_out.primitive_topology_type =
          PrimitiveTopologyType::kPoint;
      break;
    case PrimitiveState::AssembledPrimitiveType::kLine:
      pipeline_state_out.primitive_topology_type = PrimitiveTopologyType::kLine;
      break;
    default:
      pipeline_state_out.primitive_topology_type =
          PrimitiveTopologyType::kTriangle;
      break;
  }

  if (render_target_bindings.depth) {
    pipeline_state_out.depth_stencil_view_format =
        render_target_bindings.depth->host_binding_format ==
                xenos::DepthRenderTargetFormat::kD24S8
            ? DepthStencilViewFormat::kD24UNormS8UInt
            : DepthStencilViewFormat::kD32FloatS8X24UInt;

    pipeline_state_out.depth_write_mask =
        depth_stencil_state.depth_control.z_write_enable;
    pipeline_state_out.depth_func = depth_stencil_state.depth_control.zfunc;

    // TODO(Triang3l): Depth bias.

    if (depth_stencil_state.depth_control.stencil_enable) {
      pipeline_state_out.stencil_enable = 1;

      pipeline_state_out.stencil_front_face.read_mask =
          depth_stencil_state.stencil_ref_mask_front.stencilmask;
      pipeline_state_out.stencil_front_face.write_mask =
          depth_stencil_state.stencil_ref_mask_front.stencilwritemask;
      pipeline_state_out.stencil_front_face.fail_op =
          depth_stencil_state.depth_control.stencilfail;
      pipeline_state_out.stencil_front_face.depth_fail_op =
          depth_stencil_state.depth_control.stencilzfail;
      pipeline_state_out.stencil_front_face.pass_op =
          depth_stencil_state.depth_control.stencilpass;
      pipeline_state_out.stencil_front_face.func =
          depth_stencil_state.depth_control.stencilfunc;

      if (host_config.RBHLESeparateStencilRefMaskUsed()) {
        pipeline_state_out.stencil_back_face.read_mask =
            depth_stencil_state.stencil_ref_mask_back.stencilmask;
        pipeline_state_out.stencil_back_face.write_mask =
            depth_stencil_state.stencil_ref_mask_back.stencilwritemask;
        pipeline_state_out.stencil_back_face.fail_op =
            depth_stencil_state.depth_control.stencilfail;
        pipeline_state_out.stencil_back_face.depth_fail_op =
            depth_stencil_state.depth_control.stencilzfail;
        pipeline_state_out.stencil_back_face.pass_op =
            depth_stencil_state.depth_control.stencilpass_bf;
        pipeline_state_out.stencil_back_face.func =
            depth_stencil_state.depth_control.stencilfunc_bf;
      } else {
        pipeline_state_out.stencil_back_face =
            pipeline_state_out.stencil_front_face;
      }
    }
  } else {
    pipeline_state_out.depth_stencil_view_format =
        DepthStencilViewFormat::kNone;
    pipeline_state_out.depth_func = xenos::CompareFunction::kAlways;
  }

  for (unsigned color_target_index = 0;
       color_target_index < xenos::kMaxColorRenderTargets;
       ++color_target_index) {
    if (!render_target_bindings.color[color_target_index]) {
      continue;
    }

    ColorTarget& color_target_out =
        pipeline_state_out.color_targets[color_target_index];

    color_target_out.enable = 1;
    color_target_out.format =
        render_target_bindings.color[color_target_index]->format;

    // TODO(Triang3l): Color target blend state and write mask.
  }
}

}  // namespace d3d12
}  // namespace gpu
}  // namespace xe
