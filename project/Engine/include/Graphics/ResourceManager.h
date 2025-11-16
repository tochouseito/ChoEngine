#pragma once
#include <memory>
#include "include/Graphics/GpuBuffer.h"
#include "include/Utility/atomic_shared_ptr.h"
#include "include/Utility/FVector.h"
#include <typeindex>

namespace Theatria::Graphics
{
    template <typename T>
    using FVector = Theatria::Utility::FVector<T>;
    template <typename T>
    using atomic_shared_ptr = Theatria::Utility::atomic_shared_ptr<T>;

    class ResourceManager final
    {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        /*=============== CreateResources ===============*/
        template<typename GpuBufferType, typename T>
        uint32_t CreateBuffer()
        {
            if constexpr (std::is_same_v < GpuBufferType, ConstantBuffer)
            {
                uint32_t index = static_cast<uint32_t>(m_Buffers.emplace_back(std::make_shared<ConstantBuffer<T>>()));
                std::weak_ptr<GpuBuffer> buffer = m_Buffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateBuffer();
                }
                return index;
            }
            else if constexpr (std::is_same_v <GpuBufferType, StructuredBuffer>)
            {
                uint32_t index = static_cast<uint32_t>(m_Buffers.emplace_back(std::make_shared<StructuredBuffer<T>>()));
                std::weak_ptr<GpuBuffer> buffer = m_Buffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateStructuredBufferResource();
                }
                return index;
            }
            else if constexpr (std::is_same_v<GpuBufferType, RWStructuredBuffer>)
            {
                uint32_t index = static_cast<uint32_t>(m_Buffers.emplace_back(std::make_shared<RWStructuredBuffer<T>>()));
                std::weak_ptr<GpuBuffer> buffer = m_Buffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateRWStructuredBufferResource();
                }
                return index;
            }
            else if constexpr (std::is_same_v<GpuBufferType, VertexBuffer>)
            {
                uint32_t index = static_cast<uint32_t>(m_Buffers.emplace_back(std::make_shared<VertexBuffer<T>>()));
                std::weak_ptr<GpuBuffer> buffer = m_Buffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateVertexBufferResource();
                }
                return index;
            }
            else if constexpr (std::is_same_v<GpuBufferType, IndexBuffer>)
            {
                uint32_t index = static_cast<uint32_t>(m_Buffers.emplace_back(std::make_shared<IndexBuffer<T>>()));
                std::weak_ptr<GpuBuffer> buffer = m_Buffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateIndexBufferResource();
                }
                return index;
            }
            else
            {
                static_assert(false, "Unsupported GpuBufferType");
            }
        }
        /*=============== TextureBuffer ===============*/
        template<typename TextureBufferType>
        uint32_t CreateTextureBuffer()
        {
            if constexpr (std::is_same_v < TextureBufferType, TextureBuffer>)
            {
                uint32_t index = static_cast<uint32_t>(m_TextureBuffers.emplace_back(std::make_shared<TextureBuffer>()));
                std::weak_ptr<TextureBuffer> buffer = m_TextureBuffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateTextureBufferResource();
                }
                return index;
            }
            else if constexpr (std::is_same_v<TextureBufferType, DepthBuffer>)
            {
                uint32_t index = static_cast<uint32_t>(m_TextureBuffers.emplace_back(std::make_shared<DepthBuffer>()));
                std::weak_ptr<TextureBuffer> buffer = m_TextureBuffers[index].load();
                if (auto ptr = buffer.lock())
                {
                    ptr->CreateDepthBufferResource();
                }
                return index;
            }
            else
            {
                static_assert(false, "Unsupported TextureBufferType");
            }
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
        /*=============== バッファ群 ===============*/
        FVector<atomic_shared_ptr<GpuBuffer>> m_Buffers;
        std::mutex m_BufferMutex;
        /*=============== テクスチャバッファ群 ===============*/
        FVector<atomic_shared_ptr<TextureBuffer>> m_TextureBuffers;
        std::mutex m_TextureBufferMutex;
    };
};
