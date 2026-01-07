/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_VULKAN_VULKAN_TEXTURE_FORMAT_H_
#define XENIA_GPU_HW_VULKAN_VULKAN_TEXTURE_FORMAT_H_

#include <array>
#include <climits>
#include <cstdint>

#include "xenia/gpu/xenos.h"
#include "xenia/ui/vulkan/vulkan_device.h"

namespace xe {
namespace gpu {
namespace hw {
namespace vulkan {

enum class VulkanTextureFormatViewIndex : unsigned int {
  kUNorm,
  kSNorm,
  kUScaled,
  kSScaled,

  kCount,
};

constexpr VulkanTextureFormatViewIndex GetVulkanTextureFormatViewIndex(
    const xenos::TextureSign signedness, const bool number_format_scaled) {
  if (signedness == xenos::TextureSign::kSigned) {
    return number_format_scaled ? VulkanTextureFormatViewIndex::kSScaled
                                : VulkanTextureFormatViewIndex::kSNorm;
  }
  // Biasing or degamma is performed after sampling UNorm on the host.
  return number_format_scaled ? VulkanTextureFormatViewIndex::kUScaled
                              : VulkanTextureFormatViewIndex::kUNorm;
}

struct VulkanTextureFormatView {
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkFormatFeatureFlags supported_features = 0;
};

struct VulkanTextureFormat {
  std::array<VkFormat, size_t(VulkanTextureFormatView)> views;

  constexpr VulkanTextureFormat() = default;

  // If the host format is used only for specific guest format signedness or
  // number format, still initialize all view formats to the same value for
  // simplicity.
  constexpr VulkanTextureFormat(const VulkanTextureFormatView& view) {
    views.fill(view);
  }
};

using VulkanTextureFormatTable =
    std::array<VulkanTextureFormat, size_t(HWTextureFormat::kCount)>;

VulkanTextureFormatTable CreateVulkanTextureFormatTable(
    const ui::vulkan::VulkanDevice* vulkan_device);

}  // namespace vulkan
}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_VULKAN_VULKAN_TEXTURE_FORMAT_H_
