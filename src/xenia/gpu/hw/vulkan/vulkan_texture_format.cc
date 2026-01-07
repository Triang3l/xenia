/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/hw/vulkan/vulkan_texture_format.h"

#include "xenia/base/assert.h"

namespace xe {
namespace gpu {
namespace hw {
namespace vulkan {

VulkanTextureFormatTable CreateVulkanTextureFormatTable(
    const ui::vulkan::VulkanDevice* const vulkan_device) {
  VulkanTextureFormatTable formats;

  const VkPhysicalDevice physical_device = vulkan_device->physical_device();
  const PFN_vkGetPhysicalDeviceFormatProperties get_format_properties =
      vulkan_device->vulkan_instance()
          ->functions()
          .vkGetPhysicalDeviceFormatProperties;

  const auto make_view = [physical_device, get_format_properties](
                             const VkFormat format) -> VulkanTextureFormatView {
    VkFormatProperties format_properties = {};
    get_format_properties(physical_device, format, &format_properties);
    return {format, format_properties.optimalTilingFeatures};
  };

  // For formats that are not a part of the minimum Vulkan version supported by
  // Xenia, the support for the extension or the Vulkan version the format was
  // added in must be checked explicitly before getting the supported features.
  // VUID-vkGetPhysicalDeviceFormatProperties-format-parameter:
  // "format must be a valid VkFormat value"

  const VulkanTextureFormatView view_r8_unorm = make_view(VK_FORMAT_R8_UNORM);
  const VulkanTextureFormatView view_rg8_unorm =
      make_view(VK_FORMAT_R8G8_UNORM);
  const VulkanTextureFormatView view_rgba8_unorm =
      make_view(VK_FORMAT_R8G8B8A8_UNORM);

  const VulkanTextureFormatView view_rgba8_snorm =
      make_view(VK_FORMAT_R8G8B8A8_SNORM);

  const VulkanTextureFormatView view_r16_unorm = make_view(VK_FORMAT_R16_UNORM);
  const VulkanTextureFormatView view_rg16_unorm =
      make_view(VK_FORMAT_R16G16_UNORM);
  const VulkanTextureFormatView view_rgba16_unorm =
      make_view(VK_FORMAT_R16G16B16A16_UNORM);

  const VulkanTextureFormatView view_rgba16_snorm =
      make_view(VK_FORMAT_R16G16B16A16_SNORM);

  const VulkanTextureFormatView view_r16_float =
      make_view(VK_FORMAT_R16_SFLOAT);
  const VulkanTextureFormatView view_rg16_float =
      make_view(VK_FORMAT_R16G16_SFLOAT);
  const VulkanTextureFormatView view_rgba16_float =
      make_view(VK_FORMAT_R16G16B16A16_SFLOAT);

  const VulkanTextureFormatView view_r32_float =
      make_view(VK_FORMAT_R32_SFLOAT);
  const VulkanTextureFormatView view_rg32_float =
      make_view(VK_FORMAT_R32G32_SFLOAT);
  const VulkanTextureFormatView view_rgba32_float =
      make_view(VK_FORMAT_R32G32B32A32_SFLOAT);

  formats[size_t(HWTextureFormat::k_R1_Reverse_via_R8_UNorm)] = view_r8_unorm;

  formats[size_t(HWTextureFormat::k_R1_via_R8_UNorm)] = view_r8_unorm;

  formats[size_t(HWTextureFormat::k_R8)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] = view_r8_unorm;
  formats[size_t(HWTextureFormat::k_R8)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] =
      make_view(VK_FORMAT_R8_SNORM);
  formats[size_t(HWTextureFormat::k_R8)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_R8_USCALED);
  formats[size_t(HWTextureFormat::k_R8)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_R8_SSCALED);
  formats[size_t(HWTextureFormat::k_R8_UScaled_via_R16_Float)] = view_r16_float;
  formats[size_t(HWTextureFormat::k_R8_SScaled_via_R16_Float)] = view_r16_float;

  formats[size_t(HWTextureFormat::k_RG8)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] = view_rg8_unorm;
  formats[size_t(HWTextureFormat::k_RG8)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] =
      make_view(VK_FORMAT_R8G8_SNORM);
  formats[size_t(HWTextureFormat::k_RG8)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_R8G8_USCALED);
  formats[size_t(HWTextureFormat::k_RG8)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_R8G8_SSCALED);
  formats[size_t(HWTextureFormat::k_RG8_UScaled_via_RG16_Float)] =
      view_rg16_float;
  formats[size_t(HWTextureFormat::k_RG8_SScaled_via_RG16_Float)] =
      view_rg16_float;

  formats[size_t(HWTextureFormat::k_RGBA8)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] = view_rgba8_unorm;
  formats[size_t(HWTextureFormat::k_RGBA8)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] = view_rgba8_snorm;
  formats[size_t(HWTextureFormat::k_RGBA8)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_R8G8B8A8_USCALED);
  formats[size_t(HWTextureFormat::k_RGBA8)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_R8G8B8A8_SSCALED);
  formats[size_t(HWTextureFormat::k_RGBA8_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RGBA8_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  VkFormatProperties properties_gb8_gr8_unorm = {};
  VkFormatProperties properties_bg8_rg8_unorm = {};
  if (vulkan_device->extensions().ext_1_1_KHR_sampler_ycbcr_conversion) {
    get_format_properties(physical_device, VK_FORMAT_G8B8G8R8_422_UNORM,
                          &properties_gb8_gr8_unorm);
    get_format_properties(physical_device, VK_FORMAT_B8G8R8G8_422_UNORM,
                          &properties_bg8_rg8_unorm);
  }

  formats[size_t(HWTextureFormat::k_GB8_GR8_UNorm)] = {
      VK_FORMAT_G8B8G8R8_422_UNORM,
      properties_gb8_gr8_unorm.optimalTilingFeatures};
  // Same host views as for RGBA8, must initialize before this format to reuse.
  assert_true(formats[size_t(HWTextureFormat::k_RGBA8)]
                  .views[size_t(VulkanTextureFormatViewIndex::kUNorm)]
                  .format != VK_FORMAT_UNDEFINED);
  formats[size_t(HWTextureFormat::k_GB8_GR8_via_RGBA8)] =
      formats[size_t(HWTextureFormat::k_RGBA8)];
  formats[size_t(HWTextureFormat::k_GB8_GR8_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_GB8_GR8_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  formats[size_t(HWTextureFormat::k_BG8_RG8_UNorm)] = {
      VK_FORMAT_B8G8R8G8_422_UNORM,
      properties_bg8_rg8_unorm.optimalTilingFeatures};
  // Same host views as for RGBA8, must initialize before this format to reuse.
  assert_true(formats[size_t(HWTextureFormat::k_RGBA8)]
                  .views[size_t(VulkanTextureFormatViewIndex::kUNorm)]
                  .format != VK_FORMAT_UNDEFINED);
  formats[size_t(HWTextureFormat::k_BG8_RG8_via_RGBA8)] =
      formats[size_t(HWTextureFormat::k_RGBA8)];
  formats[size_t(HWTextureFormat::k_BG8_RG8_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_BG8_RG8_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  formats[size_t(HWTextureFormat::k_RGB5A1_UNorm_via_BGR5A1_Packed)] =
      make_view(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
  formats[size_t(HWTextureFormat::k_RGB5A1_UNorm_via_RGBA8)] = view_rgba8_unorm;
  formats[size_t(HWTextureFormat::k_RGB5A1_SNorm_via_RGBA8)] = view_rgba8_snorm;
  formats[size_t(HWTextureFormat::k_RGB5A1_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RGB5A1_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  const VulkanTextureFormatView view_b5g6r5_unorm =
      make_view(VK_FORMAT_R5G6B5_UNORM_PACK16);
  const VulkanTextureFormatView view_rg11b10_float =
      make_view(VK_FORMAT_B10G11R11_UFLOAT_PACK32);

  formats[size_t(HWTextureFormat::k_R5G6B5_UNorm_via_B5G6R5_Packed)] =
      view_b5g6r5_unorm;
  formats[size_t(HWTextureFormat::k_R5G6B5_UNorm_via_RGBA8)] = view_rgba8_unorm;
  formats[size_t(HWTextureFormat::k_R5G6B5_SNorm_via_RGBA8)] = view_rgba8_snorm;
  formats[size_t(HWTextureFormat::k_R5G6B5_UScaled_via_RG11B10_Packed_Float)] =
      view_rg11b10_float;
  formats[size_t(HWTextureFormat::k_R5G6B5_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  formats[size_t(HWTextureFormat::k_RG5B6_UNorm_via_B5G6R5_Packed_RBG)] =
      view_b5g6r5_unorm;
  formats[size_t(HWTextureFormat::k_RG5B6_UNorm_via_RGBA8)] = view_rgba8_unorm;
  formats[size_t(HWTextureFormat::k_RG5B6_SNorm_via_RGBA8)] = view_rgba8_snorm;
  formats[size_t(HWTextureFormat::k_RG5B6_UScaled_via_RG11B10_Packed_Float)] =
      view_rg11b10_float;
  formats[size_t(HWTextureFormat::k_RG5B6_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  // Not using `VK_EXT_4444_formats` because `VK_FORMAT_B4G4R4A4_UNORM_PACK16`
  // is mandatory.
  const VulkanTextureFormatView view_argb4_unorm =
      make_view(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
  formats[size_t(HWTextureFormat::k_RGBA4_UNorm_via_ARGB4_Packed)] =
      view_argb4_unorm;
  const VulkanTextureFormatView view_abgr4_unorm =
      make_view(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
  formats[size_t(HWTextureFormat::k_RGBA4_UNorm_via_ABGR4_Packed)] =
      view_abgr4_unorm;
  formats[size_t(HWTextureFormat::k_RGBA4_UNorm_via_RGBA8)] = view_rgba8_unorm;
  formats[size_t(HWTextureFormat::k_RGBA4_SNorm_via_RGBA8)] = view_rgba8_snorm;
  formats[size_t(HWTextureFormat::k_RGBA4_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RGBA4_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  formats[size_t(HWTextureFormat::k_RGB10A2_Packed)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] =
      make_view(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
  formats[size_t(HWTextureFormat::k_RGB10A2_Packed)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] =
      make_view(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
  formats[size_t(HWTextureFormat::k_RGB10A2_Packed)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
  formats[size_t(HWTextureFormat::k_RGB10A2_Packed)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
  formats[size_t(HWTextureFormat::k_RGB10A2_SNorm_via_RGBA16_Float)] =
      view_r16_float;
  formats[size_t(HWTextureFormat::k_RGB10A2_UScaled_via_RGBA16_Float)] =
      view_r16_float;
  formats[size_t(HWTextureFormat::k_RGB10A2_SScaled_via_RGBA16_Float)] =
      view_r16_float;

  formats[size_t(HWTextureFormat::k_RG11B10_UNorm_via_RGBA16)] =
      view_rgba16_unorm;
  formats[size_t(HWTextureFormat::k_RG11B10_UNorm_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RG11B10_SNorm_via_RGBA16)] =
      view_rgba16_snorm;
  formats[size_t(HWTextureFormat::k_RG11B10_SNorm_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RG11B10_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RG11B10_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  formats[size_t(HWTextureFormat::k_R10GB11_UNorm_via_RGBA16)] =
      view_rgba16_unorm;
  formats[size_t(HWTextureFormat::k_R10GB11_UNorm_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_R10GB11_SNorm_via_RGBA16)] =
      view_rgba16_snorm;
  formats[size_t(HWTextureFormat::k_R10GB11_SNorm_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_R10GB11_UScaled_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_R10GB11_SScaled_via_RGBA16_Float)] =
      view_rgba16_float;

  formats[size_t(HWTextureFormat::k_DXT1_UNorm)] =
      make_view(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
  formats[size_t(HWTextureFormat::k_DXT1_UNorm_via_RGBA8)] = view_rgba8_unorm;

  formats[size_t(HWTextureFormat::k_DXT3_UNorm)] =
      make_view(VK_FORMAT_BC3_UNORM_BLOCK);
  formats[size_t(HWTextureFormat::k_DXT3_UNorm_via_RGBA8)] = view_rgba8_unorm;

  formats[size_t(HWTextureFormat::k_DXT5_UNorm)] =
      make_view(VK_FORMAT_BC5_UNORM_BLOCK);
  formats[size_t(HWTextureFormat::k_DXT5_UNorm_via_RGBA8)] = view_rgba8_unorm;

  formats[size_t(HWTextureFormat::k_DXT5A_UNorm)] =
      make_view(VK_FORMAT_BC4_UNORM_BLOCK);
  formats[size_t(HWTextureFormat::k_DXT5A_UNorm_via_R16)] = view_r16_unorm;
  formats[size_t(HWTextureFormat::k_DXT5A_UNorm_via_R8)] = view_r8_unorm;

  formats[size_t(HWTextureFormat::k_DXN_UNorm)] =
      make_view(VK_FORMAT_BC4_UNORM_BLOCK);
  formats[size_t(HWTextureFormat::k_DXN_UNorm_via_RG16)] = view_rg16_unorm;
  formats[size_t(HWTextureFormat::k_DXN_UNorm_via_RG8)] = view_rg8_unorm;

  // Not using `VK_EXT_4444_formats` because `VK_FORMAT_B4G4R4A4_UNORM_PACK16`
  // is mandatory.
  formats[size_t(HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_ARGB4_Packed)] =
      view_argb4_unorm;
  formats[size_t(HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_ABGR4_Packed)] =
      view_abgr4_unorm;
  formats[size_t(HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_RGBA8)] =
      view_rgba8_unorm;

  formats[size_t(HWTextureFormat::k_CTX1_UNorm_via_RG8)] = view_rg8_unorm;

  formats[size_t(HWTextureFormat::k_X8R24_UNorm_via_R32_Float)] =
      view_r32_float;
  formats[size_t(HWTextureFormat::k_X8R24_Float_via_R32)] = view_r32_float;

  formats[size_t(HWTextureFormat::k_R16)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] = view_r16_unorm;
  formats[size_t(HWTextureFormat::k_R16)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] =
      make_view(VK_FORMAT_R16_SNORM);
  formats[size_t(HWTextureFormat::k_R16)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_R16_USCALED);
  formats[size_t(HWTextureFormat::k_R16)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_R16_SSCALED);
  formats[size_t(HWTextureFormat::k_R16_UNorm_via_R16_Float)] = view_r16_float;
  formats[size_t(HWTextureFormat::k_R16_SNorm_via_R16_Float)] = view_r16_float;
  formats[size_t(HWTextureFormat::k_R16_UScaled_via_R32_Float)] =
      view_r32_float;
  formats[size_t(HWTextureFormat::k_R16_SScaled_via_R32_Float)] =
      view_r32_float;

  formats[size_t(HWTextureFormat::k_RG16)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] = view_rg16_unorm;
  formats[size_t(HWTextureFormat::k_RG16)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] =
      make_view(VK_FORMAT_R16G16_SNORM);
  formats[size_t(HWTextureFormat::k_RG16)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_R16G16_USCALED);
  formats[size_t(HWTextureFormat::k_RG16)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_R16G16_SSCALED);
  formats[size_t(HWTextureFormat::k_RG16_UNorm_via_RG16_Float)] =
      view_rg16_float;
  formats[size_t(HWTextureFormat::k_RG16_SNorm_via_RG16_Float)] =
      view_rg16_float;
  formats[size_t(HWTextureFormat::k_RG16_UScaled_via_RG32_Float)] =
      view_rg32_float;
  formats[size_t(HWTextureFormat::k_RG16_SScaled_via_RG32_Float)] =
      view_rg32_float;

  formats[size_t(HWTextureFormat::k_RGBA16)]
      .views[size_t(VulkanTextureFormatViewIndex::kUNorm)] = view_rgba16_unorm;
  formats[size_t(HWTextureFormat::k_RGBA16)]
      .views[size_t(VulkanTextureFormatViewIndex::kSNorm)] = view_rgba16_snorm;
  formats[size_t(HWTextureFormat::k_RGBA16)]
      .views[size_t(VulkanTextureFormatViewIndex::kUScaled)] =
      make_view(VK_FORMAT_R16G16B16A16_USCALED);
  formats[size_t(HWTextureFormat::k_RGBA16)]
      .views[size_t(VulkanTextureFormatViewIndex::kSScaled)] =
      make_view(VK_FORMAT_R16G16B16A16_SSCALED);
  formats[size_t(HWTextureFormat::k_RGBA16_UNorm_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RGBA16_SNorm_via_RGBA16_Float)] =
      view_rgba16_float;
  formats[size_t(HWTextureFormat::k_RGBA16_UScaled_via_RGBA32_Float)] =
      view_rgba32_float;
  formats[size_t(HWTextureFormat::k_RGBA16_SScaled_via_RGBA32_Float)] =
      view_rgba32_float;

  formats[size_t(HWTextureFormat::k_R16_Float)] = view_r16_float;

  formats[size_t(HWTextureFormat::k_RG16_Float)] = view_rg16_float;

  formats[size_t(HWTextureFormat::k_RGBA16_Float)] = view_rgba16_float;

  formats[size_t(HWTextureFormat::k_R32_UNorm_via_R32_Float)] = view_r32_float;
  formats[size_t(HWTextureFormat::k_R32_SNorm_via_R32_Float)] = view_r32_float;
  formats[size_t(HWTextureFormat::k_R32_UScaled_via_R32_Float)] =
      view_r32_float;
  formats[size_t(HWTextureFormat::k_R32_SScaled_via_R32_Float)] =
      view_r32_float;

  formats[size_t(HWTextureFormat::k_RG32_UNorm_via_RG32_Float)] =
      view_rg32_float;
  formats[size_t(HWTextureFormat::k_RG32_SNorm_via_RG32_Float)] =
      view_rg32_float;
  formats[size_t(HWTextureFormat::k_RG32_UScaled_via_RG32_Float)] =
      view_rg32_float;
  formats[size_t(HWTextureFormat::k_RG32_SScaled_via_RG32_Float)] =
      view_rg32_float;

  formats[size_t(HWTextureFormat::k_RGBA32_UNorm_via_RGBA32_Float)] =
      view_rgba32_float;
  formats[size_t(HWTextureFormat::k_RGBA32_SNorm_via_RGBA32_Float)] =
      view_rgba32_float;
  formats[size_t(HWTextureFormat::k_RGBA32_UScaled_via_RGBA32_Float)] =
      view_rgba32_float;
  formats[size_t(HWTextureFormat::k_RGBA32_SScaled_via_RGBA32_Float)] =
      view_rgba32_float;

  formats[size_t(HWTextureFormat::k_R32_Float)] = view_r32_float;

  formats[size_t(HWTextureFormat::k_RG32_Float)] = view_rg32_float;

  formats[size_t(HWTextureFormat::k_RGBA32_Float)] = view_rgba32_float;

  return formats;
}

}  // namespace vulkan
}  // namespace hw
}  // namespace gpu
}  // namespace xe
