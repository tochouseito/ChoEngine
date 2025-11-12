#include "pch.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Core/LogAssert.h"
#include "include/Utility/TString.h"

[[nodiscard]]
bool Theatria::Graphics::RenderDevice::Initialize(bool enableDebugLayer)
{
    if (!CreateDXGIFactory(enableDebugLayer)) { return false; }
    if (!CreateDevice()) { return false; }
    CheckD3D12Options();
    return true;
}

/// @brief DXGIファクトリーの生成
/// @param enableDebugLayer
[[nodiscard]]
bool Theatria::Graphics::RenderDevice::CreateDXGIFactory([[maybe_unused]] bool enableDebugLayer)
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
    return true;
}

/// @brief デバイスの生成
[[nodiscard]]
bool Theatria::Graphics::RenderDevice::CreateDevice()
{
    HRESULT hr;

    // 使用するアダプタ用の変数。最初にNullptrを入れておく
    Microsoft::WRL::ComPtr < IDXGIAdapter4> useAdapter = nullptr;

    // 良い順にアダプタを頼む
    for (UINT i = 0; m_DXGIFactory->EnumAdapterByGpuPreference(i,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
        DXGI_ERROR_NOT_FOUND; ++i)
    {

        // アダプターの情報を取得する
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);

        // 取得できないのは一大事
        if (!Core::LogAssert::Verify(hr, "RenderDevice", "Failed to get adapter description."))
        {
            return false;
        }

        // ソフトウェアアダプタでなければ
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
        {
            // 採用したアダプタの情報をログに出力。wstringの方なので注意
            std::wstring wmsg = std::format(L"Use Adapter:{}\n", adapterDesc.Description);
            std::string smsg = Utility::ToUTF8(wmsg);
            Core::LogAssert::Log(
                std::source_location::current(), Core::LogAssert::SinkKind::Console,
                Core::LogAssert::LogLevel::Info, "RenderDevice",
                smsg);
            break;
        }
        // ソフトウェアアダプタの場合は見なかったことにする
        useAdapter = nullptr;
    }
    // 適切なアダプタが見つからなかったので起動できない
    if (!Core::LogAssert::Verify(useAdapter != nullptr, "RenderDevice", "Failed to find a suitable GPU adapter."))
    {
        return false;
    }

    // 機能レベルとログ出力用の文字列
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };
    const char* featureLevelStrings[] = {
        "12.2",
        "12.1",
        "12.0"
    };

    // 高い順に生成できるか試していく
    for (size_t i = 0; i < _countof(featureLevels); ++i)
    {

        // 採用したアダプターでデバイスを生成
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&m_Device));

        // 指定した機能レベルでデバイスが生成できたか確認
        if (SUCCEEDED(hr))
        {
            // 生成できたのでログ出力を行ってループを抜ける
            Core::LogAssert::Log(
                std::source_location::current(), Core::LogAssert::SinkKind::Console,
                Core::LogAssert::LogLevel::Info, "RenderDevice",
                "Create D3D12 Device : {}",
                featureLevelStrings[i]);
            break;
        }
    }
    // デバイスの生成がうまくいかなかったので起動できない
    if(!Core::LogAssert::Verify(m_Device != nullptr, "RenderDevice", "Failed to create D3D12 Device."))
    {
        return false;
    }

    // 初期化完了ログ
    Core::LogAssert::Log(
        std::source_location::current(), Core::LogAssert::SinkKind::Console,
        Core::LogAssert::LogLevel::Info, "RenderDevice",
        "Complete create D3D12Device!!!");

#ifdef _DEBUG
    ComPtr<ID3D12InfoQueue> infoQueue;
    // フィルタリングを一時的に無効化してみる

    if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
    {
        // ヤバいエラー時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

        // エラー時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

        // 警告時に止まる
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        // 抑制するメッセージのID
        D3D12_MESSAGE_ID denyIds[] = {

            // Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
            // https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
            D3D12_MESSAGE_ID_GPU_BASED_VALIDATION_RESOURCE_STATE_IMPRECISE // = 1044 相当
        };

        // 抑制するレベル
        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        // 指定したメッセージの表示を抑制する
        infoQueue->PushStorageFilter(&filter);
    }
#endif // DEBUG
    return true;
}

