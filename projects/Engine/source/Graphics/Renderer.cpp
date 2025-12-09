#include "pch.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/ResourceManager.h"
#include "config/engineConfig.h"
#include "include/Graphics/FrameGraph.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Platform/WinApp.h"
#ifndef NDEBUG
#include "include/Editor/ImGuiManager.h"
#endif // !NDEBUG


/// @brief 初期化
[[nodiscard]]
bool Theatria::Graphics::Renderer::Initialize(RenderDevice* device, ResourceManager* rm, DescriptorAllocator* da)
{
    m_Device = device;
    m_ResourceManager = rm;
    m_DescriptorAllocator = da;
    m_CommandPool = std::make_unique<CommandPool>(m_Device->m_Device.Get());
    // 深度バッファの作成
    m_DepthBufferIndex = m_ResourceManager->CreateDepthBuffer(Config::Graphics::ResolutionWidth, Config::Graphics::ResolutionHeight);
#ifndef NDEBUG // デバッグ、開発用
    m_DebugDepthBufferIndex = m_ResourceManager->CreateDepthBuffer(Config::Graphics::ResolutionWidth, Config::Graphics::ResolutionHeight);
#endif
    return true;
}

/// @brief 描画開始
/// @return 
Theatria::Graphics::GraphicsCommandContext* Theatria::Graphics::Renderer::BeginGraphicsPass() noexcept
{
    GraphicsCommandContext* cmd = m_CommandPool->GetGraphicsContext();
    cmd->Reset();
    return cmd;
}

/// @brief 描画終了
void Theatria::Graphics::Renderer::EndGraphicsPass(GraphicsCommandContext* cmd) noexcept
{
    // コマンドリストのクローズ
    cmd->Close();
    // コマンドリストの実行 + signal
    GraphicsQueueContext* graphicsQueue = m_Device->m_QueuePool->GetGraphicsQueue();
    graphicsQueue->Execute(cmd);
    // GPU完了待ち
    graphicsQueue->WaitForFence();
    // キューコンテキストの返却
    m_Device->m_QueuePool->ReturnQueue(graphicsQueue);
    // コマンドコンテキストの返却
    m_CommandPool->ReturnContext(cmd);
}

Theatria::Graphics::ComputeCommandContext* Theatria::Graphics::Renderer::BeginComputePass() noexcept
{
    ComputeCommandContext* cmd = m_CommandPool->GetComputeContext();
    cmd->Reset();
    return cmd;
}

void Theatria::Graphics::Renderer::EndComputePass(ComputeCommandContext* cmd) noexcept
{
    // コマンドリストのクローズ
    cmd->Close();
    // コマンドリストの実行 + signal
    ComputeQueueContext* computeQueue = m_Device->m_QueuePool->GetComputeQueue();
    computeQueue->Execute(cmd);
    // GPU完了待ち
    computeQueue->WaitForFence();
    // キューコンテキストの返却
    m_Device->m_QueuePool->ReturnQueue(computeQueue);
    // コマンドコンテキストの返却
    m_CommandPool->ReturnContext(cmd);
}

Theatria::Graphics::CopyCommandContext* Theatria::Graphics::Renderer::BeginCopyPass() noexcept
{
    CopyCommandContext* cmd = m_CommandPool->GetCopyContext();
    cmd->Reset();
    return cmd;
}

void Theatria::Graphics::Renderer::EndCopyPass(CopyCommandContext* cmd) noexcept
{
    // コマンドリストのクローズ
    cmd->Close();
    // コマンドリストの実行 + signal
    CopyQueueContext* copyQueue = m_Device->m_QueuePool->GetCopyQueue();
    copyQueue->Execute(cmd);
    // GPU完了待ち
    copyQueue->WaitForFence();
    // キューコンテキストの返却
    m_Device->m_QueuePool->ReturnQueue(copyQueue);
    // コマンドコンテキストの返却
    m_CommandPool->ReturnContext(cmd);
}

