/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

// XE_GPU_HW_TEXTURE_FORMAT(name, bytes_per_host_element_log2,
//                          guest_blocks_decompressed)

// Host byte order:
// - Non-packed formats: first component in the lower bytes, last component in
//   the upper bytes, each component has the host endianness.
// - Packed formats: the whole packed element has the host endianness, first
//   component in the least significant bits, last component in the most
//   significant bits.

// When consecutive components have the same bit widths and number formats, the
// bit width is written after all those components. This is done to make the
// names more compact, and also to more clearly distinguish between R5G6B5 and
// RG5B6, and between RG11B10 and R10GB11.

// If a format has a swizzle at the end of its name, the first character is
// which host component is the guest red, the last character is which host
// component is the last guest component.
//
// For example, BRG means:
// - Guest R in host B.
// - Guest G in host R.
// - Guest B in host G.

// Integer (scaled because the Xenos ALU only support floating-point numbers)
// formats up to and including 11_UScaled and 12_SScaled can be represented by
// 16_Float exactly.

// "Direct3D" in the format support comments refers to Direct3D 10+ on the host
// specifically (not Direct3D 9 on the host, whose feature set is largely
// insufficient for Xenos emulation, or on the guest).

XE_GPU_HW_TEXTURE_FORMAT(k_R1_Reverse_via_R8_UNorm, 0, true)

XE_GPU_HW_TEXTURE_FORMAT(k_R1_via_R8_UNorm, 0, true)

XE_GPU_HW_TEXTURE_FORMAT(k_R8, 0, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R8_UScaled_via_R16_Float, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R8_SScaled_via_R16_Float, 1, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RG8, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG8_UScaled_via_RG16_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG8_SScaled_via_RG16_Float, 2, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA8_UScaled_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA8_SScaled_via_RGBA16_Float, 3, false)

// For the 4:2:2 subsampled formats, the left G is in the lower pair of bytes,
// and the right G is in the upper pair of bytes, on both the guest and, when
// still represented by a subsampled format, the host.

// Direct3D: N/A | Metal: mandatory | Vulkan: optional.
XE_GPU_HW_TEXTURE_FORMAT(k_GB8_GR8_UNorm, 2, false)
// Direct3D: mandatory | Metal: N/A | Vulkan: N/A.
XE_GPU_HW_TEXTURE_FORMAT(k_GB8_GR8_UNorm_via_GR8_GB8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_GB8_GR8_via_RGBA8, 2, true)
XE_GPU_HW_TEXTURE_FORMAT(k_GB8_GR8_UScaled_via_RGBA16_Float, 3, true)
XE_GPU_HW_TEXTURE_FORMAT(k_GB8_GR8_SScaled_via_RGBA16_Float, 3, true)

// Direct3D: N/A | Metal: mandatory | Vulkan: optional.
XE_GPU_HW_TEXTURE_FORMAT(k_BG8_RG8_UNorm, 2, false)
// Direct3D: mandatory | Metal: N/A | Vulkan: N/A.
XE_GPU_HW_TEXTURE_FORMAT(k_BG8_RG8_UNorm_via_RG8_BG8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_BG8_RG8_via_RGBA8, 2, true)
XE_GPU_HW_TEXTURE_FORMAT(k_BG8_RG8_UScaled_via_RGBA16_Float, 3, true)
XE_GPU_HW_TEXTURE_FORMAT(k_BG8_RG8_SScaled_via_RGBA16_Float, 3, true)

// Direct3D: 11.1+ | Metal: 4+ | Vulkan: mandatory.
XE_GPU_HW_TEXTURE_FORMAT(k_RGB5A1_UNorm_via_BGR5A1_Packed, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB5A1_UNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB5A1_SNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB5A1_UScaled_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB5A1_SScaled_via_RGBA16_Float, 3, false)

// Direct3D: 11.1+ | Metal: 4+ | Vulkan: mandatory.
XE_GPU_HW_TEXTURE_FORMAT(k_R5G6B5_UNorm_via_B5G6R5_Packed, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R5G6B5_UNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R5G6B5_SNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R5G6B5_UScaled_via_RG11B10_Packed_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R5G6B5_SScaled_via_RGBA16_Float, 3, false)

// RG5B6 with guest components laid out like G5B6R5 in host memory
// (rotate right by 5 bits to convert from the guest to the host).
// Direct3D: 11.1+ | Metal: 4+ | Vulkan: mandatory.
XE_GPU_HW_TEXTURE_FORMAT(k_RG5B6_UNorm_via_B5G6R5_Packed_RBG, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG5B6_UNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG5B6_SNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG5B6_UScaled_via_RG11B10_Packed_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG5B6_SScaled_via_RGBA16_Float, 3, false)

