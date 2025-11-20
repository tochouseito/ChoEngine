#include "pch.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/ResourceManager.h"
#include "include/Graphics/GraphicsSetting.h"

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
