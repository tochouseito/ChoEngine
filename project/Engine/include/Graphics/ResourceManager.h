#pragma once
#include <memory>
#include "include/Graphics/GpuBuffer.h"
#include "include/Utility/atomic_shared_ptr.h"
#include "include/Utility/FVector.h"
#include <typeindex>

namespace Theatria::Graphics
{
    class RenderDevice;
    class DescriptorAllocator;

    template <typename T>
    using FVector = Theatria::Utility::FVector<T>;
    template <typename T>
    using atomic_shared_ptr = Theatria::Utility::atomic_shared_ptr<T>;

    class ResourceManager final
    {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        /// @brief 初期化
        /// @param device 
        /// @param descAllocator 
        /// @return 
        [[nodiscard]]
        bool Initialize(RenderDevice* device, DescriptorAllocator* descAllocator);

        /*=============== CreateResources ===============*/
        template<typename T>
        [[nodiscard]]
        uint32_t CreateConstantBuffer()
        {
            auto buffer = std::make_shared<ConstantBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice());
            uint32_t idx = static_cast<uint32_t>(m_Buffers.emplace_back(buffer));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateStructuredBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<StructuredBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_Buffers.emplace_back(buffer));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateRWStructuredBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<RWStructuredBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_Buffers.emplace_back(buffer));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateVertexBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<VertexBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_Buffers.emplace_back(buffer));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateIndexBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<IndexBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_Buffers.emplace_back(buffer));
            return idx;
        }
        /*=============== TextureBuffer ===============*/
        [[nodiscard]]
        uint32_t CreateTextureBuffer(D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES& state)
        {
            auto buffer = std::make_shared<TextureBuffer>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), desc, clearValue, state);
            uint32_t idx = static_cast<uint32_t>(m_TextureBuffers.emplace_back(buffer));
            return idx;
        }
        [[nodiscard]]
        uint32_t CreateDepthBuffer(D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES& state)
        {
            auto buffer = std::make_shared<DepthBuffer>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), desc, state);
            uint32_t idx = static_cast<uint32_t>(m_TextureBuffers.emplace_back(buffer));
            return idx;
        }

        /*=============== GetBuffer ===============*/
        GpuBuffer* GetBuffer(uint32_t idx) noexcept
        {
            std::lock_guard<std::mutex> lock(m_BufferMutex);
            return m_Buffers[idx].load().get();
        }
        TextureBuffer* GetTextureBuffer(uint32_t idx) noexcept
        {
            std::lock_guard<std::mutex> lock(m_TextureBufferMutex);
            return m_TextureBuffers[idx].load().get();
        }
        template<typename GpuBufferType, typename T>
        std::span<T> GetUploadBufferMappedData(uint32_t idx)
        {
            GpuBuffer* buffer = GetBuffer(idx);
            if (!buffer)
            {
                Core::LogAssert::Check(false, "ResourceManager", "GetBufferMappedData: Buffer is null");
                return {};
            }
            auto elementType = buffer->GetElementType();
            auto bufferType = buffer->GetBufferType();
            if (elementType != typeid(T))
            {
                Core::LogAssert::Check(false, "ResourceManager", "miss match elementType");
            }
            if (bufferType == typeid(ConstantBuffer))
            {
                ConstantBuffer<T> derivedBuffer = dynamic_cast<ConstantBuffer<T>>(buffer);
                return derivedBuffer.GetUploadMappedData();
            }
            else if (bufferType == typeid(StructuredBuffer))
            {
                StructuredBuffer<T> derivedBuffer = dynamic_cast<StructuredBuffer<T>>(buffer);
                return derivedBuffer.GetUploadMappedData();
            }
            else if (bufferType == typeid(VertexBuffer))
            {
                VertexBuffer<T> derivedBuffer = dynamic_cast<VertexBuffer<T>>(buffer);
                return derivedBuffer.GetUploadMappedData();
            }
            else if (bufferType == typeid(IndexBuffer))
            {
                IndexBuffer<T> derivedBuffer = dynamic_cast<IndexBuffer<T>>(buffer);
                return derivedBuffer.GetUploadMappedData();
            }
            else
            {
                Core::LogAssert::Check(false, "ResourceManager", "GetUploadBufferMappedData: Unsupported buffer type");
                return {};
            }
        }
        template<typename GpuBufferType, typename T>
        std::span<const T> GetReadbackBufferMappedData(uint32_t idx)
        {
            GpuBuffer* buffer = GetBuffer(idx);
            if (!buffer)
            {
                Core::LogAssert::Check(false, "ResourceManager", "GetBufferMappedData: Buffer is null");
                return {};
            }
            auto elementType = buffer->GetElementType();
            auto bufferType = buffer->GetBufferType();
            if (elementType != typeid(T))
            {
                Core::LogAssert::Check(false, "ResourceManager", "miss match elementType");
            }
            if (bufferType == typeid(RWStructuredBuffer))
            {
                RWStructuredBuffer<const T> derivedBuffer = dynamic_cast<RWStructuredBuffer<T>>(buffer);
                return derivedBuffer.GetReadBackMappedData();
            }
            else
            {
                Core::LogAssert::Check(false, "ResourceManager", "GetReadbackBufferMappedData: Unsupported buffer type");
                return {};
            }
        }
    private:
        RenderDevice* m_pDevice = nullptr; ///< レンダーデバイス
        DescriptorAllocator* m_pDescAllocator = nullptr; ///< ディスクリプタヒープアロケータ

        /*=============== バッファ群 ===============*/
        FVector<atomic_shared_ptr<GpuBuffer>> m_Buffers;
        std::mutex m_BufferMutex;
        /*=============== テクスチャバッファ群 ===============*/
        FVector<atomic_shared_ptr<TextureBuffer>> m_TextureBuffers;
        std::mutex m_TextureBufferMutex;
    };
};