/// @brief 各サポートチェック
void Theatria::Graphics::RenderDevice::CheckD3D12Options() noexcept
{
    // D3D12Options
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_Options, sizeof(m_Options))))
    {
    }
    // D3D12Options1
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &m_Options1, sizeof(m_Options1))))
    {
    }
    // D3D12Options2
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &m_Options2, sizeof(m_Options2))))
    {
    }
    // D3D12Options3
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &m_Options3, sizeof(m_Options3))))
    {
    }
    // D3D12Options4
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &m_Options4, sizeof(m_Options4))))
    {
    }
    // D3D12Options5
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &m_Options5, sizeof(m_Options5))))
    {
    }
    // D3D12Options6
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &m_Options6, sizeof(m_Options6))))
    {
    }
    // D3D12Options7
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &m_Options7, sizeof(m_Options7))))
    {
    }
    // D3D12Options8
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &m_Options8, sizeof(m_Options8))))
    {
    }
    // D3D12Options9
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &m_Options9, sizeof(m_Options9))))
    {
    }
    // D3D12Options10
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &m_Options10, sizeof(m_Options10))))
    {
    }
    // D3D12Options11
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &m_Options11, sizeof(m_Options11))))
    {
    }
    // D3D12Options12
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &m_Options12, sizeof(m_Options12))))
    {
    }
    // D3D12Options13
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &m_Options13, sizeof(m_Options13))))
    {
    }
    // D3D12Options14
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &m_Options14, sizeof(m_Options14))))
    {
    }
    // D3D12Options15
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS15, &m_Options15, sizeof(m_Options15))))
    {
    }
    // D3D12Options16
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &m_Options16, sizeof(m_Options16))))
    {
    }
    // D3D12Options17
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS17, &m_Options17, sizeof(m_Options17))))
    {
    }
    // D3D12Options18
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &m_Options18, sizeof(m_Options18))))
    {
    }
    // D3D12Options19
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &m_Options19, sizeof(m_Options19))))
    {
    }
    // D3D12Options20
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS20, &m_Options20, sizeof(m_Options20))))
    {
    }
    // D3D12Options21
    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &m_Options21, sizeof(m_Options21))))
    {
    }


    //// シェーダモデルをチェック.
    //{
    //	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
    //	if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
    //	{
    //		Log::Write(LogLevel::Info, "Shader Model 6.5 is supported");
    //	} else
    //	{
    //		Log::Write(LogLevel::Assert, "Shader Model 6.5 is not supported");
    //	}
    //}

    //// メッシュシェーダをサポートしているかどうかチェック.
    //{
    //	D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
    //	if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features))))
    //	{
    //		std::cout << "Mesh Shader: ";
    //		if (features.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    //		{
    //			// メッシュシェーダのティアを表示
    //			Log::Write(LogLevel::Info, std::format("Mesh Shader Tier: {}", (int)features.MeshShaderTier));
    //			std::cout << "Supported (Tier " << (int)features.MeshShaderTier << ")\n";
    //		} else
    //		{
    //			Log::Write(LogLevel::Assert, "Mesh Shader: Not Supported");
    //		}
    //	}
    //}

    // ExecuteIndirect (Resource Binding Tierを間接的に利用可否チェック)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
        {
            if (options.ResourceBindingTier != D3D12_RESOURCE_BINDING_TIER_1)
            {
                //Log::Write(LogLevel::Info, std::format("Resource Binding Tier: {}", (int)options.ResourceBindingTier));
            }
            else
            {
                //Log::Write(LogLevel::Assert, "Resource Binding Tier: Not Supported");
            }
        }
    }

    // Raytracing
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5))))
        {
            std::cout << "Raytracing (DXR): ";
            if (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
            {
                //Log::Write(LogLevel::Info, std::format("Raytracing Tier: {}", (int)options5.RaytracingTier));
            }
            else
            {
                //Log::Write(LogLevel::Assert, "Raytracing: Not Supported");
            }
        }
    }

    // Variable Rate Shading (VRS)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
        if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6))))
        {
            std::cout << "Variable Rate Shading (VRS): ";
            if (options6.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
            {
                //Log::Write(LogLevel::Info, std::format("Variable Rate Shading Tier: {}", (int)options6.VariableShadingRateTier));
            }
            else
            {
               // Log::Write(LogLevel::Assert, "Variable Rate Shading (VRS): Not Supported");
            }
        }
    }

    // Sampler Feedback
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7))))
        {
            if (options7.SamplerFeedbackTier != D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED)
            {
                //Log::Write(LogLevel::Info, std::format("Sampler Feedback Tier: {}", (int)options7.SamplerFeedbackTier));
            }
            else
            {
                //Log::Write(LogLevel::Assert, "Sampler Feedback: Not Supported");
            }
        }
    }

    // 16拡張機能
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
        if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))))
        {
            // GPU_UPLOAD_HEAPサポートチェック
            if (options16.GPUUploadHeapSupported != 0)
            {
                //Log::Write(LogLevel::Info, std::format("GPU Virtual Address Support: {}", (int)options16.GPUUploadHeapSupported));
            }
            else
            {
                //Log::Write(LogLevel::Info, "GPU Virtual Address Support: Not Supported");
            }

            // Dynamic Depth Biasサポートチェック
            if (options16.DynamicDepthBiasSupported != 0)
            {
                //Log::Write(LogLevel::Info, std::format("Dynamic Depth Bias Support: {}", (int)options16.DynamicDepthBiasSupported));
            }
            else
            {
                //Log::Write(LogLevel::Info, "Dynamic Depth Bias Support: Not Supported");
            }
        }
    }

    //// Neural Network Acceleration (NNA)
    //{
    //	D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9 = {};
    //	if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9))))
    //	{
    //		if (options9.NNAccelerationTier != D3D12_NN_ACCELERATION_TIER_NOT_SUPPORTED)
    //		{
    //			Log::Write(LogLevel::Info, std::format("NN Acceleration Tier: {}", (int)options9.NNAccelerationTier));
    //		} else
    //		{
    //			Log::Write(LogLevel::Assert, "NN Acceleration: Not Supported");
    //		}
    //	}
    //}

    // Neural Shader
    {
        //D3D12_FEATURE_DATA_D3D12_OPTIONS15
    }
}
