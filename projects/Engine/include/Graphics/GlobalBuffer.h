#pragma once

#include "include/Graphics/GpuBuffer.h"
#include "include/Graphics/DescriptorAllocator.h"
#include "config/engineConfig.h"
#include <array>
#include <mutex>
namespace Theatria::Graphics
{
    enum class GlobalBufferType : uint32_t
    {
        ObjectBuffer = 0,
        TransformBuffer = 1,
        MeshInfoBuffer = 2
    };

    /// @brief 
    /// @tparam T 
    template <typename T>
    class GlobalBuffer final
    {
    public:
        GlobalBuffer() = default;
        ~GlobalBuffer() = default;

        void Create(ID3D12Device* device, DescriptorAllocator* da, UINT numElements)
        {
            std::scoped_lock lock(
                m_Mutexes[0],
                m_Mutexes[1],
                m_Mutexes[2]);
            for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
            {
                m_Buffers[i].CreateBuffer(device, numElements);
                m_DescriptorTableIDs[i] = da->Allocate(DescriptorAllocator::TableKind::Buffers);
                da->CreateSRVBuffer(m_DescriptorTableIDs[i], &m_Buffers[i]);
            }
            m_UploadBuffer.CreateBuffer(device, numElements);
        }

        [[nodiscard]]
        uint32_t Allocate() noexcept
        {
            if (!m_FreeList.empty())
            {
                uint32_t index = m_FreeList.back();
                m_FreeList.pop_back();
                return index;
            }
            else
            {
                m_TotalCount++;
                return m_NextIndex++;
            }
        }

        void Free(uint32_t index) noexcept
        {
            m_FreeList.push_back(index);
            m_TotalCount--;
        }

        StructuredBuffer<T>& GetBuffer(uint32_t frameIndex) noexcept
        {
            return m_Buffers[frameIndex];
        }

        GpuBuffer& GetGpuBuffer(uint32_t frameIndex) noexcept
        {
            return m_Buffers[frameIndex];
        }

        DescriptorAllocator::TableID GetDescriptorTableID(uint32_t frameIndex) noexcept
        {
            return m_DescriptorTableIDs[frameIndex];
        }

        UploadBuffer<T>& GetUploadBuffer() noexcept
        {
            return m_UploadBuffer;
        }

        uint32_t GetTotalCount() const noexcept
        {
            return m_TotalCount;
        }

    private:
        std::array<StructuredBuffer<T>, Config::Graphics::kMaxBufferingCount> m_Buffers;
        std::array<std::mutex, Config::Graphics::kMaxBufferingCount> m_Mutexes;
        std::array<DescriptorAllocator::TableID, Config::Graphics::kMaxBufferingCount> m_DescriptorTableIDs;
        UploadBuffer<T> m_UploadBuffer;
        std::vector<uint32_t> m_FreeList{};
        uint32_t m_NextIndex{ 0 };
        uint32_t m_TotalCount{ 0 };
    };
}

