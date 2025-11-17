#include "pch.h"
#include "include/Graphics/Renderer.h"
#include "include/Graphics/RenderDevice.h"

/// @brief 初期化
[[nodiscard]]
bool Theatria::Graphics::Renderer::Initialize(RenderDevice* renderDevice)
{
    m_Device = renderDevice;
    m_CommandPool = std::make_unique<CommandPool>(m_Device->m_Device.Get());
    return true;
}
