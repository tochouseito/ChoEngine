#include "pch.h"
#include "include/Graphics/ResourceManager.h"
#include "config/engineConfig.h"
#include "include/Core/LogAssert.h"

bool Theatria::Graphics::ResourceManager::Initialize(RenderDevice* device, DescriptorAllocator* descAllocator)
{
    m_pDevice = device;
    m_pDescAllocator = descAllocator;
    CreateGlobalBuffers();

    return true;
}

void Theatria::Graphics::ResourceManager::CreateGlobalBuffers()
{
#ifndef NDEBUG
    std::lock_guard debuglock(m_DebugVPMutex);
#endif // !NDEBUG

    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        m_GlobalObjectBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 1024);
        m_GlobalTransformBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 1024);
        m_GlobalModelInfoBuffer.Create(m_pDevice->GetDevice(), m_pDescAllocator, 256);
#ifndef NDEBUG
        m_DebugVP[i].CreateBuffer(m_pDevice->GetDevice());
#endif
    }
#ifndef NDEBUG
    m_DebugVPUploadBuffer.CreateBuffer(m_pDevice->GetDevice(), 1);
#endif
}
