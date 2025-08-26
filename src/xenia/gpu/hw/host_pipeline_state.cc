/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/hw/host_pipeline_state.h"

#include "xenia/base/assert.h"

namespace xe {
namespace gpu {
namespace hw {

HostHullShader GetHostHullShader(
    const PipelineState::TessellationDomain tessellation_domain,
    const xenos::TessellationMode tessellation_mode) {
  switch (tessellation_domain) {
    case PipelineState::TessellationDomain::kNone:
      return HostHullShader::kNone;

    case PipelineState::TessellationDomain::kLinePrimitive:
      switch (tessellation_mode) {
        case xenos::TessellationMode::kDiscrete:
          return HostHullShader::kLinePrimitiveDiscrete;
        case xenos::TessellationMode::kContinuous:
          return HostHullShader::kLinePrimitiveContinuous;
        default:
          assert_unhandled_case(tessellation_mode);
          return HostHullShader::kLinePrimitiveContinuous;
      }

    case PipelineState::TessellationDomain::kLinePatch:
      switch (tessellation_mode) {
        case xenos::TessellationMode::kDiscrete:
          return HostHullShader::kLinePatchDiscrete;
        case xenos::TessellationMode::kContinuous:
          return HostHullShader::kLinePatchContinuous;
        case xenos::TessellationMode::kAdaptive:
          return HostHullShader::kLinePatchAdaptive;
        default:
          assert_unhandled_case(tessellation_mode);
          return HostHullShader::kLinePatchContinuous;
      }

    case PipelineState::TessellationDomain::kTrianglePrimitive:
      switch (tessellation_mode) {
        case xenos::TessellationMode::kDiscrete:
          return HostHullShader::kTrianglePrimitiveDiscrete;
        case xenos::TessellationMode::kContinuous:
          return HostHullShader::kTrianglePrimitiveContinuous;
        default:
          assert_unhandled_case(tessellation_mode);
          return HostHullShader::kTrianglePrimitiveContinuous;
      }

    case PipelineState::TessellationDomain::kTrianglePatch:
      switch (tessellation_mode) {
        case xenos::TessellationMode::kDiscrete:
          return HostHullShader::kTrianglePatchDiscrete;
        case xenos::TessellationMode::kContinuous:
          return HostHullShader::kTrianglePatchContinuous;
        case xenos::TessellationMode::kAdaptive:
          return HostHullShader::kTrianglePatchAdaptive;
        default:
          assert_unhandled_case(tessellation_mode);
          return HostHullShader::kTrianglePatchContinuous;
      }

    case PipelineState::TessellationDomain::kQuadPrimitive:
      switch (tessellation_mode) {
        case xenos::TessellationMode::kDiscrete:
          return HostHullShader::kQuadPrimitiveDiscrete;
        case xenos::TessellationMode::kContinuous:
          return HostHullShader::kQuadPrimitiveContinuous;
        default:
          assert_unhandled_case(tessellation_mode);
          return HostHullShader::kQuadPrimitiveContinuous;
      }

    case PipelineState::TessellationDomain::kQuadPatch:
      switch (tessellation_mode) {
        case xenos::TessellationMode::kDiscrete:
          return HostHullShader::kQuadPatchDiscrete;
        case xenos::TessellationMode::kContinuous:
          return HostHullShader::kQuadPatchContinuous;
        case xenos::TessellationMode::kAdaptive:
          return HostHullShader::kQuadPatchAdaptive;
        default:
          assert_unhandled_case(tessellation_mode);
          return HostHullShader::kQuadPatchContinuous;
      }

    default:
      assert_unhandled_case(tessellation_domain);
      return HostHullShader::kNone;
  }
}

xenos::MsaaSamples HostMSAASamplesToGuest(
    const HostMSAASamples host_msaa_samples) {
  switch (host_msaa_samples) {
    case HostMSAASamples::k1x:
      return xenos::MsaaSamples::k1X;
    case HostMSAASamples::k2xGuest2xHost:
    case HostMSAASamples::k2xGuest4xHost:
      return xenos::MsaaSamples::k2X;
    case HostMSAASamples::k4x:
      return xenos::MsaaSamples::k4X;
    default:
      assert_unhandled_case(host_msaa_samples);
      return xenos::MsaaSamples::k1X;
  }
}

HostMSAASamples GetHostMSAASamples(const xenos::MsaaSamples msaa_samples,
                                   const bool with_guest_render_targets,
                                   const HostConfig& host_config) {
  switch (msaa_samples) {
    case xenos::MsaaSamples::k1X:
      return HostMSAASamples::k1x;
    case xenos::MsaaSamples::k2X:
      if (with_guest_render_targets
              ? host_config.MSAA2xWithGuestRenderTargetsUsed()
              : host_config.MSAA2xWithoutRenderTargetsUsed()) {
        return HostMSAASamples::k2xGuest2xHost;
      }
      return HostMSAASamples::k2xGuest4xHost;
    case xenos::MsaaSamples::k4X:
      return HostMSAASamples::k4x;
    default:
      assert_unhandled_case(msaa_samples);
      return HostMSAASamples::k1X;
  }
}

}  // namespace hw
}  // namespace gpu
}  // namespace xe
