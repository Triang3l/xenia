/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/gpu/hw/wgf/d3d12/d3d12_texture.h"

#include <cstddef>

namespace xe {
namespace gpu {
namespace hw {
namespace wgf {
namespace d3d12 {

HWTextureFormatGuestFormatTable D3D12TextureProvider::CreateGuestFormatTable(
    const xenos::TextureSign signedness, const bool number_format_scaled) {
  HWTextureFormatGuestFormatTable formats;
  formats.fill(HWTextureFormat::kInvalid);

  // Biasing or degamma is performed after sampling UNorm on the host.
  const bool is_signed = signedness == xenos::TextureSign::kSigned;

  formats[size_t(xenos::TextureFormat::k_1_REVERSE)] =
      HWTextureFormat::k_R1_Reverse_via_R8_UNorm;

  formats[size_t(xenos::TextureFormat::k_1)] =
      HWTextureFormat::k_R1_via_R8_UNorm;

  formats[size_t(xenos::TextureFormat::k_8)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_R8_SScaled_via_R16_Float
                       : HWTextureFormat::k_R8_UScaled_via_R16_Float)
          : HWTextureFormat::k_R8;

  formats[size_t(xenos::TextureFormat::k_1_5_5_5)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RGB5A1_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_RGB5A1_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_RGB5A1_SNorm_via_RGBA8
                       : HWTextureFormat::k_RGB5A1_UNorm_via_BGR5A1_Packed);

  formats[size_t(xenos::TextureFormat::k_5_6_5)] =
      number_format_scaled
          ? (is_signed
                 ? HWTextureFormat::k_R5G6B5_SScaled_via_RGBA16_Float
                 : HWTextureFormat::k_R5G6B5_UScaled_via_RG11B10_Packed_Float)
          : (is_signed ? HWTextureFormat::k_R5G6B5_SNorm_via_RGBA8
                       : HWTextureFormat::k_R5G6B5_UNorm_via_B5G6R5_Packed);

  formats[size_t(xenos::TextureFormat::k_6_5_5)] =
      number_format_scaled
          ? (is_signed
                 ? HWTextureFormat::k_RG5B6_SScaled_via_RGBA16_Float
                 : HWTextureFormat::k_RG5B6_UScaled_via_RG11B10_Packed_Float)
          : (is_signed ? HWTextureFormat::k_RG5B6_SNorm_via_RGBA8
                       : HWTextureFormat::k_RG5B6_UNorm_via_B5G6R5_Packed_RBG);

  formats[size_t(xenos::TextureFormat::k_8_8_8_8)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RGBA8_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_RGBA8_UScaled_via_RGBA16_Float)
          : HWTextureFormat::k_RGBA8;

  formats[size_t(xenos::TextureFormat::k_2_10_10_10)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RGB10A2_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_RGB10A2_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_RGB10A2_SNorm_via_RGBA16_Float
                       : HWTextureFormat::k_RGB10A2_Packed);

  formats[size_t(xenos::TextureFormat::k_8_8)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RG8_SScaled_via_RG16_Float
                       : HWTextureFormat::k_RG8_UScaled_via_RG16_Float)
          : HWTextureFormat::k_RG8;

  // TODO(Triang3l): Non-2x1-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_Cr_Y1_Cb_Y0_REP)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_GB8_GR8_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_GB8_GR8_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_GB8_GR8_via_RGBA8
                       : HWTextureFormat::k_GB8_GR8_UNorm_via_GR8_GB8);

  // TODO(Triang3l): Non-2x1-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_Y1_Cr_Y0_Cb_REP)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_BG8_RG8_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_BG8_RG8_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_BG8_RG8_via_RGBA8
                       : HWTextureFormat::k_BG8_RG8_UNorm_via_RG8_BG8);

  formats[size_t(xenos::TextureFormat::k_4_4_4_4)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RGBA4_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_RGBA4_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_RGBA4_SNorm_via_RGBA8
                       : HWTextureFormat::k_RGBA4_UNorm_via_BGRA4_Packed);

  formats[size_t(xenos::TextureFormat::k_10_11_11)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RG11B10_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_RG11B10_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_RG11B10_SNorm_via_RGBA16
                       : HWTextureFormat::k_RG11B10_UNorm_via_RGBA16);

  formats[size_t(xenos::TextureFormat::k_11_11_10)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_R10GB11_SScaled_via_RGBA16_Float
                       : HWTextureFormat::k_R10GB11_UScaled_via_RGBA16_Float)
          : (is_signed ? HWTextureFormat::k_R10GB11_SNorm_via_RGBA16
                       : HWTextureFormat::k_R10GB11_UNorm_via_RGBA16);

  // TODO(Triang3l): Non-4x4-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_DXT1)] = HWTextureFormat::k_DXT1_UNorm;

  // TODO(Triang3l): Non-4x4-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_DXT2_3)] =
      HWTextureFormat::k_DXT3_UNorm;

  // TODO(Triang3l): Non-4x4-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_DXT4_5)] =
      HWTextureFormat::k_DXT5_UNorm;

  formats[size_t(xenos::TextureFormat::k_24_8)] =
      HWTextureFormat::k_X8R24_UNorm_via_R32_Float;

  formats[size_t(xenos::TextureFormat::k_24_8_FLOAT)] =
      HWTextureFormat::k_X8R24_Float_via_R32;

  formats[size_t(xenos::TextureFormat::k_16)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_R16_SScaled_via_R32_Float
                       : HWTextureFormat::k_R16_UScaled_via_R32_Float)
          : HWTextureFormat::k_R16;

