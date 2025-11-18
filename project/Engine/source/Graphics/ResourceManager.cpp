#include "pch.h"
#include "include/Graphics/ResourceManager.h"

bool Theatria::Graphics::ResourceManager::Initialize(RenderDevice* device, DescriptorAllocator* descAllocator)
{
    m_pDevice = device;
    m_pDescAllocator = descAllocator;


    return true;
}
