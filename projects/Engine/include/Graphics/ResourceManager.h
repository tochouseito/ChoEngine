#pragma once
#include <memory>
#include "include/Graphics/GpuBuffer.h"
#include "include/Graphics/GlobalBuffer.h"
#include "include/Graphics/ShaderStruct.h"
#include "include/Graphics/RenderDevice.h"
#include "include/Assets/ModelContainer.h"
#include "config/engineConfig.h"
#include "include/Utility/atomic_shared_ptr.h"
#include "include/Utility/FVector.h"
#include <typeindex>

namespace Theatria::Graphics
{
    class RenderDevice;
    class DescriptorAllocator;
    class Renderer;

    template <typename T>
    using FVector = Theatria::Utility::FVector<T>;
    template <typename T>
    using atomic_shared_ptr = Theatria::Utility::atomic_shared_ptr<T>;

    class ResourceManager final
    {
        friend class FrameGraph;
        friend class Renderer;
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;

        /// @brief 初期化
        /// @param device 
        /// @param descAllocator 
        /// @return 
        [[nodiscard]]
        bool Initialize(RenderDevice* device, DescriptorAllocator* descAllocator, Renderer* renderer);

        /*=============== CreateResources ===============*/
        void CreateGlobalBuffers();

        template<typename T>
        [[nodiscard]]
        uint32_t CreateConstantBuffer()
        {
            std::lock_guard<std::mutex> lock(m_MultiBufferMutex);
            std::array<atomic_shared_ptr<GpuBuffer>, Config::Graphics::kMaxBufferingCount> bufferSet{};
            for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
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
            std::array<atomic_shared_ptr<GpuBuffer>, Config::Graphics::kMaxBufferingCount> bufferSet{};
            for (uint32_t i = 0; i < Config::Graphics::BufferingCount; i++)
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
        [[nodiscard]]
        uint32_t CreateVertexBuffer(uint32_t numVertices, const std::vector<Assets::VertexData>& vec);
        [[nodiscard]]
        uint32_t CreateIndexBuffer(uint32_t numIndices, const std::vector<uint32_t>& vec);
        void RemakeIntegratedVBIB(const std::vector<Assets::VertexData>& vertices, const std::vector<uint32_t>& indices);
        /*=============== TextureBuffer ===============*/
        [[nodiscard]]
        uint32_t CreateTextureBuffer(D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue)
        {
            auto buffer = std::make_shared<TextureBuffer>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), desc, clearValue);
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
            clearValue.Color[0] = Config::Graphics::kClearColor[0];
            clearValue.Color[1] = Config::Graphics::kClearColor[1];
            clearValue.Color[2] = Config::Graphics::kClearColor[2];
            clearValue.Color[3] = Config::Graphics::kClearColor[3];
            auto buffer = std::make_shared<TextureBuffer>();
            buffer->CreateBuffer(m_pDevice->GetDevice(), resourceDesc, &clearValue);
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
            buffer->CreateBuffer(m_pDevice->GetDevice(), resourceDesc);
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

        template <typename T>
        GlobalBuffer<T>& GetGlobalObjectBuffer() noexcept
        {
            return m_GlobalObjectBuffer;
        }
        template <typename T>
        GlobalBuffer<T>& GetGlobalTransformBuffer() noexcept
        {
            return m_GlobalTransformBuffer;
        }
        template <typename T>
        GlobalBuffer<T>& GetGlobalMeshInfoBuffer() noexcept
        {
            return m_GlobalMeshInfoBuffer;
        }

        GpuBuffer& GetGlobalUploadBuffer(GlobalBufferType type) noexcept
        {
            switch (type)
            {
            case GlobalBufferType::ObjectBuffer:
                return m_GlobalObjectBuffer.GetUploadBuffer();
            case GlobalBufferType::TransformBuffer:
                return m_GlobalTransformBuffer.GetUploadBuffer();
            case GlobalBufferType::MeshInfoBuffer:
                return m_GlobalMeshInfoBuffer.GetUploadBuffer();
            default:
                Core::LogAssert::Check(false, "ResourceManager", "GetGlobalUploadBuffer: Unsupported GlobalBufferType");
                return m_GlobalObjectBuffer.GetUploadBuffer();
            }
        }

        /*GpuBuffer& GetIndirectCommandCountBuffer() noexcept
        {
            return m_IndirectCommandCountBuffer;
        }
        DescriptorAllocator::TableID GetIndirectCommandCountBufferDescriptorID() const noexcept
        {
            return m_IndirectCommandCountBufferDescriptorIDs;
        }*/

        VertexBuffer<Assets::VertexData>& GetIntegratedVertexBuffer() noexcept
        {
            return m_IntegratedVertexBuffer;
        }

        IndexBuffer<uint32_t>& GetIntegratedIndexBuffer() noexcept
        {
            return m_IntegratedIndexBuffer;
        }

#ifndef NDEBUG
        GpuBuffer& GetDebugCamBuf(uint32_t frameIdx)
        {
            return m_DebugVP[frameIdx];
        }
        GpuBuffer& GetDebugCamUploadBuf()
        {
            return m_DebugVPUploadBuffer;
        }
#endif // !NDEBUG

    private:
        RenderDevice* m_pDevice = nullptr; ///< レンダーデバイス
        DescriptorAllocator* m_pDescAllocator = nullptr; ///< ディスクリプタヒープアロケータ
        Renderer* m_pRenderer = nullptr; ///< レンダラー

        /*=============== シングルバッファ群 ===============*/
        FVector<atomic_shared_ptr<GpuBuffer>> m_SingleBuffers;
        std::mutex m_SingleBufferMutex;

        /*=============== マルチバッファ群 ===============*/
        FVector<std::array<atomic_shared_ptr<GpuBuffer>, Config::Graphics::kMaxBufferingCount>> m_MultiBuffers;
        std::mutex m_MultiBufferMutex;

        /*=============== 頂点、インデックスバッファ ===============*/
        FVector<VertexBuffer<Assets::VertexData>> m_VertexBuffers;
        std::mutex m_VertexBufferMutex;
        FVector<IndexBuffer<uint32_t>> m_IndexBuffers;
        std::mutex m_IndexBufferMutex;
        /*=============== 統合頂点、インデックスバッファ ===============*/
        VertexBuffer<Assets::VertexData> m_IntegratedVertexBuffer;
        IndexBuffer<uint32_t> m_IntegratedIndexBuffer;
        std::mutex m_IntVBIBMutex;

        /*=============== テクスチャバッファ群 ===============*/
        FVector<atomic_shared_ptr<TextureBuffer>> m_TextureBuffers;
        std::mutex m_TextureBufferMutex;

        /*=============== グローバルバッファ ===============*/
        GlobalBuffer<ShaderStruct::SObject> m_GlobalObjectBuffer;
        GlobalBuffer<ShaderStruct::STransform> m_GlobalTransformBuffer;
        GlobalBuffer<ShaderStruct::SMeshInfo> m_GlobalMeshInfoBuffer;

        //RWStructuredBuffer<uint32_t> m_IndirectCommandCountBuffer;
        //DescriptorAllocator::TableID m_IndirectCommandCountBufferDescriptorIDs;
        //std::mutex m_IndirectCommandCountBufferMutex;

#ifndef NDEBUG
        std::array<ConstantBuffer<ShaderStruct::SViewProjection>, Config::Graphics::kMaxBufferingCount> m_DebugVP;
        UploadBuffer<ShaderStruct::SViewProjection> m_DebugVPUploadBuffer;
        std::mutex m_DebugVPMutex;
#endif // !NDEBUG

    };
};