  formats[size_t(xenos::TextureFormat::k_16_16)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RG16_SScaled_via_RG32_Float
                       : HWTextureFormat::k_RG16_UScaled_via_RG32_Float)
          : HWTextureFormat::k_RG16;

  formats[size_t(xenos::TextureFormat::k_16_16_16_16)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RGBA16_SScaled_via_RGBA32_Float
                       : HWTextureFormat::k_RGBA16_UScaled_via_RGBA32_Float)
          : HWTextureFormat::k_RGBA16;

  formats[size_t(xenos::TextureFormat::k_16_FLOAT)] =
      HWTextureFormat::k_R16_Float;

  formats[size_t(xenos::TextureFormat::k_16_16_FLOAT)] =
      HWTextureFormat::k_RG16_Float;

  formats[size_t(xenos::TextureFormat::k_16_16_16_16_FLOAT)] =
      HWTextureFormat::k_RGBA16_Float;

  formats[size_t(xenos::TextureFormat::k_32)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_R32_SScaled_via_R32_Float
                       : HWTextureFormat::k_R32_UScaled_via_R32_Float)
          : (is_signed ? HWTextureFormat::k_R32_SNorm_via_R32_Float
                       : HWTextureFormat::k_R32_UNorm_via_R32_Float);

  formats[size_t(xenos::TextureFormat::k_32_32)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RG32_SScaled_via_RG32_Float
                       : HWTextureFormat::k_RG32_UScaled_via_RG32_Float)
          : (is_signed ? HWTextureFormat::k_RG32_SNorm_via_RG32_Float
                       : HWTextureFormat::k_RG32_UNorm_via_RG32_Float);

  formats[size_t(xenos::TextureFormat::k_32_32_32_32)] =
      number_format_scaled
          ? (is_signed ? HWTextureFormat::k_RGBA32_SScaled_via_RGBA32_Float
                       : HWTextureFormat::k_RGBA32_UScaled_via_RGBA32_Float)
          : (is_signed ? HWTextureFormat::k_RGBA32_SNorm_via_RGBA32_Float
                       : HWTextureFormat::k_RGBA32_UNorm_via_RGBA32_Float);

  formats[size_t(xenos::TextureFormat::k_32_FLOAT)] =
      HWTextureFormat::k_R32_Float;

  formats[size_t(xenos::TextureFormat::k_32_32_FLOAT)] =
      HWTextureFormat::k_RG32_Float;

  formats[size_t(xenos::TextureFormat::k_32_32_32_32_FLOAT)] =
      HWTextureFormat::k_RGBA32_Float;

  // TODO(Triang3l): Non-4x4-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_DXN)] = HWTextureFormat::k_DXN_UNorm;

  formats[size_t(xenos::TextureFormat::k_DXT3A)] =
      is_signed ? HWTextureFormat::k_DXT3A_SNorm_via_R8
                : HWTextureFormat::k_DXT3A_UNorm_via_R8;

  // TODO(Triang3l): Non-4x4-aligned fallback (but not in this table).
  formats[size_t(xenos::TextureFormat::k_DXT5A)] =
      HWTextureFormat::k_DXT5A_UNorm;

  formats[size_t(xenos::TextureFormat::k_CTX1)] =
      HWTextureFormat::k_CTX1_UNorm_via_RG8;

  formats[size_t(xenos::TextureFormat::k_DXT3A_AS_1_1_1_1)] =
      HWTextureFormat::k_DXT3A_RGBA1_UNorm_via_BGRA4_Packed;

  // Storage formats matching other formats.

  formats[size_t(xenos::TextureFormat::k_8_A)] =
      formats[size_t(xenos::TextureFormat::k_8)];
  formats[size_t(xenos::TextureFormat::k_8_B)] =
      formats[size_t(xenos::TextureFormat::k_8)];

  formats[size_t(xenos::TextureFormat::k_8_8_8_8_A)] =
      formats[size_t(xenos::TextureFormat::k_8_8_8_8)];

  formats[size_t(xenos::TextureFormat::k_16_EXPAND)] =
      formats[size_t(xenos::TextureFormat::k_16_FLOAT)];
  formats[size_t(xenos::TextureFormat::k_16_16_EXPAND)] =
      formats[size_t(xenos::TextureFormat::k_16_16_FLOAT)];
  formats[size_t(xenos::TextureFormat::k_16_16_16_16_EXPAND)] =
      formats[size_t(xenos::TextureFormat::k_16_16_16_16_FLOAT)];

  formats[size_t(xenos::TextureFormat::k_8_8_8_8_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_8_8_8_8)];
  formats[size_t(xenos::TextureFormat::k_DXT1_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_DXT1)];
  formats[size_t(xenos::TextureFormat::k_DXT2_3_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_DXT2_3)];
  formats[size_t(xenos::TextureFormat::k_DXT4_5_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_DXT4_5)];
  formats[size_t(xenos::TextureFormat::k_2_10_10_10_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_2_10_10_10)];
  formats[size_t(xenos::TextureFormat::k_10_11_11_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_10_11_11)];
  formats[size_t(xenos::TextureFormat::k_11_11_10_AS_16_16_16_16)] =
      formats[size_t(xenos::TextureFormat::k_11_11_10)];

  return formats;
}

}  // namespace d3d12
}  // namespace wgf
}  // namespace hw
}  // namespace gpu
}  // namespace xe
