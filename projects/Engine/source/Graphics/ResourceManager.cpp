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
    std::scoped_lock lock(
        m_GlobalObjectBuffer.mutex[0],
        m_GlobalObjectBuffer.mutex[1],
        m_GlobalObjectBuffer.mutex[2],
        m_GlobalTransformBuffer.mutex[0],
        m_GlobalTransformBuffer.mutex[1],
        m_GlobalTransformBuffer.mutex[2],
        m_GlobalModelInfoBuffer.mutex[0],
        m_GlobalModelInfoBuffer.mutex[1],
        m_GlobalModelInfoBuffer.mutex[2],
        m_IndirectCommandCountBufferMutex[0],
        m_IndirectCommandCountBufferMutex[1],
        m_IndirectCommandCountBufferMutex[2]
    );
#ifndef NDEBUG
    std::lock_guard debuglock(m_DebugVPMutex);
#endif // !NDEBUG

    for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
    {
        m_GlobalObjectBuffer.buffers[i].Create(m_pDevice->GetDevice(), 1024);
        m_GlobalObjectBuffer.descriptorIDs[i] = m_pDescAllocator->Allocate(DescriptorAllocator::TableKind::Buffers);
        m_pDescAllocator->CreateSRVBuffer(m_GlobalObjectBuffer.descriptorIDs[i], &m_GlobalObjectBuffer.buffers[i].GetBuffer());

        m_GlobalTransformBuffer.buffers[i].Create(m_pDevice->GetDevice(), 1024);
        m_GlobalTransformBuffer.descriptorIDs[i] = m_pDescAllocator->Allocate(DescriptorAllocator::TableKind::Buffers);
        m_pDescAllocator->CreateSRVBuffer(m_GlobalTransformBuffer.descriptorIDs[i], &m_GlobalTransformBuffer.buffers[i].GetBuffer());

        m_GlobalModelInfoBuffer.buffers[i].Create(m_pDevice->GetDevice(), 256);
        m_GlobalModelInfoBuffer.descriptorIDs[i] = m_pDescAllocator->Allocate(DescriptorAllocator::TableKind::Buffers);
        m_pDescAllocator->CreateSRVBuffer(m_GlobalModelInfoBuffer.descriptorIDs[i], &m_GlobalModelInfoBuffer.buffers[i].GetBuffer());

        m_IndirectCommandCountBuffer[i].CreateBuffer(m_pDevice->GetDevice(), 1);
        m_IndirectCommandCountBufferDescriptorIDs[i] = m_pDescAllocator->Allocate(DescriptorAllocator::TableKind::Buffers);
        m_pDescAllocator->CreateUAVRawBuffer(m_IndirectCommandCountBufferDescriptorIDs[i], &m_IndirectCommandCountBuffer[i]);
#ifndef NDEBUG
        m_DebugVP[i].CreateBuffer(m_pDevice->GetDevice());
        m_DebugVP[i].CreateUploadBuffer(m_pDevice->GetDevice());
#endif
    }
}
