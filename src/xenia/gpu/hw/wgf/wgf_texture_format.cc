/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/hw/wgf/wgf_texture_format.h"

#include <cstddef>

namespace xe {
namespace gpu {
namespace hw {
namespace wgf {

std::array<WGFTextureFormat, size_t(HWTextureFormat::kCount)>
CreateWGFTextureFormatTable() {
  std::array<WGFTextureFormat, size_t(HWTextureFormat::kCount)> formats;

  formats[size_t(HWTextureFormat::k_R1_Reverse_via_R8_UNorm)] =
      DXGI_FORMAT_R8_UNORM;

  formats[size_t(HWTextureFormat::k_R1_via_R8_UNorm)] = DXGI_FORMAT_R8_UNORM;

  formats[size_t(HWTextureFormat::k_R8)] = {
      DXGI_FORMAT_R8_TYPELESS, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_SNORM};
  formats[size_t(HWTextureFormat::k_R8_UScaled_via_R16_Float)] =
      DXGI_FORMAT_R16_FLOAT;
  formats[size_t(HWTextureFormat::k_R8_SScaled_via_R16_Float)] =
      DXGI_FORMAT_R16_FLOAT;

  formats[size_t(HWTextureFormat::k_RG8)] = {DXGI_FORMAT_R8G8_TYPELESS,
                                             DXGI_FORMAT_R8G8_UNORM,
                                             DXGI_FORMAT_R8G8_SNORM};
  formats[size_t(HWTextureFormat::k_RG8_UScaled_via_RG16_Float)] =
      DXGI_FORMAT_R16G16_FLOAT;
  formats[size_t(HWTextureFormat::k_RG8_SScaled_via_RG16_Float)] =
      DXGI_FORMAT_R16G16_FLOAT;

  formats[size_t(HWTextureFormat::k_RGBA8)] = {DXGI_FORMAT_R8G8B8A8_TYPELESS,
                                               DXGI_FORMAT_R8G8B8A8_UNORM,
                                               DXGI_FORMAT_R8G8B8A8_SNORM};
  formats[size_t(HWTextureFormat::k_RGBA8_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA8_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_GB8_GR8_UNorm_via_GR8_GB8)] =
      DXGI_FORMAT_G8R8_G8B8_UNORM;
  formats[size_t(HWTextureFormat::k_GB8_GR8_via_RGBA8)] = {
      DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_FORMAT_R8G8B8A8_SNORM};
  formats[size_t(HWTextureFormat::k_GB8_GR8_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_GB8_GR8_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_BG8_RG8_UNorm_via_RG8_BG8)] =
      DXGI_FORMAT_R8G8_B8G8_UNORM;
  formats[size_t(HWTextureFormat::k_BG8_RG8_via_RGBA8)] = {
      DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R8G8B8A8_UNORM,
      DXGI_FORMAT_R8G8B8A8_SNORM};
  formats[size_t(HWTextureFormat::k_BG8_RG8_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_BG8_RG8_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_RGB5A1_UNorm_via_BGR5A1_Packed)] =
      DXGI_FORMAT_B5G6R5_UNORM;
  formats[size_t(HWTextureFormat::k_R5G6B5_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;
  formats[size_t(HWTextureFormat::k_R5G6B5_SNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_SNORM;
  formats[size_t(HWTextureFormat::k_R5G6B5_UScaled_via_RG11B10_Packed_Float)] =
      DXGI_FORMAT_R11G11B10_FLOAT;
  formats[size_t(HWTextureFormat::k_R5G6B5_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_RG5B6_UNorm_via_B5G6R5_Packed_RBG)] =
      DXGI_FORMAT_B5G6R5_UNORM;
  formats[size_t(HWTextureFormat::k_RG5B6_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;
  formats[size_t(HWTextureFormat::k_RG5B6_SNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_SNORM;
  formats[size_t(HWTextureFormat::k_RG5B6_UScaled_via_RG11B10_Packed_Float)] =
      DXGI_FORMAT_R11G11B10_FLOAT;
  formats[size_t(HWTextureFormat::k_RG5B6_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_RGBA4_UNorm_via_BGRA4_Packed)] =
      DXGI_FORMAT_B4G4R4A4_UNORM;
  formats[size_t(HWTextureFormat::k_RGBA4_UNorm_via_ABGR4_Packed)] =
      DXGI_FORMAT_A4B4G4R4_UNORM;
  formats[size_t(HWTextureFormat::k_RGBA4_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;
  formats[size_t(HWTextureFormat::k_RGBA4_SNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_SNORM;
  formats[size_t(HWTextureFormat::k_RGBA4_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA4_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_RGB10A2_Packed)] = {
      DXGI_FORMAT_R10G10B10A2_TYPELESS, DXGI_FORMAT_R10G10B10A2_UNORM,
      DXGI_FORMAT_R10G10B10A2_SNORM};
  formats[size_t(HWTextureFormat::k_RGB10A2_SNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RGB10A2_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RGB10A2_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_RG11B10_UNorm_via_RGBA16)] =
      DXGI_FORMAT_R16G16B16A16_UNORM;
  formats[size_t(HWTextureFormat::k_RG11B10_UNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RG11B10_SNorm_via_RGBA16)] =
      DXGI_FORMAT_R16G16B16A16_SNORM;
  formats[size_t(HWTextureFormat::k_RG11B10_SNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RG11B10_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RG11B10_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_R10GB11_UNorm_via_RGBA16)] =
      DXGI_FORMAT_R16G16B16A16_UNORM;
  formats[size_t(HWTextureFormat::k_R10GB11_UNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_R10GB11_SNorm_via_RGBA16)] =
      DXGI_FORMAT_R16G16B16A16_SNORM;
  formats[size_t(HWTextureFormat::k_R10GB11_SNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_R10GB11_UScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_R10GB11_SScaled_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_DXT1_UNorm)] = DXGI_FORMAT_BC1_UNORM;
  formats[size_t(HWTextureFormat::k_DXT1_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;

  formats[size_t(HWTextureFormat::k_DXT3_UNorm)] = DXGI_FORMAT_BC2_UNORM;
  formats[size_t(HWTextureFormat::k_DXT3_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;

  formats[size_t(HWTextureFormat::k_DXT5_UNorm)] = DXGI_FORMAT_BC3_UNORM;
  formats[size_t(HWTextureFormat::k_DXT5_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;

  formats[size_t(HWTextureFormat::k_DXT5A_UNorm)] = DXGI_FORMAT_BC4_UNORM;
  formats[size_t(HWTextureFormat::k_DXT5A_UNorm_via_R16)] =
      DXGI_FORMAT_R16_UNORM;
  formats[size_t(HWTextureFormat::k_DXT5A_UNorm_via_R8)] = DXGI_FORMAT_R8_UNORM;

  formats[size_t(HWTextureFormat::k_DXN_UNorm)] = DXGI_FORMAT_BC5_UNORM;
  formats[size_t(HWTextureFormat::k_DXN_UNorm_via_RG16)] =
      DXGI_FORMAT_R16G16_UNORM;
  formats[size_t(HWTextureFormat::k_DXN_UNorm_via_RG8)] =
      DXGI_FORMAT_R8G8_UNORM;

  formats[size_t(HWTextureFormat::k_DXT3A_UNorm_via_R8)] = DXGI_FORMAT_R8_UNORM;
  formats[size_t(HWTextureFormat::k_DXT3A_SNorm_via_R8)] = DXGI_FORMAT_R8_SNORM;

  formats[size_t(HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_BGRA4_Packed)] =
      DXGI_FORMAT_B4G4R4A4_UNORM;
  formats[size_t(HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_ABGR4_Packed)] =
      DXGI_FORMAT_A4B4G4R4_UNORM;
  formats[size_t(HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_RGBA8)] =
      DXGI_FORMAT_R8G8B8A8_UNORM;

  formats[size_t(HWTextureFormat::k_CTX1_UNorm_via_RG8)] =
      DXGI_FORMAT_R8G8_UNORM;

  formats[size_t(HWTextureFormat::k_X8R24_UNorm_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;

  formats[size_t(HWTextureFormat::k_X8R24_Float_via_R32)] =
      DXGI_FORMAT_R32_FLOAT;

  formats[size_t(HWTextureFormat::k_R16)] = {
      DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_SNORM};
  formats[size_t(HWTextureFormat::k_R16_UNorm_via_R16_Float)] =
      DXGI_FORMAT_R16_FLOAT;
  formats[size_t(HWTextureFormat::k_R16_SNorm_via_R16_Float)] =
      DXGI_FORMAT_R16_FLOAT;
  formats[size_t(HWTextureFormat::k_R16_UScaled_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;
  formats[size_t(HWTextureFormat::k_R16_SScaled_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;

  formats[size_t(HWTextureFormat::k_RG16)] = {DXGI_FORMAT_R16G16_TYPELESS,
                                              DXGI_FORMAT_R16G16_UNORM,
                                              DXGI_FORMAT_R16G16_SNORM};
  formats[size_t(HWTextureFormat::k_RG16_UNorm_via_RG16_Float)] =
      DXGI_FORMAT_R16G16_FLOAT;
  formats[size_t(HWTextureFormat::k_RG16_SNorm_via_RG16_Float)] =
      DXGI_FORMAT_R16G16_FLOAT;
  formats[size_t(HWTextureFormat::k_RG16_UScaled_via_RG32_Float)] =
      DXGI_FORMAT_R32G32_FLOAT;
  formats[size_t(HWTextureFormat::k_RG16_SScaled_via_RG32_Float)] =
      DXGI_FORMAT_R32G32_FLOAT;

  formats[size_t(HWTextureFormat::k_RGBA16)] = {
      DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R16G16B16A16_UNORM,
      DXGI_FORMAT_R16G16B16A16_SNORM};
  formats[size_t(HWTextureFormat::k_RGBA16_UNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA16_SNorm_via_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA16_UScaled_via_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA16_SScaled_via_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;

  formats[size_t(HWTextureFormat::k_R16_Float)] = DXGI_FORMAT_R16_FLOAT;

  formats[size_t(HWTextureFormat::k_RG16_Float)] = DXGI_FORMAT_R16G16_FLOAT;

  formats[size_t(HWTextureFormat::k_RGBA16_Float)] =
      DXGI_FORMAT_R16G16B16A16_FLOAT;

  formats[size_t(HWTextureFormat::k_R32_UNorm_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;
  formats[size_t(HWTextureFormat::k_R32_SNorm_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;
  formats[size_t(HWTextureFormat::k_R32_UScaled_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;
  formats[size_t(HWTextureFormat::k_R32_SScaled_via_R32_Float)] =
      DXGI_FORMAT_R32_FLOAT;

  formats[size_t(HWTextureFormat::k_RG32_UNorm_via_RG32_Float)] =
      DXGI_FORMAT_R32G32_FLOAT;
  formats[size_t(HWTextureFormat::k_RG32_SNorm_via_RG32_Float)] =
      DXGI_FORMAT_R32G32_FLOAT;
  formats[size_t(HWTextureFormat::k_RG32_UScaled_via_RG32_Float)] =
      DXGI_FORMAT_R32G32_FLOAT;
  formats[size_t(HWTextureFormat::k_RG32_SScaled_via_RG32_Float)] =
      DXGI_FORMAT_R32G32_FLOAT;

  formats[size_t(HWTextureFormat::k_RGBA32_UNorm_via_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA32_SNorm_via_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA32_UScaled_via_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;
  formats[size_t(HWTextureFormat::k_RGBA32_SScaled_via_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;

  formats[size_t(HWTextureFormat::k_R32_Float)] = DXGI_FORMAT_R32_FLOAT;

  formats[size_t(HWTextureFormat::k_RG32_Float)] = DXGI_FORMAT_R32G32_FLOAT;

  formats[size_t(HWTextureFormat::k_RGBA32_Float)] =
      DXGI_FORMAT_R32G32B32A32_FLOAT;

  return formats;
}

}  // namespace wgf
}  // namespace hw
}  // namespace gpu
}  // namespace xe
