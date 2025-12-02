#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // ヘッダーから余計な情報を除外
#endif // !WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdint.h>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <array>
#include <functional>
#include <optional>
#include <span>
#include <bit>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

// Test
#include <d3d11.h>
#include <d3d11on12.h>
#include <d2d1_3.h>
#include <dwrite_3.h>

// なぜか定義されていないので追加
#ifndef D3D12_GPU_VIRTUAL_ADDRESS_NULL
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#endif

#ifndef PixelFormat
#define PixelFormat (DXGI_FORMAT_R8G8B8A8_UNORM)
#endif

static const float kClearColor[] = { 0.1f,0.25f,0.5f,1.0f };

#include <wrl.h>
using Microsoft::WRL::ComPtr;
#include "Core/ChoLog/ChoLog.h"
using namespace theatria;

// FreeListContainer
#include "Core/Utility/FArray.h"
#include "Core/Utility/FVector.h"

void DumpDred(ID3D12Device* device, HRESULT reason);