// Direct3D: 11.1+ | Metal: N/A | Vulkan: optional.
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_UNorm_via_BGRA4_Packed, 1, false)
// Direct3D: N/A | Metal: N/A | Vulkan: mandatory.
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_UNorm_via_ARGB4_Packed, 1, false)
// Direct3D: optional | Metal: 4+ | Vulkan: optional.
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_UNorm_via_ABGR4_Packed, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_UNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_SNorm_via_RGBA8, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_UScaled_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA4_SScaled_via_RGBA16_Float, 3, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RGB10A2_Packed, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB10A2_SNorm_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB10A2_UScaled_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGB10A2_SScaled_via_RGBA16_Float, 3, false)

// For 11_UNorm, conversion to 16_UNorm is preferred over 16_Float (even though
// it has 1 implicit and 10 explicit mantissa bits), because in 11_UNorm,
// 0b111'1111'1111 represents exactly 1.0, but in 16_Float, mantissa
// 0b1.11'1111'1111 and exponent -1 is 1 ULP less than 1.0.
// However, 16_UNorm is optional on Vulkan, so 16_Float conversion needs to be
// supported too.

XE_GPU_HW_TEXTURE_FORMAT(k_RG11B10_UNorm_via_RGBA16, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG11B10_UNorm_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG11B10_SNorm_via_RGBA16, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG11B10_SNorm_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG11B10_UScaled_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG11B10_SScaled_via_RGBA16_Float, 3, false)

XE_GPU_HW_TEXTURE_FORMAT(k_R10GB11_UNorm_via_RGBA16, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R10GB11_UNorm_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R10GB11_SNorm_via_RGBA16, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R10GB11_SNorm_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R10GB11_UScaled_via_RGBA16_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R10GB11_SScaled_via_RGBA16_Float, 3, false)

// DXT1-5, DXT5A and DXN (BC1-5) are mandatory in Direct3D, but not in Metal and
// Vulkan.

XE_GPU_HW_TEXTURE_FORMAT(k_DXT1_UNorm, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_DXT1_UNorm_via_RGBA8, 2, true)

XE_GPU_HW_TEXTURE_FORMAT(k_DXT3_UNorm, 4, false)
XE_GPU_HW_TEXTURE_FORMAT(k_DXT3_UNorm_via_RGBA8, 2, true)

XE_GPU_HW_TEXTURE_FORMAT(k_DXT5_UNorm, 4, false)
// 8 bits isn't enough to store distinct values for all 6 or 8 interpolation
// steps in a block in all cases in DXT4_5 alpha, but on the Xenos, DXT4_5 is
// decompressed into RGBA8 during texture fetch anyway, and BC3 on the host
// hardware may be decompressed similarly as well.
XE_GPU_HW_TEXTURE_FORMAT(k_DXT5_UNorm_via_RGBA8, 2, true)

// DXT5A and DXN preferably should be decoded with at least 16 bits of precision
// for the result to match the hardware behavior, and also because 8 bits isn't
// enough to store distinct values for all 6 or 8 interpolation steps in a block
// in all cases.
//
// Direct3D 10+ requires implementation to provide such precision for BC4 and
// BC5, however, Vulkan doesn't.
//
// However, DXN is used mostly for normal maps, and the difference between
// 16-bit and less precise (such as 8-bit, like often for DXT5) decoding is
// usually insignificant, so conversion to 8-bit may be reasonable as well.
//
// There are two situations where the conversion is necessary:
// - BC4-5 is completely unsupported by the host. S3TC-like compression (BC1-5)
//   is optional in Metal and Vulkan, and many mobile GPU architectures don't
//   implement it, so if BC4-5 is unsupported, likely so is BC1-3, and thus the
//   memory and bandwidth overhead is already high. In these cases, conversion
//   to 8 bits per component may be desirable.
// - BC4-5 is supported, but only for textures with dimensions divisible by 4x4.
//   This limitation is present in Direct3D, only relaxed in the VulkanOn12
//   extensions, however, some GPUs never received a driver update implementing
//   them. Conversion of textures that don't have their size aligned to 4x4 to
//   16 bits per component is desirable in this case for consistency with
//   hardware BC4-5 sampling.
//
// Note that 16-bit normalized format support is optional in Vulkan.

XE_GPU_HW_TEXTURE_FORMAT(k_DXT5A_UNorm, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_DXT5A_UNorm_via_R16, 1, true)
XE_GPU_HW_TEXTURE_FORMAT(k_DXT5A_UNorm_via_R8, 0, true)

XE_GPU_HW_TEXTURE_FORMAT(k_DXN_UNorm, 4, false)
XE_GPU_HW_TEXTURE_FORMAT(k_DXN_UNorm_via_RG16, 2, true)
XE_GPU_HW_TEXTURE_FORMAT(k_DXN_UNorm_via_RG8, 1, true)

