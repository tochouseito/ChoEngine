#include "pch.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Core/LogAssert.h"

[[nodiscard]]
bool Theatria::Graphics::DescriptorAllocator::Initialize(RenderDevice*)
{
    for (size_t i = 0; i < static_cast<size_t>(HeapType::kCount); i++)
    {
        D3D12_DESCRIPTOR_HEAP_TYPE heapType;
        switch(static_cast<HeapType>(i))
        {
        case HeapType::CBV_SRV_UAV:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            break;
        case HeapType::SAMPLER:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            break;
        case HeapType::RTV:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            break;
        case HeapType::DSV:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            break;
        default:
            Core::LogAssert::Check(false, "DescriptorAllocator", "Unknown HeapType");
            return false;// 不明なタイプ
        }
    }
    return true;
}
