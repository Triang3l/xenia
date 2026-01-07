/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_HW_TEXTURE_H_
#define XENIA_GPU_HW_HW_TEXTURE_H_

#include <cstdint>

#include "xenia/base/assert.h"
#include "xenia/gpu/hw/hw_texture_format.h"

namespace xe {
namespace gpu {
namespace hw {

enum class HWTextureDimensionality {
  k2DArray,
  k3D,
  kCube,

  kCount,
};

struct HWTextureDescription {
  HWTextureDimensionality dimensionality;

  uint_least32_t width;
  uint_least16_t height;
  uint_least16_t depth_or_array_layers;

  uint_least8_t mip_levels;

  HWTextureFormat format;

  uint_fast16_t Get3DDepth() const {
    return dimensionality == HWTextureDimensionality::k3D
               ? depth_or_array_layers
               : 1;
  }
  uint_fast16_t GetArrayLayers() const {
    return dimensionality == HWTextureDimensionality::k2DArray
               ? depth_or_array_layers
               : 1;
  }
};

class HWTextureProvider;

class HWTexture {
 public:
  HWTexture(const HWTexture&) = delete;
  HWTexture& operator=(const HWTexture&) = delete;
  HWTexture(HWTexture&&) = delete;
  HWTexture& operator=(HWTexture&&) = delete;

  virtual ~HWTexture() = default;

  HWTextureProvider* GetProvider() const { return provider_; }

  const HWTextureDescription& GetDescription() const { return description_; }

 protected:
  explicit HWTexture(HWTextureProvider* const provider,
                     const HWTextureDescription& description)
      : provider_(provider), description_(description) {
    assert_not_null(provider);
  }

 private:
  HWTextureProvider* provider_;

  HWTextureDescription description_;
};

class HWTextureProvider {
 public:
  HWTextureProvider(const HWTextureProvider&) = delete;
  HWTextureProvider& operator=(const HWTextureProvider&) = delete;
  HWTextureProvider(HWTextureProvider&&) = delete;
  HWTextureProvider& operator=(HWTextureProvider&&) = delete;

  virtual ~HWTextureProvider() = default;

 protected:
  explicit HWTextureProvider() = default;
};

}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_HW_TEXTURE_H_
