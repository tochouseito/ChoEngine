#include "pch.h"
#include "include/Graphics/FrameGraph.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/Renderer.h"
#include "include/Core/LogAssert.h"

using namespace Theatria::Graphics;


/// @brief パスの追加(任意のキュー指定版)
PassID Theatria::Graphics::FrameGraph::AddPass(std::string_view name, FGQueue queue, PassSetupFn setupFn, PassExecuteFn executeFn)
{
    // 1) 新しい PassNode を追加
    PassNode node;
    if (name.empty())
    {
        Core::LogAssert::Check(false, "FrameGraph", "Pass name is empty");
    }
    node.name = name.data();
    node.index = static_cast<uint32_t>(m_Passes.size());
    node.queue = queue;
    node.executeFn = std::move(executeFn);

    m_Passes.push_back(std::move(node));
    // 2) この pass 用の PassBuilder を作って、リソース宣言(Read/Write/Create) をさせる
    PassBuilder builder(*this, m_Passes.back().index);
    if (setupFn)
    {
        setupFn(builder);
    }

    // 3) ID を返す
    PassID id{ m_Passes.back().index };
    return id;
}

void Theatria::Graphics::FrameGraph::Compile(ResourceManager& rm)
{
    // 0) 前回の結果をリセット
    m_SortedPasses.clear();
    for (auto& p : m_Passes)
    {
        p.outEdges.clear();
        p.inEdges.clear();
        p.barriers.clear();
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
        uint32_t pi = pass.index; // 0..N-1 と仮定

        auto registerUse = [&](const ResourceUse& use) {
            auto& vr = m_VResources[use.handle];
            vr.usedInPass.push_back(pi);
            vr.firstPass = std::min(vr.firstPass, pi);
            vr.lastPass = std::max(vr.lastPass, pi);
            };
        for (auto& u : pass.reads)  registerUse(u);
        for (auto& u : pass.writes) registerUse(u);
    }

    // 2) 使用順にパス間エッジを張る
    for (auto& vr : m_VResources)
    {
        auto& uses = vr.usedInPass;
        if (uses.size() <= 1) continue;

        // パスindex順にソート（念のため）
        std::sort(uses.begin(), uses.end());
        uses.erase(std::unique(uses.begin(), uses.end()), uses.end());

        for (size_t i = 0; i + 1 < uses.size(); ++i)
        {
            uint32_t src = uses[i];
            uint32_t dst = uses[i + 1];
            // src -> dst に有向辺
            auto& pSrc = m_Passes[src];
            auto& pDst = m_Passes[dst];
            pSrc.outEdges.push_back(dst);
            pDst.inEdges.push_back(src);
        }
    }

    // 3) トポロジカルソート（Kahn）
    std::vector<uint32_t> indeg(m_Passes.size(), 0);
    for (auto& p : m_Passes)
    {
        indeg[p.index] = static_cast<uint32_t>(p.inEdges.size());
    }

    std::queue<uint32_t> q;
    for (auto& p : m_Passes)
    {
        if (indeg[p.index] == 0) q.push(p.index);
    }

    m_SortedPasses.clear();
    while (!q.empty())
    {
        uint32_t u = q.front(); q.pop();
        m_SortedPasses.push_back(u);
        for (auto v : m_Passes[u].outEdges)
        {
            if (--indeg[v] == 0) q.push(v);
        }
    }

    if (m_SortedPasses.size() != m_Passes.size())
    {
        // サイクル検出 → 設計ミス（同じリソースを循環参照している）
        Core::LogAssert::Check(false, "FrameGraph", "cycle detected in pass graph");
    }

    // 4) リソース状態遷移の計算
    std::vector<FGState> currentState(m_VResources.size(), FGState::Unknown);

    for (uint32_t pid : m_SortedPasses)
    {
        auto& pass = m_Passes[pid];

        // このパスで触る全リソースについて、望む状態を計算
        // （同じリソースが Read と Write で出ることもあるので、最終状態を一回決める）
        struct Desired
        {
            FGState state = FGState::Unknown;
            bool    has = false;
        };
        std::vector<Desired> desired(m_VResources.size());

        auto accumulate = [&](const ResourceUse& u) {
            auto& vr = m_VResources[u.handle];
            FGState st = DecideState(vr.desc, u.access);
            auto& d = desired[u.handle];
            if (!d.has) { d.state = st; d.has = true; }
            else
            {
                // 既にある場合、より強い状態に揃えるなどのポリシーが必要。
                // 簡単には「Write側の状態を優先」など。
                if (st != d.state)
                {
                    // ここはケースバイケース。とりあえず上書き。
                    d.state = st;
                }
            }
            };

        for (auto& u : pass.reads)  accumulate(u);
        for (auto& u : pass.writes) accumulate(u);

        // 前回状態との違いからバリアを作成
        for (ResourceHandle h = 0; h < m_VResources.size(); ++h)
        {
            auto& d = desired[h];
            if (!d.has) continue; // このパスでは使わない

            FGState prev = currentState[h];
            FGState next = d.state;

            if (prev == FGState::Unknown)
            {
                // 初回使用。初期状態を何と見るかはポリシー次第。
                // imported リソースなら別途 initialState を持っておくべき。
                currentState[h] = next;
                continue;
            }
            if (prev == next) continue; // バリア不要

            pass.barriers.push_back(BarrierInfo{ h, prev, next });
            currentState[h] = next;
        }
    }

    // 5) 物理リソース割当（まずは1:1）
    for (auto& vr : m_VResources)
    {
        // 寿命情報: vr.firstPass ～ vr.lastPass
        // 今は使わないが、将来エイリアシングに使う。

        vr.physicalId = rm.CreateRenderTargetBuffer(
            vr.desc.width,
            vr.desc.height,
            vr.desc.format);
    }
}

