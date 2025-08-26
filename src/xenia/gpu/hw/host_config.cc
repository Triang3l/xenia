/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2025 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/hw/host_config.h"

#include "xenia/base/assert.h"
#include "xenia/base/cvar.h"
#include "xenia/base/platform.h"

// Note that configuration variables are used-faced, therefore in their names
// and descriptions, generic graphics terms should be preferred over the ATI
// internal terminology. For example:
// - "Render targets" or "RT" (abbreviation may be preferred due to the large
//   number of cvars related to it) instead of "EDRAM", "RB" or "render backend"
//   (this is the most important part as the latter can easily be confused with
//   the GPU emulation host implementation backends, such as the Vulkan-based,
//   the Direct3D 12-based ones).
// - "Tessellation" instead of "HOS" or "higher-order surfaces".
// - "Rasterization" instead of "SC" or "scan conversion".
// - "Texture" or "tex" instead of "TC", "texture cache" or "tfetch".
// In addition, avoid using the term "PC" in favor of "host" as mobile platforms
// are also targeted.

DEFINE_bool(
    gpu_hw_downlevel_host_msaa_2x_without_render_targets, true,
    "Use host 2x MSAA without render targets instead of emulating guest 2x "
    "MSAA via host 4x MSAA with 2 samples masked, if conditionally supported.",
    "GPU Debug");
DEFINE_bool(
    gpu_hw_downlevel_host_msaa_2x_with_guest_render_targets, true,
    "Use host 2x MSAA when using render target HLE with guest render targets "
    "bound instead of emulating guest 2x MSAA via host 4x MSAA with 2 samples "
    "masked, if conditionally supported.",
    "GPU Debug");

DEFINE_bool(
    gpu_hw_rt_lle, false,
    "Use the low-level emulation (LLE) path for render targets (color writing "
    "and depth/stencil testing).\n"
    "- When disabled (HLE), native render target textures on the host GPU are "
    "used.\n"
    "  Because some pixel formats present on the Xbox 360 don't have "
    "equivalents on host GPUs, the HLE path needs to approximate them with "
    "other formats with varying levels of accuracy, depending on the game's "
    "rendering architecture, emulation configuration, and host GPU feature "
    "support.\n"
    "  Data inheritance between guest render target views backed by shared "
    "EDRAM regions is done on the HLE path by copying data between host render "
    "target textures.\n"
    "- When enabled (LLE), all depth/stencil testing, color blending and "
    "packing is done by the emulator logic itself working directly with guest "
    "pixel format encodings.\n",
    "  Data is stored directly in a buffer representing the whole EDRAM when "
    "LLE is used.\n"
    "In general, the LLE path provides much higher accuracy in some games.\n"
    "However, the performance impact of the LLE path may be noticeable, "
    "depending on the game, because it's not the way GPUs normally write pixel "
    "data.\n"
    "When using a host graphics debugger like RenderDoc or PIX, the render "
    "target visualization in the host debugger will only be displayed on the "
    "HLE path.\n"
    "For LLE, the host GPU and driver must support raster-ordered memory "
    "access in shaders:"
// The GPU API names are in alphabetical order.
#if XE_PLATFORM_WIN32
    "\n- On Direct3D: Rasterizer Ordered Views (ROV)\n"
#endif
#if XE_PLATFORM_MAC
    "\n- On Metal: Raster Order Groups"
#endif
    "\n- On Vulkan: Fragment Shader Interlock"
    "GPU");

DEFINE_bool(
    gpu_hw_downlevel_rt_hle_separate_stencil_ref_mask, true,
    "Use separate stencil references and masks for front and back triangle "
    "faces, if conditionally supported."
    "GPU Debug");

DEFINE_bool(
    gpu_hw_downlevel_rt_hle_constant_alpha_blend_factor, true,
    "Use the blend constant alpha factor for RGB blending instead of "
    "arbitrarily choosing between RGB and AAA components if both are needed at "
    "the same time, if conditionally supported."
    "GPU Debug");

namespace xe {
namespace gpu {
namespace hw {

bool HostConfig::HostCapabilities::IsSupported(
    const SupportLevel support_level, const bool support_override_cvar_value) {
  if (support_level == SupportLevel::kNotSupported) {
    return false;
  }
  if (support_level == SupportLevel::kSupportedSwitchable) {
    return support_override_cvar_value;
  }
  assert_true(support_level == SupportLevel::kSupportedMandatory);
  return true;
}

HostConfig::HostConfig(const HostCapabilities& host_capabilities)
    : host_capabilities_(host_capabilities) {
  UpdateFromCvarsAndHostCapabilities();
}

void HostConfig::UpdateFromCvarsAndHostCapabilities() {
  msaa_2x_without_render_targets_ = HostCapabilities::IsSupported(
      host_capabilities_.msaa_2x_without_render_targets,
      cvars::gpu_hw_downlevel_host_msaa_2x_without_render_targets);
  msaa_2x_with_guest_render_targets_ = HostCapabilities::IsSupported(
      host_capabilities_.msaa_2x_with_guest_render_targets,
      cvars::gpu_hw_downlevel_host_msaa_2x_with_guest_render_targets);

  rb_lle_ = host_capabilities.rb_lle && cvars::gpu_hw_rt_lle;

  rb_hle_separate_stencil_ref_mask_ = HostCapabilities::IsSupported(
      host_capabilities.rb_hle_separate_stencil_ref_mask,
      cvars::gpu_hw_downlevel_rt_hle_separate_stencil_ref_mask);

  rt_hle_constant_alpha_blend_factor_ = HostCapabilities::IsSupported(
      host_capabilities.rt_hle_constant_alpha_blend_factor,
      cvars::gpu_hw_downlevel_rt_hle_constant_alpha_blend_factor);
}

}  // namespace hw
}  // namespace gpu
}  // namespace xe
