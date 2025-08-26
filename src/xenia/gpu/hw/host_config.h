/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_HOST_CONFIG_H_
#define XENIA_GPU_HW_HOST_CONFIG_H_

#include <array>

#include "xenia/gpu/xenos.h"

namespace xe {
namespace gpu {
namespace hw {

// Some places in the emulator, primarily in pipeline state management, refer to
// the concept of "usable with config". This means that the given state is
// supported by the currently used host GPU via the host API, and also that
// games have a way of actually ending up configuring the state that way with
// the given user configuration (otherwise it may be preferable to skip
// processing the given state combination when creating pipeline state objects
// loaded from the persistent storage to avoid wasting the startup time because
// it will be unreachable anyway).

/// Manager of desired and actually supported configuration of
/// hardware-accelerated emulation of the Xenos.
class HostConfig {
 public:
  /// Xenos 8_8_8_8_GAMMA render target (with a curve mapping colors to stored
  /// 8-bit values consisting of 4 linear pieces) emulation path.
  enum class RBGammaMode {
    /// Direct emulation of the Xenos gamma pixel storage format, such as via
    /// programmable blending.
    /// Correct without blending, as well as with blending:
    /// - (v) Source is a color value (from the guest shader).
    /// - (v) Destination is a color value (decoded from the Xenos gamma storage
    ///       format).
    /// - (v) Result is the correct color, and is encoded into the Xenos gamma
    ///       storage format.
    k8BitBlendAware,

    /// R8G8B8A8_UNORM host render target view format, with the Xenos gamma
    /// encoding function applied to the source color in host pixel shaders.
    /// Correct without blending, but blending is wrong:
    /// - (x) Source is a reinterpretation of the Xenos gamma storage format
    ///       bits as UNorm (roughly - disregarding details of intermediate
    ///       precision and rounding for the purposes of this comment).
    /// - (x) Destination is a reinterpretation of the Xenos gamma storage
    ///       format bits as UNorm.
    /// - (x) Result is incorrect.
    /// The blending issues are especially prominent in 4D5307E6, where bullet
    /// hole and footstep decals (mainly on sand) use `src * dest + src * dest`
    /// blending with source color 0.5 used in transparent areas, but after the
    /// pre-conversion, blending receives 0.75 instead of 0.5 as the source
    /// color, resulting in brightening of the surface around the decal.
    k8BitBlendUnaware,

    /// R8G8B8A8_SRGB host render target view format, which also provides more
    /// distinct 8-bit storage values for dark shades and fewer for bright
    /// shades, but the exact distribution and encoding is different from that
    /// used for the same purpose on the Xenos.
    /// Results are always slightly different from the Xenos, but blending
    /// works with actual color values, therefore the 4D5307E6 decal
    /// transparency issue is not present:
    /// - (v) Source is a color value (from the guest shader).
    /// - (~) Destination is a color value (decoded from the sRGB storage
    ///       format), as long as it was initially written through a
    ///       8_8_8_8_GAMMA render target too (otherwise it's a reinterpretation
    ///       of the Xenos gamma format bits as sRGB format bits).
    /// - (~) Result is the correct color as long as the destination color also
    ///       is, but is encoded into the sRGB storage format.
    /// Because the encoding is different, it needs to be handled everywhere the
    /// data is accessed. This includes texture fetches with the "gamma"
    /// component signedness: if the texture contains data that was copied from
    /// a render target with the gamma format, fetching should interpret the
    /// texture data as sRGB-encoded, not as stored in the Xenos gamma format -
    /// but textures from game assets must still be read with the Xenos gamma
    /// encoding. However, covering all cases is not possible here. Some games
    /// have Xenos gamma encoding math in their code, which is not aware of the
    /// format replacement done by the emulator, and when pixels with the sRGB
    /// format are processed by it, the result ends up being incorrect. For
    /// example, the game loading screens in 4541080F become much brighter with
    /// this emulation approach, and it's not possible to convert from the host
    /// R8G8B8A8_SRGB to the Xenos 8_8_8_8_GAMMA without significant precision
    /// loss because both formats have the same number of bits per component.
    k8BitHostSRGB,
  };

