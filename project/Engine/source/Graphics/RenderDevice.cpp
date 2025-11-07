#include "pch.h"
#include "include/Graphics/RenderDevice.h"

/// @brief DXGIファクトリーの生成
/// @param enableDebugLayer 
void Theatria::Graphics::RenderDevice::CreateDXGIFactory(bool enableDebugLayer)
{
#ifdef _DEBUG
    /*
    [ INITIALIZATION MESSAGE #1016: CREATEDEVICE_DEBUG_LAYER_STARTUP_OPTIONS]
    Debug時のみの警告なため無視
    */
    ComPtr<ID3D12Debug6> debugController;
    if (enableDebugLayer)
    {
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            // デバッグレイヤーを有効化する
            debugController->EnableDebugLayer();

            // さらにGPU側でもチェックを行うようにする
            debugController->SetEnableGPUBasedValidation(TRUE);
        }
    }
    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> deviceRemoved;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&deviceRemoved))))
    {
        deviceRemoved->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        deviceRemoved->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    }
#endif
    // DXGIファクトリーの生成
    HRESULT hr;
    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_DXGIFactory));
}

/// @brief デバイスの生成
void Theatria::Graphics::RenderDevice::CreateDevice() {}

/// @brief 各サポートチェック
void Theatria::Graphics::RenderDevice::CheckD3D12Options() {}
