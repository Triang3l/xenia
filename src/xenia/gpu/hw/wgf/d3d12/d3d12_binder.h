/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2026 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_GPU_HW_WGF_D3D12_D3D12_BINDER_H_
#define XENIA_GPU_HW_WGF_D3D12_D3D12_BINDER_H_

#include <cstdint>

#include "xenia/gpu/texture_address.h"
#include "xenia/ui/d3d12/d3d12_api.h"

namespace xe {
namespace gpu {
namespace hw {
namespace wgf {
namespace d3d12 {

class D3D12Binder {
 public:
  // Common resource views placed in the beginning of shader-visible view heaps.
  // Don't make assumptions about their indices having specific values relative
  // to each other unless otherwise specified, declare each in the root
  // signature separately.
  enum class HeapStartView {
    kGlobalMemoryRawSRV,
    kGlobalMemoryRawUAV,

    kEDRAMRawUAV,

    kCount,
  };

  enum class CommonRootParameter {
    // Ordered roughly based on change frequency and the likelihood of being
    // accessed in the shaders, from high to low.
    kVertexFloatConstants,  // Root CBV.
    kPixelFloatConstants,   // Root CBV.
    kFetchConstants,        // Root CBV.
    kEmulationConstants,    // Root CBV.
    kBoolLoopConstants,     // Root CBV.
    // Global memory, EDRAM, and the global resource binding table if used.
    kResourceHeapStart,  // Table.

    kCount,
  };

  D3D12Binder(const D3D12Binder&) = delete;
  D3D12Binder& operator=(const D3D12Binder&) = delete;
  D3D12Binder(D3D12Binder&&) = delete;
  D3D12Binder& operator=(D3D12Binder&&) = delete;

  virtual ~D3D12Binder() = default;

  virtual bool GlobalBindingsTablesUsed() const = 0;

 protected:
  // Don't use sampler `D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE - 1` (2047)
  // because sampling with it is known to have been returning invalid values on
  // NVIDIA (tested on GeForce GTX 1070 on the driver 440.97 on Windows 10 1803)
  // if there's another shader-visible sampler descriptor containing 8 or less
  // samplers (such as the presentation and UI sampler heap in Xenia) on the
  // device. Also leave some space for graphics debuggers.
  static constexpr uint32_t kSamplerHeapSize = 2000;
  static_assert(kSamplerHeapSize <= D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);

  explicit D3D12Binder() = default;
};

// For binding resource views and samplers to translated shaders via tables of
// consecutive descriptors.
// Works on any Direct3D 12 device.
// Has generally higher CPU performance costs due to descriptor copying, but may
// be more convenient for debugging.
class D3D12BinderViaLocalTables : public D3D12Binder {
 public:
  struct RootSignatureKey {
   private:
    // TODO(Triang3l): Handle the texture and sampler count limits in shader
    // translation, along with the binding tier limit.
    // In the most dynamic case, 32 textures with 5 dimensionalities (1D, wide
    // 1D, 2D, 3D, cube) and 3 data representations (unsigned, signed, border
    // color fraction).
    static constexpr unsigned int kTextureCountBits = 9;
    // The maximum number of samplers in a shader-visible descriptor heap is
    // 2048, but the sampler 2047 isn't used by the binder anyway.
    static constexpr unsigned int kSamplerCountBits = 11;

    uint64_t key_ = 0;
  };

  bool GlobalBindingsTablesUsed() const override { return false; }

 private:
  // For addressing simplicity (can just take the #X part of an R#X register on
  // x86 for the index within the heap), and always a multiple of the Windows
  // allocation granularity regardless of the actual descriptor size.
  static constexpr uint32_t kViewCPUHeapSize = uint32_t(1) << 16;

  // Roughly 1/4 of the maximum shader-visible view heap size on resource
  // binding tier 1 (10^6) and on various generations of Intel and NVIDIA GPUs
  // (2^20).
  // Because Xenia allocates descriptors in shader-visible heaps linearly,
  // switching to a new heap if there's no free space, the heap will be switched
  // periodically. Usually, unless the emulator needs very many descriptors in a
  // frame for the game, there will be 2 resource view heaps. With this amount,
  // 2 heaps still stay within the limit, and depending on the driver and the
  // hardware, the driver potentially may optimize them both into 1 hardware
  // heap to avoid the pipeline stall that may be needed for an actual hardware
  // heap change.
  static constexpr uint32_t kViewGPUHeapSize = uint32_t(1) << 18;
  static_assert(kViewGPUHeapSize <=
                D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1);
};

// For binding resource views and samplers to translated shaders via indexing
// into a global descriptor table.
// Uses one heap for views, but switches to a new sampler heap in case of
// overflow.
// Requires the device to support resource binding tier 2.
// Lower CPU overhead as descriptor contents are written generally once, but
// adds the need to gather accessed resources or to perform the lookup manually
// to see the textures while debugging.
class D3D12BinderViaGlobalTables : public D3D12Binder {
 public:
  enum class RootParameter {
    kPixelTexturesAndSamplers = CommonRootParameter::kCount,  // Root CBV.
    kVertexTexturesAndSamplers,                               // Root CBV.
    kSamplerHeap,                                             // Table.

    kCount,
  };

  bool GlobalBindingsTablesUsed() const override { return true; }

 private:
  // Provide enough space - 256 * 1024 descriptors - for 2 views (such as
  // unsigned and signed) for single-page (4 KB) textures filling the entire
  // physical memory.
  // Roughly 1/4 of the maximum shader-visible view heap size.
  // Filling the entire Direct3D 12 resource binding tier 2 limit is not
  // recommended as graphics debuggers may need some descriptors for internal
  // use.
  // Games normally need a lot less, however. This was chosen to provide balance
  // between memory usage (8 MB for 256 * 1024 on AMD GCN) and edge case safety.
  static constexpr uint32_t kViewHeapSize =
      uint32_t(1) << (29 - texture_address::kPageBytesLog2 + 1);
  static_assert(kViewHeapSize <=
                D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
};

}  // namespace d3d12
}  // namespace wgf
}  // namespace hw
}  // namespace gpu
}  // namespace xe

#endif  // XENIA_GPU_HW_WGF_D3D12_D3D12_BINDER_H_