  // With a resolution scale of up to 7x7, the scaled EDRAM size is 490 MB,
  // which, if texels in the binding (including internally in the driver) are
  // 32-bit, fits into the 2^D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP
  // (2^27) limit that is present on some GPUs such as NVIDIA ones and affects
  // certain types of buffer bindings there. Also, replication of the first
  // column of pixels to fill in the gap introduced by the half-pixel offset
  // being scaled can be done within 8 pixels (which is within the tile
  // dimension in EDRAM resolving).
  static constexpr unsigned kMaxDrawResolutionScale = 7;

  // Only for initialization, not available publicly because debug override
  // cvars are present for quicker testing on downlevel host GPUs and drivers.
  struct HostCapabilities {
    enum class SupportLevel {
      /// Not supported. The host must provide the necessary fallback.
      kNotSupported,
      /// Supported, but the host implementation provides a fallback so that
      /// it's safe to pretend that it's not supported for downlevel testing.
      kSupportedSwitchable,
      /// Supported, and mandatory in the host implementation, so it doesn't
      /// provide a fallback, and the feature can't be disabled for downlevel
      /// testing.
      kSupportedMandatory,
    };

    static bool IsSupported(SupportLevel support_level,
                            bool support_override_cvar_value = true);

    // All capabilities are kSupportedMandatory by default because
    // implementations on top of host GPU APIs that provide more functionality
    // unconditionally should bear less fallback-related burden.

    /// Host (after the resolution scale is applied) rasterization coordinate
    /// space dimensions (maximum framebuffer size), independent of the maximum
    /// host 2D texture size.
    std::array<uint32_t, 2> max_render_area_size{
        xenos::kTexture2DCubeMaxWidthHeight,
        xenos::kTexture2DCubeMaxWidthHeight};

    /// Maximum host (after the resolution scale is applied) dimensions of 2D
    /// textures for emulation of guest render targets.
    uint32_t max_render_target_texture_width_height =
        xenos::kTexture2DCubeMaxWidthHeight;

    /// Whether the host implementation supports 2x MSAA when drawing without
    /// render targets (for target-independent rasterization). The fallback is
    /// 4x MSAA with 2 samples masked.
    SupportLevel msaa_2x_without_render_targets =
        SupportLevel::kSupportedMandatory;
    /// Whether the host implementation supports 2x MSAA for all supported guest
    /// render target formats for the purposes of drawing, resolving, and EDRAM
    /// ownership transfers. The fallback is 4x MSAA with 2 samples masked.
    SupportLevel msaa_2x_with_guest_render_targets =
        SupportLevel::kSupportedMandatory;

    /// Whether the host implementation supports the low-level Render Backend
    /// emulation path. The host GPU must be able to support binding a buffer
    /// representing the EDRAM to host pixel shaders and read-modify-writes of
    /// MSAA sample data in them in raster order (via the functionality called
    /// fragment shader interlock in Vulkan, rasterizer ordered views in
    /// Direct3D, raster order groups in Metal).
    bool rb_lle = true;

    /// Whether front and back faces can use different stencil references and
    /// masks in RB HLE.
    SupportLevel rb_hle_separate_stencil_ref_mask =
        SupportLevel::kSupportedMandatory;

    /// Whether the "constant alpha" blend factor (rather than only "constant
    /// color") can be used for RGB blending in RB HLE.
    SupportLevel rb_hle_constant_alpha_blend_factor =
        SupportLevel::kSupportedMandatory;
  };

  explicit HostConfig(const HostCapabilities& host_capabilities);

  void UpdateFromCvarsAndHostCapabilities();

