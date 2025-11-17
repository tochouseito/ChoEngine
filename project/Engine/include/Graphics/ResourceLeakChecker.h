#pragma once
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

    inline void SetDXGIName(IDXGIObject* obj, const wchar_t* name = L"Theatria")
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

    inline void SetD3D12Name(ID3D12Object* obj, const wchar_t* name = L"Theatria")
    {
#ifdef _DEBUG
        if (obj)
        {
            obj->SetName(name);
        }
#endif
    }
}

