#include "pch.h"
#include "include/Graphics/ResourceManager.h"
#include "config/engineConfig.h"
#include "include/Core/LogAssert.h"

bool Theatria::Graphics::ResourceManager::Initialize(RenderDevice* device, DescriptorAllocator* descAllocator)
{
    m_pDevice = device;
    m_pDescAllocator = descAllocator;

    return true;
}
