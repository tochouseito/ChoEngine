#include "pch.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "include/Core/LogAssert.h"
#include "include/Graphics/RenderDevice.h"

using namespace Theatria::Graphics;

/// @brief 初期化
[[nodiscard]]
bool Theatria::Graphics::DescriptorAllocator::Initialize(RenderDevice* pRenderDevice, uint32_t texCap, uint32_t bufCap, uint32_t rtCap, uint32_t dsCap)
{
    m_pRenderDevice = pRenderDevice;
    for (size_t i = 0; i < static_cast<size_t>(HeapType::kCount); i++)
    {
        D3D12_DESCRIPTOR_HEAP_TYPE heapType;
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        switch (static_cast<HeapType>(i))
        {
        case HeapType::CBV_SRV_UAV:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            m_DescriptorSizes[i] = m_pRenderDevice->m_Device->GetDescriptorHandleIncrementSize(heapType);
            desc.Type = heapType;
            desc.NumDescriptors = texCap + bufCap;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            Core::LogAssert::Verify(
                m_pRenderDevice->m_Device->CreateDescriptorHeap(
                    &desc, IID_PPV_ARGS(&m_DescriptorHeaps[i])), "DescriptorAllocator", "Failed CreateDescriptorHeap");
            m_Buffers.heapType = HeapType::CBV_SRV_UAV;
            m_Textures.heapType = HeapType::CBV_SRV_UAV;
            // 空きスロットを全登録
            m_Textures.capacity = texCap;
            m_Textures.freeList.reserve(m_Textures.capacity);
            for (uint32_t j = 0; j < texCap; ++j)
            {
                m_Textures.freeList.push_back(m_Textures.capacity - 1 - j);
            }
            m_Buffers.capacity = bufCap;
            m_Buffers.freeList.reserve(m_Buffers.capacity);
            for (uint32_t j = 0; j < bufCap; ++j)
            {
                m_Buffers.freeList.push_back(m_Buffers.capacity - 1 - j);
            }
            break;
        case HeapType::SAMPLER:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            // サンプラーは未対応
            break;
        case HeapType::RTV:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            m_DescriptorSizes[i] = m_pRenderDevice->m_Device->GetDescriptorHandleIncrementSize(heapType);
            desc.Type = heapType;
            desc.NumDescriptors = rtCap;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            Core::LogAssert::Verify(
                m_pRenderDevice->m_Device->CreateDescriptorHeap(
                    &desc, IID_PPV_ARGS(&m_DescriptorHeaps[i])), "DescriptorAllocator", "Failed CreateDescriptorHeap");
            m_RenderTargets.heapType = HeapType::RTV;
            // 空きスロットを全登録
            m_RenderTargets.capacity = rtCap;
            m_RenderTargets.freeList.reserve(m_RenderTargets.capacity);
            for (uint32_t j = 0; j < rtCap; ++j)
            {
                m_RenderTargets.freeList.push_back(m_RenderTargets.capacity - 1 - j);
            }
            break;
        case HeapType::DSV:
            heapType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            m_DescriptorSizes[i] = m_pRenderDevice->m_Device->GetDescriptorHandleIncrementSize(heapType);
            desc.Type = heapType;
            desc.NumDescriptors = dsCap;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            Core::LogAssert::Verify(
                m_pRenderDevice->m_Device->CreateDescriptorHeap(
                    &desc, IID_PPV_ARGS(&m_DescriptorHeaps[i])), "DescriptorAllocator", "Failed CreateDescriptorHeap");
            m_DepthStencils.heapType = HeapType::DSV;
            // 空きスロットを全登録
            m_DepthStencils.capacity = dsCap;
            m_DepthStencils.freeList.reserve(m_DepthStencils.capacity);
            for (uint32_t j = 0; j < dsCap; ++j)
            {
                m_DepthStencils.freeList.push_back(m_DepthStencils.capacity - 1 - j);
            }
            break;
        default:
            Core::LogAssert::Check(false, "DescriptorAllocator", "Unknown HeapType");
            return false;// 不明なタイプ
        }
    }
    return true;
}

