#pragma once
#include <memory>
#include "include/Graphics/GpuBuffer.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Graphics/GraphicsSetting.h"
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
            std::lock_guard<std::mutex> lock(m_MultiBufferMutex);
            std::array<atomic_shared_ptr<GpuBuffer>, Graphics::Setting::kMaxBufferingCount> bufferSet{};
            for (uint32_t i = 0; i < Graphics::Setting::BufferingCount; i++)
            {
                bufferSet[i] = std::make_shared<ConstantBuffer<T>>();
                bufferSet[i]->CreateBuffer(m_pDevice->GetDevice());
            }
            uint32_t idx = static_cast<uint32_t>(m_MultiBuffers.emplace_back(bufferSet));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateStructuredBuffer(uint32_t numElements)
        {
            std::lock_guard<std::mutex> lock(m_MultiBufferMutex);
            std::array<atomic_shared_ptr<GpuBuffer>, Graphics::Setting::kMaxBufferingCount> bufferSet{};
            for (uint32_t i = 0; i < Graphics::Setting::BufferingCount; i++)
            {
                bufferSet[i] = std::make_shared<StructuredBuffer<T>>();
                bufferSet[i]->CreateBuffer(m_pDevice->GetDevice(), numElements);
            }
            uint32_t idx = static_cast<uint32_t>(m_MultiBuffers.emplace_back(bufferSet));
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateRWStructuredBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<RWStructuredBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_SingleBuffers.emplace_back(buffer));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateVertexBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<VertexBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_SingleBuffers.emplace_back(buffer));
            return idx;
        }
        template<typename T>
        [[nodiscard]]
        uint32_t CreateIndexBuffer(uint32_t numElements)
        {
            auto buffer = std::make_shared<IndexBuffer<T>>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), numElements);
            uint32_t idx = static_cast<uint32_t>(m_SingleBuffers.emplace_back(buffer));
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
        uint32_t CreateRenderTargetBuffer(uint32_t width, uint32_t height, DXGI_FORMAT format)
        {
            // RenderTargetBufferの生成
            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.MipLevels = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Format = format;
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            // クリア値の設定
            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = format;
            clearValue.Color[0] = Setting::kClearColor[0];
            clearValue.Color[1] = Setting::kClearColor[1];
            clearValue.Color[2] = Setting::kClearColor[2];
            clearValue.Color[3] = Setting::kClearColor[3];
            auto buffer = std::make_shared<TextureBuffer>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), resourceDesc, &clearValue, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            uint32_t idx = static_cast<uint32_t>(m_TextureBuffers.emplace_back(buffer));
            return idx;
        }
        [[nodiscard]]
        uint32_t CreateDepthBuffer(uint32_t width, uint32_t height)
        {
            // DepthBufferの生成
            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.MipLevels = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            auto buffer = std::make_shared<DepthBuffer>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            uint32_t idx = static_cast<uint32_t>(m_TextureBuffers.emplace_back(buffer));
            return idx;
        }

        /*=============== GetBuffer ===============*/
        GpuBuffer* GetSingleBuffer(uint32_t idx) noexcept
        {
            std::lock_guard<std::mutex> lock(m_SingleBufferMutex);
            return m_SingleBuffers[idx].load().get();
        }
        GpuBuffer* GetMultiBuffer(uint32_t bufferIdx, uint32_t frameIdx) noexcept
        {
            std::lock_guard<std::mutex> lock(m_MultiBufferMutex);
            return m_MultiBuffers[bufferIdx][frameIdx].load().get();
        }
        TextureBuffer* GetTextureBuffer(uint32_t idx) noexcept
        {
            std::lock_guard<std::mutex> lock(m_TextureBufferMutex);
            return m_TextureBuffers[idx].load().get();
        }
        /*template<typename GpuBufferType, typename T>
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
        }*/
    private:
        RenderDevice* m_pDevice = nullptr; ///< レンダーデバイス
        DescriptorAllocator* m_pDescAllocator = nullptr; ///< ディスクリプタヒープアロケータ

        /*=============== シングルバッファ群 ===============*/
        FVector<atomic_shared_ptr<GpuBuffer>> m_SingleBuffers;
        std::mutex m_SingleBufferMutex;

        /*=============== マルチバッファ群 ===============*/
        FVector<std::array<atomic_shared_ptr<GpuBuffer>, Setting::kMaxBufferingCount>> m_MultiBuffers;
        std::mutex m_MultiBufferMutex;

        /*=============== テクスチャバッファ群 ===============*/
        FVector<atomic_shared_ptr<TextureBuffer>> m_TextureBuffers;
        std::mutex m_TextureBufferMutex;
    };
};
