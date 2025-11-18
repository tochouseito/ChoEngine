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
    CreateDepthBuffer();
    return true;
}

void Theatria::Graphics::Renderer::CreateDepthBuffer()
{
    
}

void Theatria::Graphics::Renderer::CreateDepthBufferDebug()
{
}
