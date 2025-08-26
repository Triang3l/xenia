/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/hw/host_render_target_binding_tracker.h"

#include "xenia/gpu/registers.h"

namespace xe {
namespace gpu {
namespace hw {

bool HostRenderTargetBindingTracker::Bindings::AnyBound() const {
  if (depth) {
    return true;
  }
  for (const std::optional<Binding>& color_binding : color) {
    if (color_binding) {
      return true;
    }
  }
  return false;
}

HostRenderTargetBindingTracker::HostRenderTargetBindingTracker(
    const HostConfig* const host_config)
    : host_config_(host_config) {}

void HostRenderTargetBindingTracker::ClearBindings() { bindings_ = Bindings(); }

void HostRenderTargetBindingTracker::UpdateBindings(
    const RegisterFile& regs, const ScissorState& scissor_state,
    const SurfaceState& surface_state,
    const DepthStencilState& depth_stencil_state,
    const ColorWriteState& color_write_state) {
  if (host_config_->RBLLEUsed()) {
    // Writing to a buffer, not to host render targets.
    ClearBindings();
    return;
  }

  if (surface_state.edram_mode != xenos::EdramMode::kColorDepth &&
      surface_state.edram_mode != xenos::EdramMode::kDepthOnly) {
    // Force target-independent rasterization without implicit scissoring to the
    // render target size in the host GPU API (pixels may still be needed for SC
    // viz queries for instance). Also handle disabled RB operations early for
    // the simplicity of the rest of this function.
    ClearBindings();
    return;
  }

  if (surface_state.surface_pitch == 0) {
    // It may be possible (though not tested on the hardware yet) that drawing
    // to up to one row of tiles (8192x16, or 8192x8 with MSAA) can be safely
    // done with a zero surface pitch, but this case is currently not handled
    // throughout the emulator, so just disable render targets for safety and
    // simplicity.
    ClearBindings();
    return;
  }

  // TODO(Triang3l): Round the surface pitch to EDRAM tiles (precalculate in
  // SurfaceState).

  // Unbind all render targets if the surface layout was changed. This is done
  // unconditionally and early to make sure that after this function, the host
  // bindings can't be inconsistent with the SurfaceState.
  if (bindings_surface_pitch_ != surface_state.surface_pitch ||
      bindings_msaa_samples_ != surface_state.msaa_samples) {
    ClearBindings();
    bindings_surface_pitch_ = surface_state.surface_pitch;
    bindings_msaa_samples_ = surface_state.msaa_samples;
  }

  const bool depth_stencil_enabled = depth_stencil_state.DepthStencilEnabled();

  bool accumulated_bindings_outdated = false;

  // Set the new bindings.
  const auto rb_depth_info = regs.Get<reg::RB_DEPTH_INFO>();
  if (depth_stencil_enabled) {
    const Binding<xenos::DepthRenderTargetFormat> new_depth_binding(
        rb_depth_info.depth_base, rb_depth_info.depth_format);
    if (bindings_.depth && bindings_.depth.value() != new_depth_binding) {
      accumulated_bindings_outdated = true;
    }
    bindings_.depth = new_depth_binding;
  }
  for (unsigned color_target_index = 0;
       color_target_index < xenos::kMaxColorRenderTargets;
       ++color_target_index) {
    if (!color_write_state.GetTargetComponentMask(color_target_index)) {
      continue;
    }
    const auto rb_color_info = regs.Get<reg::RB_COLOR_INFO>(
        reg::RB_COLOR_INFO::rt_register_indices[color_target_index]);
    const Binding<xenos::ColorRenderTargetFormat> new_color_binding(
        rb_color_info.color_base,
        GetColorHostBindingFormat(rb_color_info.color_format));
    std::optional<Binding<xenos::ColorRenderTargetFormat>>& color_binding_ref =
        bindings_.color[color_target_index];
    if (color_binding_ref && color_binding_ref.value() != new_color_binding) {
      accumulated_bindings_outdated = true;
    }
    color_binding_ref = new_color_binding;
  }

  // If any active binding was actually changed, reset all inactive bindings.
  // There's no need to try to optimize out host render target changing (host
  // render pass ending) by not unbinding render targets when not necessary in
  // this case, because the host render target bindings are going to be changed
  // anyway.
  if (accumulated_bindings_outdated) {
    if (!depth_stencil_enabled) {
      bindings_.depth.reset();
    }
    for (unsigned color_target_index = 0;
         color_target_index < xenos::kMaxColorRenderTargets;
         ++color_target_index) {
      if (!color_write_state.GetTargetComponentMask(color_target_index)) {
        bindings_.color[color_target_index].reset();
      }
    }
  }

  if (!depth_stencil_enabled && !color_write_state.component_masks) {
    if (scissor_state.br[0] > surface_state.surface_pitch) {
      // Use target-independent rasterization to disable implicit scissoring to
      // the render target size on the host.
      ClearBindings();
    } else {
      if (surface_state.msaa_samples == xenos::MsaaSamples::k2X &&
          host_config->MSAA2xWithGuestRenderTargetsUsed() !=
              host_config->MSAA2xWithoutRenderTargetsUsed() &&
          !bindings_.AnyBound()) {
        // Different host MSAA sample counts may be needed to emulate guest 2x
        // MSAA depending on whether target-independent rasterization is used -
        // a common situation in Direct3D 11/12 with its ForcedSampleCount
        // limitations.
        // This may result in different sample locations for 2x MSAA with and
        // without render targets, and that may slightly affect coverage, and
        // thus occlusion query results: if the game performs one query with the
        // depth test always passing to obtain the total number of samples in
        // the object, and another with the actual depth test to determine the
        // number of visible samples, if the former draw is done with target-
        // independent rasterization and the latter without it, the sample
        // counts may be slightly different between the two queries if the
        // object is fully visible.
        // For draws with the color / depth or the depth-only EDRAM mode, avoid
        // using target-independent rasterization unless it's really needed
        // (especially because Xenia may disable render targets by itself when
        // it determines that they aren't needed, for instance, if the depth /
        // stencil test always passes and never writes) to use the same host
        // MSAA sample locations regardless of whether guest render targets are
        // needed, by binding a dummy render target.
        bindings_.depth = Binding<xenos::DepthRenderTargetFormat>(
            rb_depth_info.depth_base, rb_depth_info.depth_format);
      }
    }
  }
}

xenos::ColorRenderTargetFormat
HostRenderTargetBindingTracker::GetColorHostBindingFormat(
    const xenos::ColorRenderTargetFormat guest_format) const {
  switch (guest_format) {
    case xenos::ColorRenderTargetFormat::k_8_8_8_8_GAMMA:
      switch (host_config_->GetRBGammaMode()) {
        case HostConfig::RBGammaMode::k8BitBlendAware:
        case HostConfig::RBGammaMode::k8BitBlendUnaware:
          return xenos::ColorRenderTargetFormat::k_8_8_8_8;
        default:
          return xenos::ColorRenderTargetFormat::k_8_8_8_8_GAMMA;
      }
    case xenos::ColorRenderTargetFormat::k_2_10_10_10_AS_10_10_10_10:
      return xenos::ColorRenderTargetFormat::k_2_10_10_10;
    case xenos::ColorRenderTargetFormat::k_2_10_10_10_FLOAT_AS_16_16_16_16:
      return xenos::ColorRenderTargetFormat::k_2_10_10_10_FLOAT;
    default:
      return guest_format;
  }
}

}  // namespace hw
}  // namespace gpu
}  // namespace xe