void Theatria::Graphics::FrameGraph::Execute(Renderer& renderer, ResourceManager& rm)
{
    for (uint32_t pid : m_SortedPasses)
    {
        auto& pass = m_Passes[pid];
        PassContext pctx(*this, rm, pid);

        switch (pass.queue)
        {
        case FGQueue::Graphics:
        {
            GraphicsCommandContext* cmd = renderer.BeginGraphicsPass(); // 旧 BeginRenderPass に相当

            renderer.ApplyBarriers(*this, cmd, pass.barriers);
            pass.executeFn(pctx, *cmd); // void* に渡す

            renderer.EndGraphicsPass(cmd);
            break;
        }
        case FGQueue::Compute:
        {
            ComputeCommandContext* cmd = renderer.BeginComputePass();

            renderer.ApplyBarriers(*this, cmd, pass.barriers);
            pass.executeFn(pctx, *cmd);

            renderer.EndComputePass(cmd);
            break;
        }
        case FGQueue::Copy:
        {
            CopyCommandContext* cmd = renderer.BeginCopyPass();

            renderer.ApplyBarriers(*this, cmd, pass.barriers);
            pass.executeFn(pctx, *cmd);

            renderer.EndCopyPass(cmd);
            break;
        }
        default:
        {
            Core::LogAssert::Check(false, "FrameGraph", "Execute: Unknown FGQueue");
            break;
        }
        }
    }
    // フリップ
    renderer.Present();
}

ResourceHandle Theatria::Graphics::FrameGraph::CreateVirtualResource(std::string_view name, const ResourceDesc& desc)
{
    // すでに同名があったらエラー。新規作成のみ。
    if (!name.empty() && m_NameToResource.find(name.data()) != m_NameToResource.end())
    {
        Core::LogAssert::Check(false, "FrameGraph", std::string("FrameGraph: resource already exists '") + name.data() + "'");
    }

    ResourceHandle h = static_cast<ResourceHandle>(m_VResources.size());
    VirtualResource vr;
    vr.name = name.data();
    vr.desc = desc;
    m_VResources.push_back(std::move(vr));

    m_NameToResource.emplace(name.data(), h);
    return h;
}

void Theatria::Graphics::FrameGraph::AddDependency(PassID before, PassID after)
{
    auto b = before.idx;
    auto a = after.idx;
    m_Passes[b].outEdges.push_back(a);
    m_Passes[a].inEdges.push_back(b);
}

FGState Theatria::Graphics::DecideState(const ResourceDesc& desc, FGAccess access)
{
    switch (desc.usage)
    {
    case FGUsage::RenderTarget:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
            return FGState::RenderTarget;
        else
            return FGState::ShaderRead; // post effect で読む etc.
    case FGUsage::DepthStencil:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
            return FGState::DepthWrite;
        else
            return FGState::ShaderRead; // shadow map etc.
    case FGUsage::Texture:
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
            return FGState::CopyDst;    // 雑に copy 先とする（本当は UAV とか区別すべき）
        else
            return FGState::ShaderRead;
    case FGUsage::Buffer:
        // ここは用途次第。とりあえず ShaderRead 向け。
        if (access == FGAccess::Write || access == FGAccess::ReadWrite)
            return FGState::CopyDst;
        else
            return FGState::ShaderRead;
    }
    return FGState::Unknown;
}

D3D12_RESOURCE_STATES Theatria::Graphics::FGStateToD3D12State(FGState state)
{
    switch (state)
    {
    case Theatria::Graphics::FGState::RenderTarget:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
        break;
    case Theatria::Graphics::FGState::DepthWrite:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        break;
    case Theatria::Graphics::FGState::DepthRead:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
        break;
    case Theatria::Graphics::FGState::ShaderRead:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        break;
    case Theatria::Graphics::FGState::UnorderedAccess:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        break;
    case Theatria::Graphics::FGState::CopySrc:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
        break;
    case Theatria::Graphics::FGState::CopyDst:
        return D3D12_RESOURCE_STATE_COPY_DEST;
        break;
    case Theatria::Graphics::FGState::Present:
        return D3D12_RESOURCE_STATE_PRESENT;
        break;
    case Theatria::Graphics::FGState::Unknown:
    default:
        Core::LogAssert::Check(false, "FrameGraph", "FGStateToD3D12State: Unknown FGState");
        return D3D12_RESOURCE_STATE_COMMON;
        break;
    }
}

ResourceHandle Theatria::Graphics::PassBuilder::registerNewResource(std::string_view name, const ResourceDesc& desc)
{
    return m_FrameGraph.CreateVirtualResource(name, desc);
}

ResourceHandle Theatria::Graphics::PassBuilder::registerUse(std::string_view name, FGAccess access)
{
    ResourceHandle h = m_FrameGraph.FindResourceHandle(name);
    if (h == InvalidResource)
    {
        // なければエラー
        Core::LogAssert::Check(false, "FrameGraph", std::string("FrameGraph: unknown resource '") + name.data() + "'");
    }
    registerUse(h, access);
    return h;
}

void Theatria::Graphics::PassBuilder::registerUse(ResourceHandle h, FGAccess access)
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
    }
}
