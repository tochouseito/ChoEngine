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
    std::scoped_lock lock(m_ObjectBufferMutex, m_TransformBufferMutex, m_ModelInfoBufferMutex, m_DebugVPMutex);

    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        m_ObjectBuffer[i].Create(m_pDevice->GetDevice(), 1024);
        m_TransformBuffer[i].Create(m_pDevice->GetDevice(), 1024);
        m_ModelInfoBuffer[i].Create(m_pDevice->GetDevice(), 256);
        m_DebugVP[i].CreateBuffer(m_pDevice->GetDevice());
        m_DebugVP[i].CreateUploadBuffer(m_pDevice->GetDevice());
    }
}
