/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_VULKAN_VULKAN_TEXTURE_H_
#define XENIA_GPU_HW_VULKAN_VULKAN_TEXTURE_H_

#include "xenia/gpu/hw/vulkan/vulkan_texture_format.h"
#include "xenia/gpu/xenos.h"
#include "xenia/ui/vulkan/vulkan_mem_alloc.h"

namespace xe {
namespace gpu {
namespace hw {
namespace vulkan {

class VulkanTextureProvider {
 private:
  static HWTextureFormatGuestFormatTable CreateGuestFormatTable(
      const VulkanTextureFormatTable& format_support_table,
      VulkanTextureFormatViewIndex view_index);
};

}  // namespace vulkan
}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_VULKAN_VULKAN_TEXTURE_H_