  // TODO(Triang3l): Runtime updating, with safe synchronization (via mutual
  // exclusion, as well as the actual cvars being abstracted away as snapshots),
  // between the thread changing the configuration (such as the UI thread) and
  // GPU emulation. Changes should take effect on frame (display frontbuffer
  // swap, or executable switch) boundaries. This is truly relevant only to
  // functionality changing the behavior significantly though, such as render
  // target HLE vs. LLE. Simple downlevel debugging functionality doesn't need
  // to support runtime toggling. Runtime changes, however, may be needed for
  // generic Xenos emulation configuration (such as the half-pixel offset) as
  // opposed to specifically hardware-accelerated emulation. Also, in the future
  // it may be possible to provide a runtime switch of the whole GPU emulation
  // implementation (such as between Direct3D 12 and Vulkan - need to be careful
  // with the flip model swap chain in this specific case though, may possibly
  // have to recreate the window, the top-level one or maybe a child one
  // specifically for the purpose of holding the surface). Probably config
  // update points should be handled in the common code, but trigger a virtual
  // method of the implementation that causes the host config refresh.

  /// Whether to use host 2x multisample antialiasing for guest 2x MSAA when
  /// drawing without render targets (for target-independent rasterization).
  /// If not used, must emulate via 4x with 2 samples masked
  /// (see kGuestToHostMSAAStandardSampleIndices2xTo4x).
  bool MSAA2xWithoutRenderTargetsUsed() const {
    return msaa_2x_without_render_targets_;
  }
  /// Whether to use host 2x multisample antialiasing for guest 2x MSAA when
  /// drawing with at least one binding of a render target with a guest format.
  /// If not used, must emulate via 4x with 2 samples masked
  /// (see kGuestToHostMSAAStandardSampleIndices2xTo4x).
  bool MSAA2xWithGuestRenderTargetsUsed() const {
    return msaa_2x_with_guest_render_targets_;
  }

  /// Whether the low-level Render Backend emulation path is used, where pixels
  /// are stored directly in the guest formats in a buffer representing the
  /// EDRAM instead of host render target textures and processed in pixel
  /// shaders via raster order memory access.
  void RBLLEUsed() const { return rb_lle_; }

  /// Whether front and back faces can use different stencil references and
  /// masks in RB HLE.
  bool RBHLESeparateStencilRefMaskUsed() const {
    return rb_hle_separate_stencil_ref_mask_;
  }
  bool RBSeparateStencilRefMaskUsed() const {
    return RBLLEUsed() || RBHLESeparateStencilRefMaskUsed();
  }

  /// Whether the "constant alpha" blend factor (rather than only "constant
  /// color") can be used for RGB blending in RB HLE.
  bool RBHLEConstantAlphaColorBlendFactorUsed() const {
    return rb_hle_constant_alpha_blend_factor_;
  }
  bool RBConstantAlphaColorBlendFactorUsed() const {
    return RBLLEUsed() || RBHLEConstantAlphaColorBlendFactorUsed();
  }

  RBGammaMode GetRBHLEGammaMode() const { return rb_hle_gamma_mode_; }
  RBGammaMode GetRBGammaMode() const {
    return RBLLEUsed() ? RBGammaMode::k8BitBlendAware : GetRBHLEGammaMode();
  }

  const std::array<unsigned>& GetDrawResolutionScale() const {
    return draw_resolution_scale_;
  }

 private:
  HostCapabilities host_capabilities_;

  bool msaa_2x_without_render_targets_ = false;
  bool msaa_2x_with_guest_render_targets_ = false;

  bool rb_lle_ = false;

  bool rb_hle_separate_stencil_ref_mask_ = false;

  bool rb_hle_constant_alpha_color_blend_factor_ = false;

  RBGammaMode rb_hle_gamma_mode_ = RBGammaMode::k8BitBlendUnaware;

  std::array<unsigned, 2> draw_resolution_scale_{1, 1};
};

}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_HOST_CONFIG_H_
