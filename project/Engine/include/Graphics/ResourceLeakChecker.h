#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#ifdef _DEBUG
#include <d3d12sdklayers.h>
#endif

namespace Theatria::Graphics
{
    /// @brief リソースリークチェッカー
    class ResourceLeakChecker
    {
    public:
        /// @brief デストラクタ
        ~ResourceLeakChecker();
    };

    inline void SetDXGIName([[maybe_unused]] IDXGIObject* obj, [[maybe_unused]] const wchar_t* name = L"Theatria")
    {
#ifdef _DEBUG
        if (obj)
        {
            obj->SetPrivateData(
                WKPDID_D3DDebugObjectName,
                static_cast<UINT>((wcslen(name) + 1) * sizeof(wchar_t)),
                name);
        }
#endif
    }

    inline void SetD3D12Name([[maybe_unused]] ID3D12Object* obj, [[maybe_unused]] const wchar_t* name = L"Theatria")
    {
#ifdef _DEBUG
        if (obj)
        {
            obj->SetName(name);
        }
#endif
    }
}

