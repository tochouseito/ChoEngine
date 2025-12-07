#include "pch.h"
#include "include/Graphics/PipelineManager.h"
#include "include/Graphics/ShaderCompiler.h"
#include "include/Core/Parser.h"
#include "include/Core/LogAssert.h"
#include "config/engineConfig.h"
#include "include/Utility/TString.h"

void Theatria::Graphics::PipelineManager::CreateDefaultPipelines(ID3D12Device* device, ShaderCompiler* compiler)
{
    // 1) グラフィックスパイプラインの設定ファイルをパースして読み込み
    m_GraphicsPipelines = Theatria::Core::Parser::LoadGraphicsPipelines_ini();

    // 2) 各パイプラインの生成
    for (auto& setting : m_GraphicsPipelines)
    {
        // 設定からpsoの生成
        CreateGraphicsPipeline(device,setting, compiler);
    }
}

void Theatria::Graphics::PipelineManager::CreateGraphicsPipeline(ID3D12Device* device, GraphicsPipelineSettings& setting, ShaderCompiler* compiler)
{
    // ルートシグネチャの作成
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    // ルートパラメータの作成
    std::vector<D3D12_ROOT_PARAMETER> rootParms;// ルートパラメータ配列
    std::vector<D3D12_DESCRIPTOR_RANGE> renges;// ディスクリプタレンジ配列
    D3D12_DESCRIPTOR_RANGE texRenge{};// テクスチャ用ディスクリプタレンジ
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;// スタティックサンプラ配列
    std::vector<D3D12_INDIRECT_ARGUMENT_DESC> indirectArgs;// 間接描画用引数配列
    bool useTexBuf = false;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};// 入力レイアウト
    ComPtr<IDxcBlob> vsBlob = nullptr;
    ComPtr<IDxcBlob> psBlob = nullptr;
    // シェーダーのコンパイル
    // vs
    if (!setting.vs.empty())
    {
        ShaderCompileDesc vsDesc;
        vsDesc.entryPoint = L"VSMain";
        vsDesc.profile = L"vs_" + Graphics::ShaderProfileToWString(Config::Graphics::HighestShaderModel);
        vsDesc.filePath = Utility::ToUTF16(Config::FilePath::ShaderDirectory + setting.vs);
#ifndef NDEBUG
        vsDesc.debug = true;
#else
        vsDesc.debug = false;
#endif
        vsBlob = compiler->GetOrCompileShader(vsDesc);
        // リフレクション
        ComPtr<ID3D12ShaderReflection> vsRef = compiler->ReflectShader(vsBlob.Get());
        // シェーダーの詳細情報を取得
        D3D12_SHADER_DESC shaderDesc;
        vsRef->GetDesc(&shaderDesc);
        // リソースバインディング情報を取得
        GetReflectionRootParms(vsRef.Get(),
            shaderDesc,
            D3D12_SHADER_VISIBILITY_VERTEX,
            rootParms,
            staticSamplers,
            renges,
            texRenge,
            useTexBuf,
            indirectArgs);

        // InputLayoutの設定
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
        for (UINT i = 0; i < shaderDesc.InputParameters; ++i)
        {
            D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
            vsRef->GetInputParameterDesc(i, &paramDesc);
            D3D12_INPUT_ELEMENT_DESC elementDesc{};
            elementDesc.SemanticName = paramDesc.SemanticName;
            elementDesc.SemanticIndex = paramDesc.SemanticIndex;
            elementDesc.Format = compiler->GetDXGIFormat(paramDesc.ComponentType, paramDesc.Mask);
            elementDesc.InputSlot = 0;
            elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elementDesc.InstanceDataStepRate = 0;
            inputElementDescs.push_back(elementDesc);
        }

        inputLayoutDesc.pInputElementDescs = inputElementDescs.data();
        inputLayoutDesc.NumElements = static_cast<UINT>(inputElementDescs.size());
    }// vs

    // ps
    if (!setting.ps.empty())
    {
        ShaderCompileDesc psDesc;
        psDesc.entryPoint = L"PSMain";
        psDesc.profile = L"ps_" + Graphics::ShaderProfileToWString(Config::Graphics::HighestShaderModel);
        psDesc.filePath = Utility::ToUTF16(Config::FilePath::ShaderDirectory + setting.ps);
#ifndef NDEBUG
        psDesc.debug = true;
#else
        psDesc.debug = false;
#endif
        psBlob = compiler->GetOrCompileShader(psDesc);
        // リフレクション
        ComPtr<ID3D12ShaderReflection> psRef = compiler->ReflectShader(psBlob.Get());
        // シェーダーの詳細情報を取得
        D3D12_SHADER_DESC shaderDesc;
        psRef->GetDesc(&shaderDesc);
        // リソースバインディング情報を取得
        GetReflectionRootParms(psRef.Get(),
            shaderDesc,
            D3D12_SHADER_VISIBILITY_PIXEL,
            rootParms,
            staticSamplers,
            renges,
            texRenge,
            useTexBuf,
            indirectArgs);
    }

    // テクスチャバッファのルートパラメータを最後に追加
    if (useTexBuf)
    {
        D3D12_ROOT_PARAMETER rootParm{};
        rootParm.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParm.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        // ディスクリプタレンジの設定
        rootParm.DescriptorTable.NumDescriptorRanges = 1;
        rootParm.DescriptorTable.pDescriptorRanges = &texRenge;
        // ルートパラメータに追加
        rootParms.push_back(rootParm);
    }
    // 間接描画用引数のvbv,ibv,drawIndexedの追加
    {
        D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
        argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
        argDesc.VertexBuffer.Slot = 0;
        indirectArgs.push_back(argDesc);
        argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
        indirectArgs.push_back(argDesc);
        argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
        indirectArgs.push_back(argDesc);
    }

    // ルートシグネチャの設定
    rootSignatureDesc.pParameters = rootParms.data();
    rootSignatureDesc.NumParameters = static_cast<UINT>(rootParms.size());
    rootSignatureDesc.pStaticSamplers = staticSamplers.data();
    rootSignatureDesc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
    // ルートシグネチャの生成
    ComPtr<ID3DBlob> pSignature;
    ComPtr<ID3DBlob> pError;
    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &pSignature,
        &pError);
    Core::LogAssert::Check(hr, "PipelineManager", "Failed Serialize RootSignature!!");
    hr = device->CreateRootSignature(
        0,
        pSignature->GetBufferPointer(),
        pSignature->GetBufferSize(),
        IID_PPV_ARGS(&setting.rootSignature));
    Core::LogAssert::Check(hr, "PipelineManager", "Failed Create RootSignature!!");
    // コマンドシグネチャの生成
    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
    UINT byteStride = static_cast<UINT>(sizeof(BasicIndirectCommand));
    commandSignatureDesc.ByteStride = byteStride;
    commandSignatureDesc.NumArgumentDescs = static_cast<UINT>(indirectArgs.size());
    commandSignatureDesc.pArgumentDescs = indirectArgs.data();
    hr = device->CreateCommandSignature(
        &commandSignatureDesc,
        setting.rootSignature.Get(),
        IID_PPV_ARGS(&setting.commandSignature));
    Core::LogAssert::Check(hr, "PipelineManager", "Failed Create CommandSignature!!");
    // コマンド引数バッファの生成
    const UINT maxCmdCount = 256;
    const UINT64 bufferSize = static_cast<UINT64>(byteStride * maxCmdCount);
    setting.argsBuffer = std::make_unique<StructuredBuffer<BasicIndirectCommand>>();
    setting.argsBuffer->CreateBuffer(device, maxCmdCount);
    setting.argsBuffer->CreateUploadBuffer(device, maxCmdCount);

    // RasterizerStateの設定
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;// 塗りつぶし
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;// 裏面カリング

    // DepthStencilStateの設定
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = true;// 深度有効
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// 書き込み許可
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;// 近ければ描画

    // PipelineStateDescの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.pRootSignature = setting.rootSignature.Get();
    pipelineDesc.VS = { vsBlob->GetBufferPointer(),vsBlob->GetBufferSize() };
    pipelineDesc.PS = { psBlob->GetBufferPointer(),psBlob->GetBufferSize() };
    pipelineDesc.InputLayout = inputLayoutDesc;
    pipelineDesc.RasterizerState = rasterizerDesc;
    pipelineDesc.DepthStencilState = depthStencilDesc;
    pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = Config::Graphics::DefaultDXGIFormat;

    for (size_t i = 0; i < static_cast<size_t>(BlendMode::kCount); ++i)
    {
        // BlendStateの設定
        /*
        out.rgb = src.rgb * SrcBlend     + dst.rgb * DestBlend
        out.a   = src.a   * SrcBlendAlpha + dst.a   * DestBlendAlpha
        */
        D3D12_BLEND_DESC blendDesc = {};
        switch (static_cast<BlendMode>(i))
        {
        case BlendMode::None:// out = src * 1 + dst * 0 = src
            blendDesc.RenderTarget[0].BlendEnable = false;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            break;
        case BlendMode::Normal:// out.rgb = src.rgb * src.a + dst.rgb * (1 - src.a)
            blendDesc.RenderTarget[0].BlendEnable = true;

            // 色
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

            // アルファ（だいたい同じでいい）
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        case BlendMode::Add:// out.rgb = src.rgb * src.a + dst.rgb
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;  // src * alpha
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;        // dst * 1
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

            // αはとりあえずそのまま足すか、dst を維持するかは好み
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            break;
        case BlendMode::Subtract:// out.rgb = dst.rgb - src.rgb * src.a
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;      // src * src.a
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;            // dst * 1
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
            // out = dst*1 - src*src.a

            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // αは適当でOKなことが多い
            break;
        case BlendMode::Multiply:// out = src * dst + dst * 0 = src * dst
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR; // src * dst
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;       // dst * 0
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            break;
        case BlendMode::Screen:// out.rgb = src.rgb * 1 + dst.rgb * (1 - src.rgb) = src + dst - src * dst
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;         // src * 1
            blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR; // dst * (1 - src)
            blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

            // アルファはお好み
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            break;
        default:
            break;
        }

        pipelineDesc.BlendState = blendDesc;

        // CreatePSO
        HRESULT hr{};
        hr = device->CreateGraphicsPipelineState(
            &pipelineDesc, IID_PPV_ARGS(&setting.pso[i]));
        // 生成できたかチェック 失敗ならアサート
        Core::LogAssert::Check(hr, "PipelineManager", "Failed Create GraphicsPipelineState!!");
    }
}