XE_GPU_HW_TEXTURE_FORMAT(k_DXT3A_UNorm_via_R8, 0, true)
XE_GPU_HW_TEXTURE_FORMAT(k_DXT3A_SNorm_via_R8, 0, true)

// Direct3D: 11.1+ | Metal: N/A | Vulkan: optional.
XE_GPU_HW_TEXTURE_FORMAT(k_DXT3A_RGBA1_UNorm_via_BGRA4_Packed, 1, true)
// Direct3D: N/A | Metal: N/A | Vulkan: mandatory.
XE_GPU_HW_TEXTURE_FORMAT(k_DXT3A_RGBA1_UNorm_via_ARGB4_Packed, 1, true)
// Direct3D: optional | Metal: 4+ | Vulkan: optional.
XE_GPU_HW_TEXTURE_FORMAT(k_DXT3A_RGBA1_UNorm_via_ABGR4_Packed, 1, true)
XE_GPU_HW_TEXTURE_FORMAT(k_DXT3A_RGBA1_UNorm_via_RGBA8, 2, true)

XE_GPU_HW_TEXTURE_FORMAT(k_CTX1_UNorm_via_RG8, 1, true)

// TODO(Triang3l): Signed and integer DXT decompression when researched well
// enough.

// D24_UNorm shouldn't be used for this format on the host because depth
// framebuffer attachment writes are not needed for fetched textures, and for
// other uses on the host, it often has much more restricted capabilities than
// R32_Float. It also has very limited or no support on AMD GCN+, generally not
// available on Vulkan on those GPUs at all, and in Direct3D, it may be
// implicitly promoted to 32-bit floating-point, with CopyTextureRegion
// interpreting the buffer data as 32-bit floating-point rather than 24-bit
// unsigned normalized.
XE_GPU_HW_TEXTURE_FORMAT(k_X8R24_UNorm_via_R32_Float, 2, false)

XE_GPU_HW_TEXTURE_FORMAT(k_X8R24_Float_via_R32, 2, false)

// 16-bit normalized formats are filterable on the guest and mandatory in
// Direct3D and Metal, but optional in Vulkan, though supported almost
// ubiquitously without filtering there, and commonly with filtering.
// 32-bit floating-point is preferred over 16-bit floating-point as the
// fallback, even though filterability is mandatory for 16-bit floating-point,
// because the latter is lossy, and 16_Norm is sometimes used to pass raw bits
// on the Xenos (though that's usually done via vertex fetch, not texture fetch,
// like for copying texture data of arbitrary formats via vertex fetch and
// memory export in texture streaming in 41560817).

XE_GPU_HW_TEXTURE_FORMAT(k_R16, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R16_UNorm_via_R32_Float, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R16_SNorm_via_R32_Float, 1, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R16_UScaled_via_R32_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R16_SScaled_via_R32_Float, 2, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RG16, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG16_UNorm_via_RG32_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG16_SNorm_via_RG32_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG16_UScaled_via_RG32_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG16_SScaled_via_RG32_Float, 3, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RGBA16, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA16_UNorm_via_RGBA32_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA16_SNorm_via_RGBA32_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA16_UScaled_via_RGBA32_Float, 4, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA16_SScaled_via_RGBA32_Float, 4, false)

// Note that IEEE 754 16-bit floating-point on the host can't represent the
// largest exponent value available for normal numbers on the Xenos, instead
// using it for infinity and NaN.
// Exponent biasing in shaders may be used to represent the entire range, at the
// cost of one denormal bit.

XE_GPU_HW_TEXTURE_FORMAT(k_R16_Float, 1, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RG16_Float, 2, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RGBA16_Float, 3, false)

// 32-bit fixed-point to floating-point conversion is inherently lossy.

XE_GPU_HW_TEXTURE_FORMAT(k_R32_UNorm_via_R32_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R32_SNorm_via_R32_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R32_UScaled_via_R32_Float, 2, false)
XE_GPU_HW_TEXTURE_FORMAT(k_R32_SScaled_via_R32_Float, 2, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RG32_UNorm_via_RG32_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG32_SNorm_via_RG32_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG32_UScaled_via_RG32_Float, 3, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RG32_SScaled_via_RG32_Float, 3, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RGBA32_UNorm_via_RGBA32_Float, 4, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA32_SNorm_via_RGBA32_Float, 4, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA32_UScaled_via_RGBA32_Float, 4, false)
XE_GPU_HW_TEXTURE_FORMAT(k_RGBA32_SScaled_via_RGBA32_Float, 4, false)

XE_GPU_HW_TEXTURE_FORMAT(k_R32_Float, 2, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RG32_Float, 3, false)

XE_GPU_HW_TEXTURE_FORMAT(k_RGBA32_Float, 4, false)
