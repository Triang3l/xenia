/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_HOST_RENDER_TARGET_BINDING_TRACKER_H_
#define XENIA_GPU_HW_HOST_RENDER_TARGET_BINDING_TRACKER_H_

#include <array>
#include <cstdint>
#include <optional>

#include "xenia/gpu/hw/host_config.h"
#include "xenia/gpu/pipeline_state.h"
#include "xenia/gpu/register_file.h"
#include "xenia/gpu/xenos.h"

namespace xe {
namespace gpu {
namespace hw {

/// Tracker of host framebuffer attachments, performing optimizations to avoid
/// switching the host framebuffer when not needed.
class HostRenderTargetBindingTracker {
 public:
  template <typename Format>
  struct Binding {
    uint_fast16_t edram_base;

    // Different guest formats can be mapped to the same host data and view
    // format if:
    // - Same bits in the EDRAM in the two guest formats are represented by the
    //   same bits in the host resource in the two host formats (the same host
    //   resource can be used if the guest format is changed between the two).
    // - Same host GPU API view format can be used for writing to the target
    //   via the two guest formats (the same host view can be used if the guest
    //   format is changed between the two).
    Format host_binding_format;

    explicit Binding(const uint_fast16_t edram_base,
                     const Format host_binding_format)
        : edram_base(edram_base), host_binding_format(host_binding_format) {}

    bool operator==(const Binding& other) const {
      return host_binding_format == other.host_binding_format &&
             edram_base == other.edram_base;
    }

    bool operator!=(const Binding& other) const { return !(*this == other); }
  };

  struct Bindings {
    std::optional<Binding<xenos::DepthRenderTargetFormat>> depth;
    std::array<std::optional<Binding<xenos::ColorRenderTargetFormat>>> color;

    bool AnyBound() const;
  };

  explicit HostRenderTargetBindingTracker(const HostConfig* host_config);

  void ClearBindings();

  void UpdateBindings(const RegisterFile& regs,
                      const ScissorState& scissor_state,
                      const SurfaceState& surface_state,
                      const DepthStencilState& depth_stencil_state,
                      const ColorWriteState& color_write_state);

  // UpdateBindings must be called prior to getting the bindings.
  //
  // Bindings may be demanded on the host even when they are not needed on the
  // guest side, such as to reduce host render target changing (host render pass
  // ending) when they are not necessary (an example is 58410954 where single
  // depth-only and color draws are interleaved), or to force disable target-
  // independent rasterization.
  // The format field may also not match the guest register in this case (though
  // for purposes like alpha test and alpha to mask, where the RB DB uses the
  // alpha even though the RB CB doesn't write to the target), the actual format
  // in the register must still be used).
  // If a host render target binding is enabled without the corresponding render
  // target being actually needed on the guest side, on the host, a valid view
  // of the host format specified in the binding must be bound.
  //
  // Note that the opposite - render target needed by the guest, but disabled in
  // the host bindings - is also a valid situation (though exceptional, but must
  // be handled safely). This may happen, for example, if the surface pitch is
  // not compatible with the host GPU. In this case, host render targets views
  // must not be bound to the disabled slots.
  const Bindings& GetBindings() const { return bindings_; }

 private:
  const HostConfig* host_config_;

  xenos::ColorRenderTargetFormat GetColorHostBindingFormat(
      xenos::ColorRenderTargetFormat guest_format) const;

  // May be outdated if host bindings are not needed, purely used for tracking
  // purposes, so not exposed publicly (SurfaceState should be used for the
  // actual state instead).
  uint_fast16_t bindings_surface_pitch_ = 0;
  xenos::MsaaSamples bindings_msaa_samples_ = xenos::MsaaSamples::k1X;

  Bindings bindings_;
};

}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_HOST_RENDER_TARGET_BINDING_TRACKER_H_
