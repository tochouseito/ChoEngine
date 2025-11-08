#include "pch.h"
#include "include/Graphics/RenderDevice.h"

[[nodiscard]]
bool Theatria::Graphics::RenderDevice::Initialize(bool enableDebugLayer)
{
    CreateDXGIFactory(enableDebugLayer);
    CreateDevice();
    return true;
}

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
void Theatria::Graphics::RenderDevice::CreateDevice()
{
//    HRESULT hr;
//
//    // 使用するアダプタ用の変数。最初にNullptrを入れておく
//    Microsoft::WRL::ComPtr < IDXGIAdapter4> useAdapter = nullptr;
//
//    // 良い順にアダプタを頼む
//    for (UINT i = 0; m_DXGIFactory->EnumAdapterByGpuPreference(i,
//        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
//        DXGI_ERROR_NOT_FOUND; ++i)
//    {
//
//        // アダプターの情報を取得する
//        DXGI_ADAPTER_DESC3 adapterDesc{};
//        hr = useAdapter->GetDesc3(&adapterDesc);
//
//        // 取得できないのは一大事
//        Log::Write(LogLevel::Assert, "Adapter description", hr);
//
//        // ソフトウェアアダプタでなければ
//        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
//        {
//            // 採用したアダプタの情報をログに出力。wstringの方なので注意
//            Log::Write(LogLevel::Info, ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
//            break;
//        }
//        // ソフトウェアアダプタの場合は見なかったことにする
//        useAdapter = nullptr;
//    }
//    // 適切なアダプタが見つからなかったので起動できない
//    assert(useAdapter != nullptr);
//
//    // 機能レベルとログ出力用の文字列
//    D3D_FEATURE_LEVEL featureLevels[] = {
//        D3D_FEATURE_LEVEL_12_2,
//        D3D_FEATURE_LEVEL_12_1,
//        D3D_FEATURE_LEVEL_12_0,
//    };
//    const char* featureLevelStrings[] = {
//        "12.2",
//        "12.1",
//        "12.0"
//    };
//
//    // 高い順に生成できるか試していく
//    for (size_t i = 0; i < _countof(featureLevels); ++i)
//    {
//
//        // 採用したアダプターでデバイスを生成
//        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&m_Device));
//
//        // 指定した機能レベルでデバイスが生成できたか確認
//        if (SUCCEEDED(hr))
//        {
//
//            // 生成できたのでログ出力を行ってループを抜ける
//            Log::Write(LogLevel::Info, std::format("Create D3D12 Device : {}", featureLevelStrings[i]));
//            break;
//        }
//    }
//    // デバイスの生成がうまくいかなかったので起動できない
//    if (!m_Device)
//    {
//        Log::Write(LogLevel::Assert, "Failed to create D3D12 Device");
//    }
//
//    // 初期化完了ログ
//    Log::Write(LogLevel::Info, "Complete create D3D12Device!!!");
//
//    // デバイスの機能をチェック
//    CheckD3D12Features();
//
//#ifdef _DEBUG
//    ComPtr<ID3D12InfoQueue> infoQueue;
//    // フィルタリングを一時的に無効化してみる
//
//    if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
//    {
//        // ヤバいエラー時に止まる
//        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
//
//        // エラー時に止まる
//        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
//
//        // 警告時に止まる
//        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
//
//        // 抑制するメッセージのID
//        D3D12_MESSAGE_ID denyIds[] = {
//
//            // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
//            // https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
//            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
//            D3D12_MESSAGE_ID_GPU_BASED_VALIDATION_RESOURCE_STATE_IMPRECISE // = 1044 相当
//        };
//
//        // 抑制するレベル
//        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
//        D3D12_INFO_QUEUE_FILTER filter{};
//        filter.DenyList.NumIDs = _countof(denyIds);
//        filter.DenyList.pIDList = denyIds;
//        filter.DenyList.NumSeverities = _countof(severities);
//        filter.DenyList.pSeverityList = severities;
//
//        // 指定したメッセージの表示を抑制する
//        infoQueue->PushStorageFilter(&filter);
//    }
//#endif // DEBUG
}

/// @brief 各サポートチェック
void Theatria::Graphics::RenderDevice::CheckD3D12Options() {}