void Theatria::Graphics::PipelineManager::GetReflectionRootParms(ID3D12ShaderReflection* shaderRef, D3D12_SHADER_DESC shaderDesc, D3D12_SHADER_VISIBILITY shaderVis, std::vector<D3D12_ROOT_PARAMETER>& outRootParms, std::vector<D3D12_STATIC_SAMPLER_DESC>& outStaticSamplers, std::vector<D3D12_DESCRIPTOR_RANGE>& outRenges, D3D12_DESCRIPTOR_RANGE& outTexRenge, bool& outUseTexBuf, std::vector<D3D12_INDIRECT_ARGUMENT_DESC>& outIndirectArgs)
{
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC resourceDesc;
        shaderRef->GetResourceBindingDesc(i, &resourceDesc);
        D3D12_ROOT_PARAMETER rootParm{};
        D3D12_INDIRECT_ARGUMENT_DESC argDesc{};
        switch (resourceDesc.Type)
        {
        case D3D_SIT_CBUFFER:// 定数バッファ
            rootParm.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParm.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            rootParm.Descriptor.ShaderRegister = resourceDesc.BindPoint;
            rootParm.Descriptor.RegisterSpace = resourceDesc.Space;
            // ルートパラメータに追加
            outRootParms.push_back(rootParm);
            // 間接描画用引数に追加
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
            argDesc.ConstantBufferView.RootParameterIndex = static_cast<UINT>(outRootParms.size() - 1);
            outIndirectArgs.push_back(argDesc);
            break;
        case D3D_SIT_STRUCTURED:// 構造バッファ
            rootParm.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
            rootParm.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            rootParm.Descriptor.ShaderRegister = resourceDesc.BindPoint;
            rootParm.Descriptor.RegisterSpace = resourceDesc.Space;
            // ルートパラメータに追加
            outRootParms.push_back(rootParm);
            // 間接描画用引数に追加
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
            argDesc.ShaderResourceView.RootParameterIndex = static_cast<UINT>(outRootParms.size() - 1);
            outIndirectArgs.push_back(argDesc);
            break;
        case D3D_SIT_UAV_RWSTRUCTURED:// 書き込み可能バッファ
            rootParm.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
            rootParm.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            rootParm.Descriptor.ShaderRegister = resourceDesc.BindPoint;
            rootParm.Descriptor.RegisterSpace = resourceDesc.Space;
            // ルートパラメータに追加
            outRootParms.push_back(rootParm);
            // 間接描画用引数に追加
            argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
            argDesc.UnorderedAccessView.RootParameterIndex = static_cast<UINT>(outRootParms.size() - 1);
            outIndirectArgs.push_back(argDesc);
            break;
        case D3D_SIT_TEXTURE:// テクスチャバッファ
            if (outUseTexBuf) { break; }// すでにテクスチャ用ディスクリプタレンジが使われていたらスキップ
            // テクスチャ用ディスクリプタレンジの設定
            outTexRenge.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            outTexRenge.NumDescriptors = UINT_MAX;// unboundにするため最大値
            outTexRenge.BaseShaderRegister = resourceDesc.BindPoint;
            outTexRenge.RegisterSpace = resourceDesc.Space;
            outTexRenge.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            outUseTexBuf = true;///< unboundにするため最後にする
            break;
        case D3D_SIT_SAMPLER:// サンプラー
            D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;// バイリニアフィルタ
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// 0~1の範囲で繰り返す
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;// 比較しない
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;// 最大LOD
            samplerDesc.ShaderRegister = resourceDesc.BindPoint;
            samplerDesc.RegisterSpace = resourceDesc.Space;
            samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
            // スタティックサンプラに追加
            outStaticSamplers.push_back(samplerDesc);
            break;
        default:
            break;
        }
    }
}
