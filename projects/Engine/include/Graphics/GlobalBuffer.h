#pragma once

#include "include/Graphics/GpuBuffer.h"
namespace Theatria::Graphics
{
    /// @brief 
    /// @tparam T 
    template <typename T>
    class GlobalBuffer final
    {
    public:
        GlobalBuffer() = default;
        ~GlobalBuffer() = default;

        void Create(ID3D12Device* device, UINT numElements)
        {
            m_Buffer.CreateBuffer(device, numElements);
            m_Buffer.CreateUploadBuffer(device, numElements);
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
                return m_NextIndex++;
            }
        }

        void Free(uint32_t index) noexcept
        {
            m_FreeList.push_back(index);
        }

        StructuredBuffer<T>& GetBuffer() noexcept { return m_Buffer; }

    private:
        StructuredBuffer<T> m_Buffer;
        size_t m_Size{};
        std::vector<uint32_t> m_FreeList{};
        uint32_t m_NextIndex{ 0 };
    };
}

