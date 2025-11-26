#include "pch.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/GraphicsSetting.h"
#include "include/Graphics/FrameGraph.h"

/// @brief 初期化
[[nodiscard]]
bool Theatria::Graphics::Renderer::Initialize(RenderDevice* renderDevice, ResourceManager* resourceManager)
{
    m_Device = renderDevice;
    m_ResourceManager = resourceManager;
    m_CommandPool = std::make_unique<CommandPool>(m_Device->m_Device.Get());
    // 深度バッファの作成
    m_DepthBufferIndex = m_ResourceManager->CreateDepthBuffer();
#ifndef NDEBUG // デバッグ、開発用
    m_DebugDepthBufferIndex = m_ResourceManager->CreateDepthBuffer();
#endif
    return true;
}

/// @brief 描画開始
/// @return 
Theatria::Graphics::GraphicsCommandContext* Theatria::Graphics::Renderer::BeginRenderPass() noexcept
{
    GraphicsCommandContext* cmd = m_CommandPool->GetGraphicsContext();
    cmd->Reset();
    return cmd;
}

/// @brief 描画終了
void Theatria::Graphics::Renderer::EndRenderPass(GraphicsCommandContext* cmd) noexcept
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

void Theatria::Graphics::Renderer::ApplyBarriers(FrameGraph& fg, const std::vector<BarrierInfo>& barriers)
{
    for (auto& barrier : barriers)
    {
        const VirtualResource& vres = fg.GetVirtualResource(barrier.handle);
    }
}

void Theatria::Graphics::Renderer::Present()
{
    GraphicsCommandContext* cmd = m_CommandPool->GetGraphicsContext();
    cmd->Reset();
    // バリア遷移 RenderTargetに遷移 → 描画 → バリア遷移 Presentに遷移
    //
    //
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
    // スワップチェーンのPresent
    GraphicsQueueContext* presentQueue = m_Device->m_QueuePool->GetPresentQueue();
    if (Setting::EnableVSync)
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
