#include "pch.h"
#include "include/Graphics/FrameGraph.h"

#include <algorithm>
#include <queue>

#include "include/Graphics/DescriptorAllocator.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/PipelineManager.h"
#include "config/engineConfig.h"
#include "include/Platform/WinApp.h"
#include "include/Core/LogAssert.h"

using namespace Theatria::Graphics;

//--------------------------------------------------
// FrameGraph::CreateDefaultPasses
//--------------------------------------------------

void FrameGraph::CreateDefaultPasses()
{
    AddPass(
        "CreateIndirectCommand", FGQueue::Compute,
        [&](PassBuilder& builder)
        {
            builder;
        },
        [&](PassContext& passCtx, CommandContext& cmdCtx, uint32_t frameIdx)
        {
            GraphicsPipelineSettings* gPipeline = passCtx.m_PipelineManager.GetGraphicsPipelineByName("BasicPipeline");
            ComputePipelineSettings* pipeline = passCtx.m_PipelineManager.GetComputePipelineByName("BasicPipeline");

            // バッファ取得
            auto& gObjBuf = passCtx.m_ResourceManager.GetGlobalObjectBuffer<ShaderStruct::SObject>();
            auto& objBuf = gObjBuf.GetGpuBuffer(frameIdx);
            auto& gTransBuf = passCtx.m_ResourceManager.GetGlobalTransformBuffer<ShaderStruct::STransform>();
            auto& transBuf = gTransBuf.GetGpuBuffer(frameIdx);
            auto& gMeshBuf = passCtx.m_ResourceManager.GetGlobalMeshInfoBuffer<ShaderStruct::SMeshInfo>();
            auto& meshBuf = gMeshBuf.GetGpuBuffer(frameIdx);
            auto& indirectCmdCountBuf = passCtx.m_ResourceManager.GetIndirectCommandCountBuffer();
            auto& indirectCmdBuf = gPipeline->argsBuffer;

            // バリア
            cmdCtx.BarrierTransition(
                &objBuf,
                objBuf.GetUseState(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmdCtx.BarrierTransition(
                &transBuf,
                transBuf.GetUseState(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmdCtx.BarrierTransition(
                &meshBuf,
                meshBuf.GetUseState(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            cmdCtx.BarrierTransition(
                &indirectCmdCountBuf,
                indirectCmdCountBuf.GetUseState(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            cmdCtx.BarrierTransition(
                &indirectCmdBuf,
                indirectCmdBuf.GetUseState(),
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);


            // CommandCount バッファのクリア
            UINT clearValues[4] = { 0, 0, 0, 0 };
            GpuBuffer& u1 = passCtx.m_ResourceManager.GetIndirectCommandCountBuffer();
            DescriptorAllocator::TableID u1Handle = passCtx.m_ResourceManager.GetIndirectCommandCountBufferDescriptorID();
            D3D12_GPU_DESCRIPTOR_HANDLE u1GPUHandle = passCtx.m_DescriptorAllocator.GetGPUHandle(u1Handle);
            D3D12_CPU_DESCRIPTOR_HANDLE u1CPUHandle = passCtx.m_DescriptorAllocator.GetCPUHandle(u1Handle);
            cmdCtx.ClearUnorderedAccessViewUint(
                u1GPUHandle,
                u1CPUHandle,
                u1.GetResource(),
                clearValues,
                0, nullptr);

            // デスクリプタヒープ設定
            ID3D12DescriptorHeap* heap =
                passCtx.m_DescriptorAllocator.GetDescriptorHeap(HeapType::CBV_SRV_UAV);
            cmdCtx.SetDescriptorHeap(heap);

            // パイプライン
            cmdCtx.SetPipelineState(pipeline->pso.Get());
            cmdCtx.SetComputeRootSignature(pipeline->rootSignature.Get());

            // バインド
            D3D12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle = passCtx.m_DescriptorAllocator.GetGPUHandle(gObjBuf.GetDescriptorTableID(frameIdx));
            cmdCtx.SetComputeRootDescriptorTable(0, gpuSrvHandle);

            D3D12_GPU_DESCRIPTOR_HANDLE gpuUavHandle = passCtx.m_DescriptorAllocator.GetGPUHandle(gPipeline->argsDescriptorTableID);
            cmdCtx.SetComputeRootDescriptorTable(1, gpuUavHandle);

            uint32_t objCount = gObjBuf.GetTotalCount();
            cmdCtx.SetComputeRoot32BitConstant(2, objCount, 0);

            // Dispatch
            UINT groupSize = 64;
            UINT dispatchCount = (objCount + groupSize - 1) / groupSize;
            cmdCtx.Dispatch(dispatchCount, 1, 1);

            // UAVバリア
            cmdCtx.BarrierUAV(&indirectCmdCountBuf);
            cmdCtx.BarrierUAV(&indirectCmdBuf);
        });

    AddPass(
        "FinalColor",
        FGQueue::Graphics,
        [&](PassBuilder& builder)
        {
            ResourceDesc fcDesc{};
            fcDesc.usage = FGUsage::RenderTarget;
            fcDesc.width = Config::Graphics::ResolutionWidth;
            fcDesc.height = Config::Graphics::ResolutionHeight;
            fcDesc.format = Config::Graphics::DefaultDXGIFormat;

            // 初期ステートは ShaderRead にしておく
            // （フレーム外では SRV として参照できる状態にしておく想定）
            builder.Create("finalColor", fcDesc, FGState::ShaderRead);

            // このパスでは RenderTarget として書き込む
            builder.Write("finalColor");
        },
        [&](PassContext& passCtx, CommandContext& cmdCtx, uint32_t frameIdx)
        {
            // デスクリプタヒープ設定
            ID3D12DescriptorHeap* heap =
                passCtx.m_DescriptorAllocator.GetDescriptorHeap(HeapType::CBV_SRV_UAV);
            cmdCtx.SetDescriptorHeap(heap);

            // finalColor の RTV を取得
            ResourceHandle h = passCtx.m_FrameGraph.FindResourceHandle("finalColor");
            const VirtualResource& vr = passCtx.m_FrameGraph.GetVirtualResource(h);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
                passCtx.m_DescriptorAllocator.GetCPUHandle(vr.rtvTableId);

            // RTV 設定 & クリア
            cmdCtx.SetRenderTargets(1, &rtvHandle, false, nullptr);
            cmdCtx.ClearRenderTargetView(rtvHandle, Config::Graphics::kClearColor, 0, nullptr);

            // ビューポートとシザー矩形の設定
            D3D12_VIEWPORT viewport{
                0.0f,
                0.0f,
                static_cast<float>(Platform::WinApp::m_WindowWidth),
                static_cast<float>(Platform::WinApp::m_WindowHeight),
                0.0f,
                1.0f
            };
            cmdCtx.SetViewport(viewport);

            D3D12_RECT rect{
                0,
                0,
                static_cast<LONG>(Platform::WinApp::m_WindowWidth),
                static_cast<LONG>(Platform::WinApp::m_WindowHeight)
            };
            cmdCtx.SetScissorRect(rect);

            // トポロジ設定（ここでは何も描画していないが雛形として）
            cmdCtx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            //// TODO: パイプライン設定 / DrawCall など
            //GraphicsPipelineSettings* pipeline = passCtx.m_PipelineManager.GetGraphicsPipelineByName("BasicPipeline");
            //cmdCtx.SetPipelineState(pipeline->pso[static_cast<size_t>(BlendMode::Normal)].Get());
            //cmdCtx.SetGraphicsRootSignature(pipeline->rootSignature.Get());

            //auto& devCam = passCtx.m_ResourceManager.GetDebugCamBuf(frameIdx);
            //cmdCtx.SetGraphicsRootConstantBufferView(0, devCam->GetGPUVirtualAddress());

            //auto& gObjBuf = passCtx.m_ResourceManager.GetGlobalObjectBuffer<ShaderStruct::SObject>();
            //auto& objBuf = gObjBuf.GetGpuBuffer(frameIdx);
            //auto& gTransBuf = passCtx.m_ResourceManager.GetGlobalTransformBuffer<ShaderStruct::STransform>();
            //auto& transBuf = gTransBuf.GetGpuBuffer(frameIdx);
            //auto& gMeshBuf = passCtx.m_ResourceManager.GetGlobalMeshInfoBuffer<ShaderStruct::SMeshInfo>();
            //auto& meshBuf = gMeshBuf.GetGpuBuffer(frameIdx);
            //auto& integratedVB = passCtx.m_ResourceManager.GetVerte
            frameIdx;
        });
}

//--------------------------------------------------
// FrameGraph::AddPass
//--------------------------------------------------

PassID FrameGraph::AddPass(std::string_view name,
    FGQueue queue,
    PassSetupFn setupFn,
    PassExecuteFn executeFn)
{
    PassNode node;
    if (name.empty())
    {
        Core::LogAssert::Check(false, "FrameGraph", "AddPass: Pass name is empty");
    }

    node.name = std::string(name);
    node.index = static_cast<uint32_t>(m_Passes.size());
    node.queue = queue;
    node.executeFn = std::move(executeFn);

    m_Passes.push_back(std::move(node));

    PassBuilder builder(*this, m_Passes.back().index);
    if (setupFn)
    {
        setupFn(builder);
    }

    PassID id{ m_Passes.back().index };
    return id;
}

//--------------------------------------------------
// FrameGraph::Compile
//  - パス依存 / トポロジカルソート / 物理リソース割り当てのみ
//  - バリアは実行時に計算する
//--------------------------------------------------

void FrameGraph::Compile(DescriptorAllocator& da, ResourceManager& rm)
{
    // 0) 前回結果をリセット
    m_SortedPasses.clear();
    for (auto& p : m_Passes)
    {
        p.outEdges.clear();
        p.inEdges.clear();
    }
    for (auto& r : m_VResources)
    {
        r.usedInPass.clear();
        r.firstPass = UINT32_MAX;
        r.lastPass = 0;
        r.physicalId = UINT32_MAX;
    }

    // 1) リソースごとの使用パスを集計
    for (auto& pass : m_Passes)
    {
        uint32_t pi = pass.index;

        auto registerUse = [&](const ResourceUse& use)
            {
                auto& vr = m_VResources[use.handle];
                vr.usedInPass.push_back(pi);
                vr.firstPass = std::min(vr.firstPass, pi);
                vr.lastPass = std::max(vr.lastPass, pi);
            };

        for (auto& u : pass.reads)
        {
            registerUse(u);
        }
        for (auto& u : pass.writes)
        {
            registerUse(u);
        }
    }

    // 2) 使用順にパス間エッジを張る
    for (auto& vr : m_VResources)
    {
        auto& uses = vr.usedInPass;
        if (uses.size() <= 1) { continue; }

        std::sort(uses.begin(), uses.end());
        uses.erase(std::unique(uses.begin(), uses.end()), uses.end());

        for (size_t i = 0; i + 1 < uses.size(); ++i)
        {
            uint32_t src = uses[i];
            uint32_t dst = uses[i + 1];

            auto& pSrc = m_Passes[src];
            auto& pDst = m_Passes[dst];

            pSrc.outEdges.push_back(dst);
            pDst.inEdges.push_back(src);
        }
    }

    // 3) トポロジカルソート（Kahn 法）
    std::vector<uint32_t> indeg(m_Passes.size(), 0);
    for (auto& p : m_Passes)
    {
        indeg[p.index] = static_cast<uint32_t>(p.inEdges.size());
    }

    std::queue<uint32_t> q;
    for (auto& p : m_Passes)
    {
        if (indeg[p.index] == 0)
        {
            q.push(p.index);
        }
    }

    m_SortedPasses.clear();
    while (!q.empty())
    {
        uint32_t u = q.front();
        q.pop();
        m_SortedPasses.push_back(u);

        for (auto v : m_Passes[u].outEdges)
        {
            if (--indeg[v] == 0)
            {
                q.push(v);
            }
        }
    }

    if (m_SortedPasses.size() != m_Passes.size())
    {
        Core::LogAssert::Check(false, "FrameGraph", "Compile: cycle detected in pass graph");
    }

    // 4) 物理リソース割当（現状は 1:1 割り当て）
    for (auto& vr : m_VResources)
    {
        if (vr.desc.existsGlobalBufferType.has_value())
        {
            // グローバルバッファは外側で管理する想定
            continue;
        }

        switch (vr.desc.usage)
        {
        case FGUsage::Texture:
            // TODO: Texture 用の割当
            break;
        case FGUsage::Buffer:
            // TODO: Buffer 用の割当
            break;
        case FGUsage::RenderTarget:
            vr.physicalId = rm.CreateRenderTargetBuffer(
                vr.desc.width,
                vr.desc.height,
                vr.desc.format);
            vr.rtvTableId =
                da.Allocate(DescriptorAllocator::TableKind::RenderTargets);
            da.CreateRTV(vr.rtvTableId, rm.GetTextureBuffer(vr.physicalId));
            vr.srvTableId =
                da.Allocate(DescriptorAllocator::TableKind::Textures);
            da.CreateSRVTexture2D(vr.srvTableId, rm.GetTextureBuffer(vr.physicalId));
            break;
        case FGUsage::DepthStencil:
            vr.physicalId = rm.CreateDepthBuffer(
                vr.desc.width,
                vr.desc.height);
            break;
        case FGUsage::UnorderedAccess:
            // TODO: UAV 用の割当
            break;
        case FGUsage::Unknown:
        default:
            Core::LogAssert::Check(false, "FrameGraph", "Compile: Unknown FGUsage");
            break;
        }
    }

    // 5) 現在ステート配列を初期化（実際のステートは Execute 側で決める）
    m_CurrentStates.clear();
    m_CurrentStates.resize(m_VResources.size(), FGState::Unknown);
}

//--------------------------------------------------
// FrameGraph::Execute
//  - キューごとに CommandContext を用意
//  - 各パスの前後で ApplyPassBarriersBegin/End を呼ぶ
//--------------------------------------------------

void FrameGraph::Execute(uint32_t frameIdx,
    Renderer& renderer,
    DescriptorAllocator& da,
    ResourceManager& rm,
    PipelineManager& pm)
{
    // 念のためサイズ同期（Compile 後に呼ばれる前提だが安全側）
    if (m_CurrentStates.size() != m_VResources.size())
    {
        m_CurrentStates.clear();
        m_CurrentStates.resize(m_VResources.size(), FGState::Unknown);
    }

    GraphicsCommandContext* gCmd = nullptr;
    ComputeCommandContext* cCmd = nullptr;
    CopyCommandContext* copyCmd = nullptr;

    for (uint32_t pid : m_SortedPasses)
    {
        auto& pass = m_Passes[pid];
        PassContext pctx(*this, da, rm, pm, pid);

        CommandContext* baseCmd = nullptr;

        switch (pass.queue)
        {
        case FGQueue::Graphics:
            if (!gCmd)
            {
                gCmd = renderer.BeginGraphicsPass();
            }
            baseCmd = gCmd;
            break;
        case FGQueue::Compute:
            if (!cCmd)
            {
                cCmd = renderer.BeginComputePass();
            }
            baseCmd = cCmd;
            break;
        case FGQueue::Copy:
            if (!copyCmd)
            {
                copyCmd = renderer.BeginCopyPass();
            }
            baseCmd = copyCmd;
            break;
        default:
            Core::LogAssert::Check(false, "FrameGraph", "Execute: Unknown FGQueue");
            continue;
        }

        // パス開始前バリア（休み状態 → パス用ステート）
        ApplyPassBarriersBegin(renderer, baseCmd, pass);

        // パス実行
        pass.executeFn(pctx, *baseCmd, frameIdx);

        // パス終了後バリア（パス用ステート → 休み状態）
        ApplyPassBarriersEnd(renderer, baseCmd, pass);
    }

    // キューごとに閉じる
    if (gCmd) { renderer.EndGraphicsPass(gCmd); }
    if (cCmd) { renderer.EndComputePass(cCmd); }
    if (copyCmd) { renderer.EndCopyPass(copyCmd); }
}

//--------------------------------------------------
// FrameGraph::CreateVirtualResource
//--------------------------------------------------

ResourceHandle FrameGraph::CreateVirtualResource(std::string_view name,
    const ResourceDesc& desc,
    FGState initialState)
{
    // 同名があればエラー（新規のみ）
    if (!name.empty() && m_NameToResource.find(std::string(name)) != m_NameToResource.end())
    {
        Core::LogAssert::Check(
            false,
            "FrameGraph",
            std::string("CreateVirtualResource: resource already exists '") +
            std::string(name) + "'");
    }

    ResourceHandle h = static_cast<ResourceHandle>(m_VResources.size());

    VirtualResource vr{};
    vr.name = std::string(name);
    vr.desc = desc;
    vr.initialState = initialState;

    m_NameToResource.emplace(vr.name, h);
    m_VResources.push_back(std::move(vr));

    return h;
}

//--------------------------------------------------
// FrameGraph::AddDependency
//--------------------------------------------------

void FrameGraph::AddDependency(PassID before, PassID after)
{
    auto b = before.idx;
    auto a = after.idx;
    m_Passes[b].outEdges.push_back(a);
    m_Passes[a].inEdges.push_back(b);
}

//--------------------------------------------------
// DecideState
//--------------------------------------------------

FGState Theatria::Graphics::DecideState(const ResourceDesc& desc, FGAccess access)
{
    switch (desc.usage)
    {
    case FGUsage::RenderTarget:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
        {
            return FGState::RenderTarget;
        }
        else
        {
            // RenderTarget を SRV として読むケース（ポストエフェクトなど）
            return FGState::ShaderRead;
        }
    case FGUsage::DepthStencil:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
        {
            return FGState::DepthWrite;
        }
        else
        {
            // シャドウマップなど読み取り専用
            return FGState::ShaderRead;
        }
    case FGUsage::Texture:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
        {
            // 雑に CopyDst として扱う（本当は UAV などと区別すべき）
            return FGState::CopyDst;
        }
        else
        {
            return FGState::ShaderRead;
        }
    case FGUsage::Buffer:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
        {
            return FGState::UnorderedAccess;
        }
        else
        {
            return FGState::ShaderRead;
        }
    case FGUsage::UnorderedAccess:
        return FGState::UnorderedAccess;
    case FGUsage::Unknown:
    default:
        return FGState::Unknown;
    }
}

//--------------------------------------------------
// FGStateToD3D12State
//--------------------------------------------------

D3D12_RESOURCE_STATES Theatria::Graphics::FGStateToD3D12State(FGState state)
{
    switch (state)
    {
    case FGState::Common:
        return D3D12_RESOURCE_STATE_COMMON;
    case FGState::RenderTarget:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case FGState::DepthWrite:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case FGState::DepthRead:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case FGState::ShaderRead:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case FGState::UnorderedAccess:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case FGState::CopySrc:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case FGState::CopyDst:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case FGState::Present:
        return D3D12_RESOURCE_STATE_PRESENT;
    case FGState::Unknown:
    default:
        Core::LogAssert::Check(false, "FrameGraph", "FGStateToD3D12State: Unknown FGState");
        return D3D12_RESOURCE_STATE_COMMON;
    }
}

//--------------------------------------------------
// PassBuilder
//--------------------------------------------------

ResourceHandle PassBuilder::Create(std::string_view name,
    const ResourceDesc& desc,
    FGState initialState)
{
    return registerNewResource(name, desc, initialState);
}

ResourceHandle PassBuilder::Read(std::string_view name)
{
    return registerUse(name, FGAccess::Read);
}

ResourceHandle PassBuilder::Write(std::string_view name)
{
    return registerUse(name, FGAccess::Write);
}

ResourceHandle PassBuilder::ReadWrite(std::string_view name)
{
    return registerUse(name, FGAccess::ReadWrite);
}

void PassBuilder::Read(ResourceHandle h)
{
    registerUse(h, FGAccess::Read);
}

void PassBuilder::Write(ResourceHandle h)
{
    registerUse(h, FGAccess::Write);
}

void PassBuilder::ReadWrite(ResourceHandle h)
{
    registerUse(h, FGAccess::ReadWrite);
}

ResourceHandle PassBuilder::registerNewResource(std::string_view name,
    const ResourceDesc& desc,
    FGState initialState)
{
    return m_FrameGraph.CreateVirtualResource(name, desc, initialState);
}

ResourceHandle PassBuilder::registerUse(std::string_view name, FGAccess access)
{
    ResourceHandle h = m_FrameGraph.FindResourceHandle(name);
    if (h == InvalidResource)
    {
        Core::LogAssert::Check(
            false,
            "FrameGraph",
            std::string("registerUse: unknown resource '") +
            std::string(name) + "'");
    }
    registerUse(h, access);
    return h;
}

void PassBuilder::registerUse(ResourceHandle h, FGAccess access)
{
    auto& pass = m_FrameGraph.GetPass(m_PassIndex);
    ResourceUse u{ h, access };

    switch (access)
    {
    case FGAccess::Read:
        pass.reads.push_back(u);
        break;
    case FGAccess::Write:
        pass.writes.push_back(u);
        break;
    case FGAccess::ReadWrite:
        pass.reads.push_back(u);
        pass.writes.push_back(u);
        break;
    case FGAccess::Unknown:
    default:
        Core::LogAssert::Check(false, "FrameGraph", "registerUse: Unknown FGAccess");
        break;
    }
}

//--------------------------------------------------
// PassContext
//--------------------------------------------------

GraphicsPipelineSettings* PassContext::GetGraphicsPipelineByName(const std::string& name)
{
    return m_PipelineManager.GetGraphicsPipelineByName(name);
}

ComputePipelineSettings* PassContext::GetComputePipelineByName(const std::string& name)
{
    return m_PipelineManager.GetComputePipelineByName(name);
}

//--------------------------------------------------
// FrameGraph::ApplyPassBarriersBegin
//  - 休み状態(initialState) → パス用ステート への遷移
//--------------------------------------------------

void FrameGraph::ApplyPassBarriersBegin(Renderer& renderer,
    CommandContext* cmd,
    const PassNode& pass)
{
    // このパスで必要な最終ステートを集計
    struct Desired
    {
        FGState state = FGState::Unknown;
        bool    has = false;
    };

    std::vector<Desired> desired(m_VResources.size());

    auto accumulate = [&](const ResourceUse& u)
        {
            auto& vr = m_VResources[u.handle];
            FGState st = DecideState(vr.desc, u.access);

            auto& d = desired[u.handle];
            if (!d.has)
            {
                d.state = st;
                d.has = true;
            }
            else
            {
                // Read + Write などが混在する場合は「後勝ち」で単純化
                if (st != d.state)
                {
                    d.state = st;
                }
            }
        };

    for (auto& u : pass.reads)
    {
        accumulate(u);
    }
    for (auto& u : pass.writes)
    {
        accumulate(u);
    }

    std::vector<BarrierInfo> barriers;
    barriers.reserve(pass.reads.size() + pass.writes.size());

    for (ResourceHandle h = 0; h < m_VResources.size(); ++h)
    {
        auto& d = desired[h];
        if (!d.has) { continue; }

        auto& vr = m_VResources[h];

        FGState& cur = m_CurrentStates[h];
        FGState  next = d.state;

        if (cur == FGState::Unknown)
        {
            // 初回は initialState から始まるとみなす
            cur = vr.initialState;
        }

        if (cur == next)
        {
            // UAV の場合は同一ステートでも UAV バリアを打つ
            if (next == FGState::UnorderedAccess)
            {
                barriers.push_back(BarrierInfo{ h, BarrierType::UAV, next, next });
            }
            continue;
        }

        barriers.push_back(BarrierInfo{ h, BarrierType::Transition, cur, next });
        cur = next;
    }

    if (!barriers.empty())
    {
        renderer.ApplyBarriers(*this, cmd, barriers);
    }
}

//--------------------------------------------------
// FrameGraph::ApplyPassBarriersEnd
//  - パス用ステート → 休み状態(initialState) への遷移
//--------------------------------------------------

void FrameGraph::ApplyPassBarriersEnd(Renderer& renderer,
    CommandContext* cmd,
    const PassNode& pass)
{
    std::vector<BarrierInfo> barriers;

    // 同じリソースを reads/writes 両方で触る場合があるので、重複を避ける
    std::vector<bool> touched(m_VResources.size(), false);

    auto processList = [&](const std::vector<ResourceUse>& uses)
        {
            for (auto& u : uses)
            {
                ResourceHandle h = u.handle;
                if (touched[h]) { continue; }
                touched[h] = true;

                auto& vr = m_VResources[h];

                FGState& cur = m_CurrentStates[h];
                FGState  target = vr.initialState;

                if (cur == FGState::Unknown)
                {
                    // 何もしていないとみなし、initialState に揃えておく
                    cur = target;
                }

                if (cur == target)
                {
                    continue;
                }

                barriers.push_back(BarrierInfo{
                    h,
                    BarrierType::Transition,
                    cur,
                    target
                    });
                cur = target;
            }
        };

    processList(pass.reads);
    processList(pass.writes);

    if (!barriers.empty())
    {
        renderer.ApplyBarriers(*this, cmd, barriers);
    }
}
