/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_WGF_D3D12_D3D12_TEXTURE_H_
#define XENIA_GPU_HW_WGF_D3D12_D3D12_TEXTURE_H_

#include "xenia/gpu/hw/hw_texture_format.h"
#include "xenia/gpu/xenos.h"
#include "xenia/ui/d3d12/d3d12_api.h"

namespace xe {
namespace gpu {
namespace hw {
namespace wgf {
namespace d3d12 {

class D3D12TextureProvider;

class D3D12Texture : public HWTexture {
 public:
  explicit D3D12Texture(D3D12TextureProvider* const provider,
                        const HWTextureDescription& description,
                        ID3D12Resource* const resource)
      : HWTexture(provider, description), resource_(resource) {}

  ID3D12Resource* GetResource() const { return resource_.Get(); }

 private:
  Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
};

class D3D12TextureProvider {
 private:
  static HWTextureFormatGuestFormatTable CreateGuestFormatTable(
      xenos::TextureSign signedness, bool number_format_scaled);
};

}  // namespace d3d12
}  // namespace wgf
}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_WGF_D3D12_D3D12_TEXTURE_H_
