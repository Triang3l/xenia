/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_D3D12_D3D12_PIPELINE_STATE_H_
#define XENIA_GPU_D3D12_D3D12_PIPELINE_STATE_H_

#include <cstdint>
#include <type_traits>

#include "xenia/base/math.h"
#include "xenia/gpu/hw/host_config.h"
#include "xenia/gpu/hw/host_pipeline_state.h"
#include "xenia/gpu/hw/host_render_target_binding_tracker.h"
#include "xenia/gpu/pipeline_state.h"
#include "xenia/gpu/register_file.h"
#include "xenia/gpu/shader.h"
#include "xenia/gpu/xenos.h"
#include "xenia/ui/d3d12/d3d12_api.h"

namespace xe {
namespace gpu {
namespace d3d12 {

#pragma pack(push, 4)
struct D3D12SerializedPipelineState {
  static D3D12_COMPARISON_FUNC ComparisonFuncToD3D12(
      xenos::CompareFunction func);

  // Update the version if changing any fields or used enumerations to/
  // invalidate pipeline state object description storages!
  static constexpr uint32_t kVersion = 0x20250828;  // 0xYYYYMMDD

  uint64_t vertex_shader_hash;
  // 0 if drawing without a pixel shader.
  uint64_t pixel_shader_hash;

  uint64_t vertex_shader_modification;
  // 0 if drawing without a pixel shader.
  uint64_t pixel_shader_modification;

  XE_ENUM_BIT_FIELD(hw::HostHullShader, hull_shader, kCount);

  XE_ENUM_BIT_FIELD(hw::HostGeometryShader, geometry_shader, kCount);

  uint32_t fill_mode_wireframe : 1;
  uint32_t cull_front : 1;
  uint32_t cull_back : 1;
  uint32_t front_counter_clockwise : 1;
  uint32_t depth_clip_enable : 1;

  XE_ENUM_BIT_FIELD(hw::HostMSAASamples, msaa_samples, kCount);

  enum class PrimitiveTopologyType : uint32_t {
    kPoint,
    kLine,
    kTriangle,

    kCount,
  };

  // Ignored by pipeline state object creation if `tessellation_domain` isn't
  // `kNone` (D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH is implied otherwise).
  XE_ENUM_BIT_FIELD(PrimitiveTopologyType, primitive_topology_type, kCount);

  enum class DepthStencilViewFormat : uint32_t {
    kNone = 0,

    kD32FloatS8X24UInt,
    kD24UNormS8UInt,

    kCount,
  };

  // `depth_stencil_view_format != kNone` with disabled depth / stencil testing
  // can be used, for instance, to preserve the DSV binding from preceding
  // draws, or to force disable target-independent rasterization, for instance,
  // to make sure 2x MSAA is used with its sample positions instead of
  // ForcedSampleCount = 4 with 2 samples masked, so the coverage is the same
  // for two draws of identical primitives with and without depth / stencil
  // testing and / or color writing.
  XE_ENUM_BIT_FIELD(DepthStencilView, depth_stencil_view_format, kCount);
  uint32_t depth_write_mask : 1;
  xenos::CompareFunction depth_func : 3;
  uint32_t stencil_enable : 1;

  int32_t depth_bias;
  float slope_scaled_depth_bias;

  // The stencil state is ignored by pipeline state object creation if
  // `stencil_enable` is 0.

  struct StencilFace {
    uint32_t read_mask : 8;
    uint32_t write_mask : 8;
    xenos::StencilOp fail_op : 3;
    xenos::StencilOp depth_fail_op : 3;
    xenos::StencilOp pass_op : 3;
    xenos::CompareFunction func : 3;

    static D3D12_STENCIL_OP OpToD3D12(xenos::StencilOp op);

    void ToD3D12(D3D12_DEPTH_STENCILOP_DESC& stencil_op_desc) const;
    void ToD3D12(D3D12_DEPTH_STENCILOP_DESC1& stencil_op_desc) const;
  };

  StencilFace stencil_front_face;
  StencilFace stencil_back_face;

  struct ColorTarget {
    enum class BlendFactor : uint32_t {
      kZero,
      kOne,
      kSrcColor,
      kInvSrcColor,
      kSrcAlpha,
      kInvSrcAlpha,
      kDestAlpha,
      kInvDestAlpha,
      kDestColor,
      kInvDestColor,
      kSrcAlphaSat,
      kBlendFactor,
      kInvBlendFactor,
      kAlphaFactor,
      kInvAlphaFactor,

      kCount,
    };

    constexpr bool IsColorOnlyBlendFactor(const BlendFactor blend_factor) {
      switch (blend_factor) {
        case BlendFactor::kSrcColor:
        case BlendFactor::kInvSrcColor:
        case BlendFactor::kDestColor:
        case BlendFactor::kInvDestColor:
          return true;
        default:
          return false;
      }
    }

    // Index with `| 0b10` to get the alpha factor.
    static const BlendFactor kBlendFactorFromXenos[1 << 5];

    static D3D12_BLEND BlendFactorToD3D12(BlendFactor blend_factor);

    enum class BlendOp : uint32_t {
      kAdd,
      kSubtract,
      kRevSubtract,
      kMin,
      kMax,

      kCount,
    };

    static const BlendOp kBlendOpFromXenos[1 << 3];

    static D3D12_BLEND_OP BlendOpToD3D12(BlendOp blend_op);

    // `enable` 1 with `write_mask` is 0 can be used, for instance, to preserve
    // RTV bindings from preceding draws.
    uint32_t enable : 1;

    uint32_t format : xenos::kColorRenderTargetFormatBits;

    static_assert(uint32_t(BlendFactor::kCount) <= uint32_t(1) << 4);
    static_assert(uint32_t(BlendOp::kCount) <= uint32_t(1) << 3);
    BlendFactor src_blend_color : 4;
    BlendFactor dest_blend_color : 4;
    BlendOp blend_op_color : 3;
    BlendFactor src_blend_alpha : 4;
    BlendFactor dest_blend_alpha : 4;
    BlendOp blend_op_alpha : 3;

    // Must be 0 if the target is not enabled.
    uint32_t write_mask : 4;

    bool IsBlendEnabled() const;

    void BlendToD3D12(D3D12_RENDER_TARGET_BLEND_DESC& blend_desc_out) const;
  };

  ColorTarget color_targets[xenos::kMaxColorRenderTargets];

  explicit D3D12SerializedPipelineState();

  bool IsValid() const;

  bool IsUsableWithConfig(const hw::HostConfig& host_config) const;

  bool TargetIndependentRasterizationUsed() const;

  void RasterizerAndSampleDescToD3D12(
      D3D12_RASTERIZER_DESC& rasterizer_desc_out,
      DXGI_SAMPLE_DESC& sample_desc_out) const;

  static UINT SampleMaskToD3D12(hw::HostMSAASamples msaa_samples);

  static DXGI_FORMAT DepthStencilViewToD3D12(
      DepthStencilView depth_stencil_view_format);

  bool IsDepthTestEnabled() const;

  void DepthStencilToD3D12(
      D3D12_DEPTH_STENCIL_DESC& depth_stencil_desc_out) const;
  void DepthStencilToD3D12(
      D3D12_DEPTH_STENCIL_DESC2& depth_stencil_desc_out) const;

  static void Get(D3D12SerializedPipelineState& pipeline_state_out);
};
#pragma pack(pop)

static_assert(std::is_trivially_copyable<D3D12SerializedPipelineState>());

}  // namespace d3d12
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_D3D12_D3D12_PIPELINE_STATE_H_
