/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_D3D12_D3D12_API_H_
#define XENIA_UI_D3D12_D3D12_API_H_

// Must be included before D3D and DXGI for things like NOMINMAX.
#include "xenia/base/platform_win.h"

// Include the Direct3D headers from DirectX-Headers before the system DirectX
// headers, so that deeper includes like dxgiformats.h are up to date.
#include "third_party/DirectX-Headers/include/directx/d3d12.h"
#include "third_party/DirectX-Headers/include/directx/d3d12sdklayers.h"

#include <DXProgrammableCapture.h>
#include <d3dcompiler.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>
// For Microsoft::WRL::ComPtr.
#include <wrl/client.h>

#include "third_party/DirectXShaderCompiler/include/dxc/dxcapi.h"
#include "third_party/DirectXShaderCompiler/projects/dxilconv/include/DxbcConverter.h"

#define XELOGD3D XELOGI

#endif  // XENIA_UI_D3D12_D3D12_API_H_