void Theatria::Graphics::Renderer::ApplyBarriers(FrameGraph& fg, CommandContext* cmd, const std::vector<BarrierInfo>& barriers)
{
    for (auto& barrier : barriers)
    {
        // 仮想リソースから物理リソースを取得
        const VirtualResource& vres = fg.GetVirtualResource(barrier.handle);
        GpuResource* resource = m_ResourceManager->GetTextureBuffer(vres.physicalId);
        // バリア挿入
        if (barrier.type == BarrierType::Transition)
        {
            D3D12_RESOURCE_STATES beforeState = // FGStateToD3D12State(barrier.beforeState);
                resource->GetUseState();
            D3D12_RESOURCE_STATES afterState = FGStateToD3D12State(barrier.afterState);
            cmd->BarrierTransition(resource, beforeState, afterState);
        }
        else if (barrier.type == BarrierType::UAV)
        {
            cmd->BarrierUAV(resource);
        }
    }
}

void Theatria::Graphics::Renderer::Present()
{
    UINT backBufferIdx = m_Device->m_SwapChainContext.m_SwapChain->GetCurrentBackBufferIndex();
    SwapChainBuffer& backBuffer = m_Device->m_SwapChainContext.m_BackBuffers[backBufferIdx];
    GraphicsCommandContext* cmd = m_CommandPool->GetGraphicsContext();
    cmd->Reset();
    // ディスクリプタヒープのセット
    cmd->SetDescriptorHeap(m_DescriptorAllocator->GetDescriptorHeap(HeapType::CBV_SRV_UAV));
    // ビューポートとシザー矩形の設定
    D3D12_VIEWPORT viewport{
        0.0f, 0.0f,
        static_cast<float>(Platform::WinApp::m_WindowWidth),
        static_cast<float>(Platform::WinApp::m_WindowHeight),
        0.0f, 1.0f
    };
    cmd->SetViewport(viewport);
    D3D12_RECT rect{
        0, 0,
        static_cast<LONG>(Platform::WinApp::m_WindowWidth),
        static_cast<LONG>(Platform::WinApp::m_WindowHeight)
    };
    cmd->SetScissorRect(rect);
    // プリミティブトポロジーの設定
    cmd->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // バリア遷移 RenderTargetに遷移 → 描画 → バリア遷移 Presentに遷移
    cmd->BarrierTransition(
        backBuffer.pResource.get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    // レンダーターゲットの設定
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_DescriptorAllocator->GetCPUHandle(backBuffer.rtvTableID);
    cmd->SetRenderTargets(1, &handle, false, nullptr);
    // レンダーターゲットのクリア
    cmd->ClearRenderTargetView(handle, Config::Graphics::kClearColor, 0, nullptr);
#ifndef NDEBUG // デバッグ、開発用
    // ImGuiの描画
    if(m_ImGuiManager)
    {
        m_ImGuiManager->Draw(*cmd);
    }
#else
    // パイプライン、ルートシグネチャの設定
    // finalRenderTextureをセット
    // DrawCall
#endif
    cmd->BarrierTransition(
        backBuffer.pResource.get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    // コマンドリストのクローズ
    cmd->Close();
    // コマンドリストの実行 + signal
    GraphicsQueueContext* graphicsQueue = m_Device->m_QueuePool->GetPresentQueue();
    graphicsQueue->Execute(cmd);
    // GPU完了待ち
    graphicsQueue->WaitForFence();
    // Presentキューは返却不要
    // コマンドコンテキストの返却
    m_CommandPool->ReturnContext(cmd);
    // スワップチェーンのPresent
    GraphicsQueueContext* presentQueue = m_Device->m_QueuePool->GetPresentQueue();
    if (Config::Graphics::EnableVSync)
    {
        // VSync有効
        m_Device->m_SwapChainContext.m_SwapChain->Present(1, 0);
    }
    else
    {
        // VSync無効
        m_Device->m_SwapChainContext.m_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    }
    presentQueue->Flush();
}