DescriptorAllocator::TableID Theatria::Graphics::DescriptorAllocator::Allocate(TableKind k)
{
    Table& t = GetTable(k);
    if (t.freeList.empty())
    {
        Core::LogAssert::Check(false, "DescriptorAllocator", "Descriptor table full, need to expand");
        EnsureCapacity(k, /*needOneMore=*/1);
    }
    auto idx = t.freeList.back(); t.freeList.pop_back();
    return TableID{ k, t.generation, idx };
}

void Theatria::Graphics::DescriptorAllocator::Free(const TableID& id)
{
    Table& t = GetTable(id.kind);
    t.freeList.push_back(id.index);
    // 任意：ヌルSRV/UAVを書いておくと安全
}

void Theatria::Graphics::DescriptorAllocator::CreateSRVTexture2D(TableID& id, ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
    auto cpuH = GetCPUHandle(id);
    m_pRenderDevice->m_Device->CreateShaderResourceView(res, &desc, cpuH);
}

void Theatria::Graphics::DescriptorAllocator::CreateSRVBuffer(TableID& id, ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
    auto cpuH = GetCPUHandle(id);
    m_pRenderDevice->m_Device->CreateShaderResourceView(res, &desc, cpuH);
}

void Theatria::Graphics::DescriptorAllocator::CreateUAVBuffer(TableID& id, ID3D12Resource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
    auto cpuH = GetCPUHandle(id);
    m_pRenderDevice->m_Device->CreateUnorderedAccessView(res, nullptr, &desc, cpuH);
}

