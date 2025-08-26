/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_HOST_PIPELINE_STATE_H_
#define XENIA_GPU_HW_HOST_PIPELINE_STATE_H_

#include <cstdint>

#include "xenia/gpu/hw/host_config.h"
#include "xenia/gpu/pipeline_state.h"

namespace xe {
namespace gpu {
namespace hw {

// Can be packed in uint32_t and zero-initialized.
// May be used in serialized host pipeline state object descriptions, update
// their versions when changing.
enum class HostHullShader : uint32_t {
  kNone = 0,

  kLinePrimitiveDiscrete,
  kLinePrimitiveContinuous,

  kLinePatchDiscrete,
  kLinePatchContinuous,
  kLinePatchAdaptive,

  kTrianglePrimitiveDiscrete,
  kTrianglePrimitiveContinuous,

  kTrianglePatchDiscrete,
  kTrianglePatchContinuous,
  kTrianglePatchAdaptive,

  kQuadPrimitiveDiscrete,
  kQuadPrimitiveContinuous,

  kQuadPatchDiscrete,
  kQuadPatchContinuous,
  kQuadPatchAdaptive,

  kCount,
};

HostHullShader GetHostHullShader(
    PipelineState::TessellationDomain tessellation_domain,
    xenos::TessellationMode tessellation_mode);

constexpr bool IsTessellationModeValidAndCompatible(
    const PipelineState::TessellationDomain tessellation_domain,
    const xenos::TessellationMode tessellation_mode) {
  switch (tessellation_domain) {
    case PipelineState::TessellationDomain::kLinePrimitive:
    case PipelineState::TessellationDomain::kTrianglePrimitive:
    case PipelineState::TessellationDomain::kQuadPrimitive:
      return tessellation_mode == xenos::TessellationMode::kDiscrete ||
             tessellation_mode == xenos::TessellationMode::kContinuous;
    case PipelineState::TessellationDomain::kLinePatch:
    case PipelineState::TessellationDomain::kTrianglePatch:
    case PipelineState::TessellationDomain::kQuadPatch:
      return tessellation_mode == xenos::TessellationMode::kDiscrete ||
             tessellation_mode == xenos::TessellationMode::kContinuous;
    default:
      return false;
  }
}

// Can be packed in uint32_t and zero-initialized.
// May be used in serialized host pipeline state object descriptions, update
// their versions when changing.
enum class HostGeometryShader : uint32_t {
  kNone = 0,

  kPoint,
  kQuadFromLineListWithAdjacency,
  kRectangle,

  kCount,
};

// Can be packed in uint32_t.
// May be used in serialized host pipeline state object descriptions, update
// their versions when changing.
enum class HostMSAASamples : uint32_t {
  k1x = 0,
  // May be unsupported by the host GPU, fall back to k2xGuest4xHost with 2
  // samples masked in this case.
  k2xGuest2xHost,
  k2xGuest4xHost,
  k4x,

  kCount,
};

// Direct3D 10.1 and Vulkan standard sample locations.
// https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_standard_multisample_quality_levels
// {X, Y} towards bottom-right, in 1/16 subpixels.
constexpr int_fast8_t kHostMSAAStandardSampleLocations2x[2][2] = {
    {4, 4},    // Bottom-right.
    {-4, -4},  // Top-left.
};
constexpr int_fast8_t kHostMSAAStandardSampleLocations4x[4][2] = {
    {-2, -6},  // Top-left.
    {6, -2},   // Top-right.
    {-6, 2},   // Bottom-left.
    {2, 6},    // Bottom-right.
};

// TODO(Triang3l): Research the sample indices and locations on the Xenos.
// Assuming purely from how multisampled render targets are stored compared to
// single-sampled render targets (as 1x2 or 2x2 pixels) for now.

// Coarse mapping of guest sample indices to the Direct3D 10.1 standard samples.
constexpr uint_fast8_t kGuestToHostMSAAStandardSampleIndices2xTo2x[2] = {
    1,  // Top (top-left).
    0,  // Bottom (bottom-right).
};
constexpr uint_fast8_t kGuestToHostMSAAStandardSampleIndices2xTo4x[2] = {
    0,  // Top (top-left).
    3,  // Bottom (bottom-right).
};
constexpr uint_fast8_t kGuestToHostMSAAStandardSampleIndices4x[4] = {
    0,  // Top-left.
    2,  // Bottom-left.
    1,  // Top-right.
    3,  // Bottom-right.
};

constexpr uint_fast8_t kGuestToHostMSAASampleMask2xTo4x =
    (uint_fast8_t(1) << kGuestToHostMSAAStandardSampleIndices2xTo4x[0]) |
    (uint_fast8_t(1) << kGuestToHostMSAAStandardSampleIndices2xTo4x[1]);

xenos::MsaaSamples HostMSAASamplesToGuest(HostMSAASamples host_msaa_samples);

/// Host MSAA sample mapping supported and preferred for the given
/// configuration.
HostMSAASamples GetHostMSAASamples(xenos::MsaaSamples msaa_samples,
                                   bool with_guest_render_targets,
                                   HostConfig& host_config);

}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_HOST_PIPELINE_STATE_H_
