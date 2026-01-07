/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_WGF_WGF_TEXTURE_FORMAT_H_
#define XENIA_GPU_HW_WGF_WGF_TEXTURE_FORMAT_H_

#include <array>
#include <cstddef>

#include "third_party/DirectX-Headers/include/directx/dxgiformat.h"

namespace xe {
namespace gpu {
namespace hw {
namespace wgf {

struct WGFTextureFormat {
  DXGI_FORMAT resource = DXGI_FORMAT_UNKNOWN;
  DXGI_FORMAT view_unsigned = DXGI_FORMAT_UNKNOWN;
  DXGI_FORMAT view_signed = DXGI_FORMAT_UNKNOWN;

  constexpr WGFTextureFormat() = default;

  // If the host format is used specifically for either unsigned or signed guest
  // texture data, still initialize both view formats to the same value for
  // simplicity.
  constexpr WGFTextureFormat(const DXGI_FORMAT format)
      : resource(format), view_unsigned(format), view_signed(format) {}

  constexpr WGFTextureFormat(const DXGI_FORMAT resource,
                             const DXGI_FORMAT view_unsigned,
                             const DXGI_FORMAT view_signed)
      : resource(resource),
        view_unsigned(view_unsigned),
        view_signed(view_signed) {}
};

// Returns direct mappings of `HWTextureFormat` to `DXGI_FORMAT` resource and
// view format.
//
// In the table, if the contents of the same data in the host format can be
// interpreted differently as unsigned and signed, the resource format is
// typeless, and the unsigned and signed view formats are different. Otherwise
// the resource and both view formats are initialized to the same value.
//
// Some formats may be unsupported depending on the Direct3D runtime and driver
// feature support:
// - B5G6R5_UNORM, B5G5R5A1_UNORM, B4G4R4A4_UNORM require Direct3D 11.1+ runtime
//   and driver, although the enumeration values can be passed safely to
//   ID3D11Device::CheckFormatSupport.
// - A4B4G4R4_UNORM was added to Direct3D 12 among VulkanOn12 features, and its
//   support must be queried via `ID3D12Device::CheckFeatureSupport` for
//   `D3D12_FEATURE_FORMAT_SUPPORT` (see the Dozen driver in Mesa for an
//   example).
//
// Format in this table that are supported by the device can be used in the
// following ways (see "Format support for Direct3D Feature Level hardware" on
// MSDN):
// - Sampling with linear or anisotropic filtering is supported for all formats
//   on Direct3D feature levels 10_1+. On 10_0, filtering is optional for 32-bit
//   floating-point formats.
// - Render target views are unsupported for block formats and optional for
//   B5G5R5A1_UNORM, B4G4R4A4_UNORM and A4B4G4R4_UNORM (but for B5G6R5_UNORM,
//   support is required if the runtime and the driver support the format
//   itself).
// - Typed UAV stores are unsupported for block formats and optional for
//   B5G6R5_UNORM, B5G5R5A1_UNORM, B4G4R4A4_UNORM and A4B4G4R4_UNORM.
// - On Direct3D 10.1+, copying is supported between 4x4 block-compressed
//   formats and 16-bit or 32-bit integer textures representing the block data.
//   See the "Format Conversion Using Direct3D 10.1" section of "Block
//   Compression" on MSDN.
//
// To check if optional format features are supported,
// `ID3D11Device::CheckFormatSupport`, or `ID3D12Device::CheckFeatureSupport`
// for `D3D12_FEATURE_FORMAT_SUPPORT`, can be used.
//
// Note that A4B4G4R4_UNORM is included in the table for completeness, but its
// use cases in Xenia are covered by the more widely supported B4G4R4A4_UNORM,
// so it can be ignored.
std::array<WGFTextureFormat, size_t(HWTextureFormat::kCount)>
CreateWGFTextureFormatTable();

}  // namespace wgf
}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_WGF_WGF_TEXTURE_FORMAT_H_