void Theatria::Graphics::DescriptorAllocator::CreateRTV(TableID& id, ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
{
    m_pRenderDevice->m_Device->CreateRenderTargetView(res, &desc, GetCPUHandle(id));
}

D3D12_GPU_DESCRIPTOR_HANDLE Theatria::Graphics::DescriptorAllocator::GetTableBaseGPU(TableKind k) 
{
    Table& t = GetTable(k);
    D3D12_GPU_DESCRIPTOR_HANDLE h = m_DescriptorHeaps[static_cast<size_t>(t.heapType)]->GetGPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<size_t>(m_DescriptorSizes[static_cast<size_t>(t.heapType)]) * (t.baseIndex);
    return h;
}

D3D12_GPU_DESCRIPTOR_HANDLE Theatria::Graphics::DescriptorAllocator::GetGPUHandle(TableID& id)
{
    Table& t = GetTable(id.kind);
    D3D12_GPU_DESCRIPTOR_HANDLE h = m_DescriptorHeaps[static_cast<size_t>(t.heapType)]->GetGPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<size_t>(m_DescriptorSizes[static_cast<size_t>(t.heapType)]) * (t.baseIndex + id.index);
    return h;
}

D3D12_CPU_DESCRIPTOR_HANDLE Theatria::Graphics::DescriptorAllocator::GetCPUHandle(TableID& id)
{
    Table& t = GetTable(id.kind);
    D3D12_CPU_DESCRIPTOR_HANDLE h = m_DescriptorHeaps[static_cast<size_t>(t.heapType)]->GetCPUDescriptorHandleForHeapStart();
    h.ptr += static_cast<size_t>(m_DescriptorSizes[static_cast<size_t>(t.heapType)]) * (t.baseIndex + id.index);
    return h;
}

void Theatria::Graphics::DescriptorAllocator::EnsureCapacity(TableKind k, uint32_t needOneMore)
{
    needOneMore;
    // 増やす対象を2倍（or +N）する
    switch (k)
    {
    case Theatria::Graphics::DescriptorAllocator::TableKind::Textures:
    {
        /*uint32_t newTex = m_Textures.capacity;
        uint32_t newBuf = m_Buffers.capacity;
        newTex = std::max(1u, newTex * 2);
        RecreateHeap(newTex, newBuf);*/
    }
        break;
    case Theatria::Graphics::DescriptorAllocator::TableKind::Buffers:
    {
        /*uint32_t newTex = m_Textures.capacity;
        uint32_t newBuf = m_Buffers.capacity;
        newBuf = std::max(1u, newBuf * 2);
        RecreateHeap(newTex, newBuf);*/
    }
        break;
    case Theatria::Graphics::DescriptorAllocator::TableKind::RenderTargets:
    {
        uint32_t newRT = m_RenderTargets.capacity;
        newRT = std::max(1u, newRT * 2);

    }
        break;
    case Theatria::Graphics::DescriptorAllocator::TableKind::DepthStencils:
    {
        uint32_t newDS = m_DepthStencils.capacity;
        newDS = std::max(1u, newDS * 2);
    }
        break;
    default:
        break;
    }
}

void Theatria::Graphics::DescriptorAllocator::RecreateHeap(TableKind k, uint32_t newCap, uint32_t newBufCap)
{
    k; newCap; newBufCap;

    //// 旧ヒープを退役リストに（フェンスで寿命管理するのが理想）
    //if (m_heap)
    //{
    //    m_retired.push_back({ m_heap, /*fenceValue*/ 0 }); // ★フェンス値を外から渡す設計推奨
    //}

    //// 新ヒープ作成
    //D3D12_DESCRIPTOR_HEAP_DESC desc{};
    //desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    //desc.NumDescriptors = newTexCap + newBufCap;
    //desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //desc.NodeMask = 0;
    //HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap));
    //Core::LogAssert::Verify(hr, "DescriptorHeapManager", "CreateDescriptorHeap failed");

    //// 新ブロックの配置
    //UINT64 baseCPU = m_heap->GetCPUDescriptorHandleForHeapStart().ptr;
    //UINT64 baseGPU = m_heap->GetGPUDescriptorHandleForHeapStart().ptr;

    //uint32_t texBase = 0;
    //uint32_t bufBase = newTexCap; // テクスチャの後ろにバッファ

    //// 旧ヒープからコピー（「同じローカルindex」に置く）
    //auto copyRange = [&](Table& oldT, uint32_t newBase, uint32_t newCap) {
    //    if (oldT.capacity == 0) return;
    //    // 旧ヒープの base と increment を仮に滞在中として解決
    //    // （本実装では old heap を覚えておく必要あり。ここでは簡略化のため省略記述）
    //    // 実装のコツ：前回の baseCPU/GPU や旧heapをメンバに保持しておき、
    //    //             CopyDescriptorsSimple で [old.base + i] -> [newBase + i] をループでコピー。
    //    };

    //copyRange(m_textures, texBase, newTexCap);
    //copyRange(m_buffers, bufBase, newBufCap);

    //// baseIndex / capacity / generation を更新
    //m_textures.baseIndex = texBase;
    //m_textures.capacity = newTexCap;
    //m_textures.generation++; // ブロックが動いたので世代++
    //// 空きリストは「既存割り当て以外」のスロットをpushし直すのが正（ここでは簡略）

    //m_buffers.baseIndex = bufBase;
    //m_buffers.capacity = newBufCap;
    //m_buffers.generation++;
}

DescriptorAllocator::Table& Theatria::Graphics::DescriptorAllocator::GetTable(TableKind k)
{
    switch (k)
    {
    case Theatria::Graphics::DescriptorAllocator::TableKind::Textures:
        return m_Textures;
        break;
    case Theatria::Graphics::DescriptorAllocator::TableKind::Buffers:
        return m_Buffers;
        break;
    case Theatria::Graphics::DescriptorAllocator::TableKind::RenderTargets:
        return m_RenderTargets;
        break;
    case Theatria::Graphics::DescriptorAllocator::TableKind::DepthStencils:
        return m_DepthStencils;
        break;
    default:
        Core::LogAssert::Check(false, "DescriptorAllocator", "Unknown TableKind");
        break;
    }
    return m_Textures; // 適当
}
