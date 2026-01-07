/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_HW_TEXTURE_FORMAT_H_
#define XENIA_GPU_HW_HW_TEXTURE_FORMAT_H_

#include <array>

#include "xenia/gpu/xenos.h"

namespace xe {
namespace gpu {
namespace hw {

// Host-side representations of guest texture data and number formats.
enum class HWTextureFormat : unsigned int {
#define XE_GPU_HW_TEXTURE_FORMAT(name, bytes_per_host_element_log2, \
                                 guest_blocks_decompressed)         \
  name,
#include "xenia/gpu/hw/hw_texture_format_table.inc"
#undef XE_GPU_HW_TEXTURE_FORMAT
  kInvalid,
  kCount = kInvalid,
};

using HWTextureFormatGuestFormatTable =
    std::array<HWTextureFormat, size_t(1) << xenos::kFormatBits>;

// In guest to host format mapping tables, expecting that for guest formats for
// which the signedness or the number type is not relevant (such as 16_FLOAT and
// 32_FLOAT), the host format is the same regardless of the signedness or the
// number type.

}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_HW_TEXTURE_FORMAT_H_
