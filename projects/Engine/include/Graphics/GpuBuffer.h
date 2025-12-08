#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <span>
#include <concepts>
#include <typeindex>
#include <type_traits>
#include <cstddef>
#include "include/Core/LogAssert.h"
#include "include/Graphics/ResourceLeakChecker.h"

// なぜか定義されていないので追加
#ifndef D3D12_GPU_VIRTUAL_ADDRESS_NULL
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#endif

namespace Theatria::Graphics
{
    using namespace Microsoft::WRL;

    /// @brief 型許可
    template <typename T>
    concept GpuBufferType = std::derived_from<T, class GpuBuffer>;

    template <typename T>
    concept TextureBufferType = std::derived_from<T, class TextureBuffer>;

    /// @brief D3D12Resourceラッパークラス。Default前提
    class GpuResource : public std::enable_shared_from_this<GpuResource>
    {
    public:
        /// @brief コンストラクタ
        GpuResource() = default;
        GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            m_pResource(pResource), m_UseState(CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~GpuResource() = default;
        /// @brief 破棄
        virtual void Destroy()
        {
            if (m_pResource)
            {
                m_pResource.Reset();
                m_pResource = nullptr;
            }
            m_UseState = D3D12_RESOURCE_STATE_COMMON;
            ++m_VersionID;
        }

        /// @brief 直接リソースを取得する演算子
        ID3D12Resource* operator->() { return m_pResource.Get(); }
        const ID3D12Resource* operator->() const { return m_pResource.Get(); }
        /// @brief リソースを取得
        ID3D12Resource* GetResource() { return m_pResource.Get(); }
        ID3D12Resource** GetAddressOf() { return m_pResource.GetAddressOf(); }
        uint32_t GetVersionID() const { return m_VersionID; }
        void AttachResource(ID3D12Resource* pResource)
        {
            m_pResource.Attach(pResource);
        }
        /// @brief リソースの使用状態を取得/設定
        D3D12_RESOURCE_STATES GetUseState() const { return m_UseState; }
        void SetUseState(D3D12_RESOURCE_STATES state) { m_UseState = state; }
        /// @brief フェンスとフェンス値を設定
        void SetFence(ID3D12Fence* pFence, uint64_t fenceValue)
        {
            m_pFence = pFence;
            m_FenceValue = fenceValue;
        }
        /// @brief リソースの使用状態を取得
        bool IsUsed() const
        {
            if (m_pFence)
            {
                return m_pFence->GetCompletedValue() < m_FenceValue;
            }
            return false;
        }
    protected:
        void CreateResource(
            ID3D12Device* device,
            D3D12_HEAP_PROPERTIES& heapProperties,
            D3D12_HEAP_FLAGS heapFlags,
            D3D12_RESOURCE_DESC& desc,
            D3D12_RESOURCE_STATES InitialState,
            D3D12_CLEAR_VALUE* pClearValue)
        {
            HRESULT hr = device->CreateCommittedResource(
                &heapProperties,
                heapFlags,
                &desc,
                InitialState,
                pClearValue,
                IID_PPV_ARGS(&m_pResource)
            );
            m_UseState = InitialState;
            m_HeapType = heapProperties.Type;
            Core::LogAssert::Verify(hr, "GpuResource", "CreateCommittedResource failed");
            SetD3D12Name(m_pResource.Get());
        }
    private:
        ComPtr<ID3D12Resource> m_pResource;///< D3D12リソースポインタ
        D3D12_RESOURCE_STATES m_UseState = D3D12_RESOURCE_STATE_COMMON;///< リソースの使用状態
        D3D12_HEAP_TYPE m_HeapType = D3D12_HEAP_TYPE_DEFAULT;///< ヒープタイプ
        uint32_t m_VersionID = 0;///< バージョンID （リソースが再作成されるたびにインクリメントされる）
        ID3D12Fence* m_pFence = nullptr;///< リソースの同期用フェンス
        uint64_t m_FenceValue = 0;///< フェンス値
    };

    class GpuBuffer : public GpuResource
    {
    public:
        // using byte_span = std::span<const std::byte>;

        /// @brief コンストラクタ
        GpuBuffer() = default;
        GpuBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuResource(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~GpuBuffer() = default;
        virtual void Destroy() override
        {
            GpuResource::Destroy();
            m_BufferSize = {};
            m_NumElements = {};
            m_StructureByteStride = {};
        }
        //virtual byte_span GetRawSpan() const noexcept = 0;
        virtual std::type_index GetElementType() const noexcept { return typeid(void); }
        virtual std::type_index GetBufferType() const noexcept { return typeid(GpuBuffer); }
        UINT64 GetBufferSize() const noexcept { return m_BufferSize; }
        UINT GetNumElements() const noexcept { return m_NumElements; }
        UINT GetStructureByteStride() const noexcept { return m_StructureByteStride; }
    protected:
        virtual void CreateBuffer(
            ID3D12Device* device,
            D3D12_HEAP_PROPERTIES& heapProperties,
            D3D12_HEAP_FLAGS heapFlags,
            D3D12_RESOURCE_STATES InitialState,
            D3D12_RESOURCE_FLAGS resourceFlags,
            UINT numElements,
            UINT structureByteStride)
        {
            // バッファのサイズを取得
            m_BufferSize = static_cast<UINT64>(numElements * structureByteStride);
            // 要素数を取得
            m_NumElements = numElements;
            // 要素のサイズを取得
            m_StructureByteStride = structureByteStride;
            // リソースの設定
            D3D12_RESOURCE_DESC resourceDesc{};
            resourceDesc.Width = m_BufferSize;// リソースのサイズ
            // バッファリソースの設定
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.Flags = resourceFlags;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            GpuResource::CreateResource(device, heapProperties, heapFlags, resourceDesc, InitialState, nullptr);
        }
    private:
        UINT64 m_BufferSize = {};          ///< バッファサイズ
        UINT m_NumElements = {};           ///< 要素数
        UINT m_StructureByteStride = {};   ///< 構造体バイトストライド
    };

    /* DefaultはMapできないので不要
    template<typename T>
    class DefaultBuffer : public GpuBuffer
    {
        public:
        DefaultBuffer() = default;
        virtual ~DefaultBuffer() = default;
    private:
    };
    */

    template<typename T>
    class UploadBuffer final : public GpuBuffer
    {
    public:
        /// @brief コンストラクタ
        UploadBuffer() = default;
        UploadBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        ~UploadBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            if (GetResource())
            {
                GetResource()->Unmap(0, nullptr);
            }
            GpuBuffer::Destroy();
            m_MappedData = std::span<T>{};
        }

        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, UINT numElements)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;// UploadHeapを使う
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
            GpuBuffer::CreateBuffer(
                device, heapProperties, heapFlags,
                initialState, resourceFlags,
                numElements, structureByteStride);
            // マッピング
            T* mappedData = nullptr;// 一時マップ用
            GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
            m_MappedData = std::span<T>(mappedData, numElements);
            // 0クリア
            memset(mappedData, 0, sizeof(T) * numElements);
        }
        /// @brief マッピングデータ取得
        std::span<T>       GetMappedData() { return m_MappedData; }
        std::span<const T> GetMappedData() const { return m_MappedData; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
    private:
        std::span<T> m_MappedData;
    };

    template<typename T>
    class ReadBackBuffer final : public GpuBuffer
    {
    public:
        /// @brief コンストラクタ
        ReadBackBuffer() = default;
        ReadBackBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        ~ReadBackBuffer() = default;
        void Destroy() override
        {
            GetResource()->Unmap(0, nullptr);
            GpuBuffer::Destroy();
            m_MappedData = std::span<T>{};
        }

        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, UINT numElements)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_READBACK;// ReadBackHeapを使う
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COPY_DEST;
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
            GpuBuffer::CreateBuffer(
                device, heapProperties, heapFlags,
                initialState, resourceFlags,
                numElements, structureByteStride);
            // マッピング
            T* mappedData = nullptr;// 一時マップ用
            size_t bufferSize = numElements;
            GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
            m_MappedData = std::span<T>(mappedData, bufferSize);
        }

        /// @brief マッピングデータ取得
        std::span<const T> GetMappedData() const { return m_MappedData; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
        std::type_index GetBufferType() const noexcept override { return typeid(ReadBackBuffer<T>); }
    private:
        std::span<T> m_MappedData;
    };

    template<typename T>
    class ConstantBuffer : public GpuBuffer
    {
    public:
        /// @brief コンストラクタ
        ConstantBuffer() = default;
        ConstantBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~ConstantBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            m_UploadBuffer.Destroy();
            GpuBuffer::Destroy();
        }
        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
            GpuBuffer::CreateBuffer(
                device, heapProperties, heapFlags,
                initialState, resourceFlags,
                1, structureByteStride);
        }
        void CreateUploadBuffer(ID3D12Device* device)
        {
            m_UploadBuffer.CreateBuffer(device, 1);
        }

        std::span<T> GetUploadMappedData() { return m_UploadBuffer.GetMappedData(); }
        UploadBuffer<T>& GetUploadBuffer() { return m_UploadBuffer; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
        std::type_index GetBufferType() const noexcept override { return typeid(ConstantBuffer<T>); }
    private:
        UploadBuffer<T> m_UploadBuffer;
    };

    template<typename T>
    class StructuredBuffer : public GpuBuffer
    {
    public:
        /// @brief コンストラクタ
        StructuredBuffer() = default;
        StructuredBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~StructuredBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            m_UploadBuffer.Destroy();
            GpuBuffer::Destroy();
        }
        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, UINT numElements)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
            GpuBuffer::CreateBuffer(
                device, heapProperties, heapFlags,
                initialState, resourceFlags,
                numElements, structureByteStride);
        }
        void CreateUploadBuffer(ID3D12Device* device, UINT numElements)
        {
            m_UploadBuffer.CreateBuffer(device, numElements);
        }

        std::span<T> GetUploadMappedData() { return m_UploadBuffer.GetMappedData(); }
        UploadBuffer<T>& GetUploadBuffer() { return m_UploadBuffer; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
        std::type_index GetBufferType() const noexcept override { return typeid(StructuredBuffer<T>); }
    private:
        UploadBuffer<T> m_UploadBuffer;
    };

    template<typename T>
    class RWStructuredBuffer : public GpuBuffer
    {
        public:
        /// @brief コンストラクタ
        RWStructuredBuffer() = default;
        RWStructuredBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~RWStructuredBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            m_ReadBackBuffer.Destroy();
            GpuBuffer::Destroy();
        }
        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, UINT numElements)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            GpuBuffer::CreateBuffer(
                device, heapProperties, heapFlags,
                initialState, resourceFlags,
                numElements, structureByteStride);
        }
        void CreateReadBackBuffer(ID3D12Device* device, UINT numElements)
        {
            m_ReadBackBuffer.CreateBuffer(device, numElements);
        }

        std::span<const T> GetReadBackMappedData() const { return m_ReadBackBuffer.GetMappedData(); }
        ReadBackBuffer<T>& GetReadBackBuffer() { return m_ReadBackBuffer; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
        std::type_index GetBufferType() const noexcept override { return typeid(RWStructuredBuffer<T>); }
    private:
        ReadBackBuffer<T> m_ReadBackBuffer;
    };

    template<typename T>
    class VertexBuffer final : public GpuBuffer
    {
    public:
        /// @brief コンストラクタ
        VertexBuffer() = default;
        VertexBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~VertexBuffer() = default;
        void Destroy() override
        {
            m_UploadBuffer.Destroy();
            m_ReadBackBuffer.Destroy();
            GpuBuffer::Destroy();
        }
        /// @brief バッファ作成
        /// @param device 
        /// @param numElements 
        /// @param isSkinningVertex 
        void CreateBuffer(ID3D12Device* device, UINT numElements)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            D3D12_RESOURCE_FLAGS resourceFlag = D3D12_RESOURCE_FLAG_NONE;
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = heapType;
            GpuBuffer::CreateBuffer(
                device, heapProperties, D3D12_HEAP_FLAG_NONE,
                resourceState, resourceFlag,
                numElements, structureByteStride);
            // 頂点バッファビューの設定
            m_View.BufferLocation = GetResource()->GetGPUVirtualAddress();
            m_View.SizeInBytes = static_cast<UINT>(GetResource()->GetDesc().Width);
            m_View.StrideInBytes = structureByteStride;
        }
        void CreateUploadBuffer(ID3D12Device* device, UINT numElements)
        {
            m_UploadBuffer.CreateBuffer(device, numElements);
        }
        void CreateReadBackBuffer(ID3D12Device* device, UINT numElements)
        {
            m_ReadBackBuffer.CreateBuffer(device, numElements);
        }
        /// @brief 頂点バッファビュー取得
        D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView() { return &m_View; }
        /// @brief マッピングデータ取得
        std::span<T>       GetUploadMappedData() { return m_UploadBuffer.GetMappedData(); }
        std::span<const T> GetReadBackMappedData() const { return m_ReadBackBuffer.GetMappedData(); }
        UploadBuffer<T>& GetUploadBuffer() { return m_UploadBuffer; }
        ReadBackBuffer<T>& GetReadBackBuffer() { return m_ReadBackBuffer; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
        std::type_index GetBufferType() const noexcept override { return typeid(VertexBuffer<T>); }
    private:
        D3D12_VERTEX_BUFFER_VIEW m_View{};///< 頂点バッファビュー
        UploadBuffer<T> m_UploadBuffer;
        ReadBackBuffer<T> m_ReadBackBuffer;
    };

    template<typename T>
    class IndexBuffer final : public GpuBuffer
    {
    public:
        /// @brief コンストラクタ
        IndexBuffer() = default;
        IndexBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        ~IndexBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            m_UploadBuffer.Destroy();
            m_ReadBackBuffer.Destroy();
            GpuBuffer::Destroy();
        }
        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, UINT numElements)
        {
            // Tが構造体、クラスの時、サイズチェック
            static_assert(!std::is_class_v<T> || sizeof(T) % 16 == 0, "The size of T must be a multiple of 16 bytes.");
            // リソースのサイズ
            UINT structureByteStride = static_cast<UINT>(sizeof(T));
            // リソース用のヒープの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
            D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
            GpuBuffer::CreateBuffer(
                device, heapProperties, heapFlags,
                initialState, resourceFlags,
                numElements, structureByteStride);
            // インデックスバッファビューの設定
            m_View.BufferLocation = GetResource()->GetGPUVirtualAddress();
            m_View.SizeInBytes = static_cast<UINT>(GetResource()->GetDesc().Width);
            // インデックスの形式
            if constexpr (std::is_same_v<T, uint16_t>)
            {
                m_View.Format = DXGI_FORMAT_R16_UINT;
            }
            else if constexpr (std::is_same_v<T, uint32_t>)
            {
                m_View.Format = DXGI_FORMAT_R32_UINT;
            }
            else
            {
                Core::LogAssert::Check(false, "IndexBuffer", "Unsupported index buffer type");
            }
        }
        void CreateUploadBuffer(ID3D12Device* device, UINT numElements)
        {
            m_UploadBuffer.CreateBuffer(device, numElements);
        }
        void CreateReadBackBuffer(ID3D12Device* device, UINT numElements)
        {
            m_ReadBackBuffer.CreateBuffer(device, numElements);
        }
        /// @brief インデックスバッファビュー取得
        D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() { return &m_View; }
        /// @brief マッピングデータ取得
        std::span<T>       GetUploadMappedData() { return m_UploadBuffer.GetMappedData(); }
        std::span<const T> GetReadBackMappedData() const { return m_ReadBackBuffer.GetMappedData(); }
        UploadBuffer<T>& GetUploadBuffer() { return m_UploadBuffer; }
        ReadBackBuffer<T>& GetReadBackBuffer() { return m_ReadBackBuffer; }
        /// @brief 要素の型を取得
        std::type_index GetElementType() const noexcept override { return typeid(T); }
        std::type_index GetBufferType() const noexcept override { return typeid(IndexBuffer<T>); }
    private:
        D3D12_INDEX_BUFFER_VIEW m_View{};///< インデックスバッファビュー
        UploadBuffer<T> m_UploadBuffer;
        ReadBackBuffer<T> m_ReadBackBuffer;
    };

    class TextureBuffer : public GpuResource
    {
    public:
        /// @brief コンストラクタ
        TextureBuffer() = default;
        TextureBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            GpuResource(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~TextureBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            GpuResource::Destroy();
            m_Width = {};
            m_Height = {};
            m_MipLevels = {};
            m_ArraySize = {};
            m_Format = DXGI_FORMAT_UNKNOWN;
            m_Dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
        }
        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, D3D12_RESOURCE_DESC& desc, D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES state)
        {
            // 利用するHeapの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
            // パラメータの設定
            m_Width = desc.Width;
            m_Height = desc.Height;
            m_MipLevels = desc.MipLevels;
            m_ArraySize = desc.DepthOrArraySize;
            m_Format = desc.Format;
            m_Dimension = desc.Dimension;
            GpuResource::CreateResource(device, heapProperties, D3D12_HEAP_FLAG_NONE, desc, state, clearValue);
        }
        UINT64 GetWidth() const { return m_Width; }
        UINT GetHeight() const { return m_Height; }
        UINT16 GetMipLevels() const { return m_MipLevels; }
        UINT16 GetArraySize() const { return m_ArraySize; }
        const DXGI_FORMAT& GetFormat() const { return m_Format; }
        D3D12_RESOURCE_DIMENSION GetDimension() const { return m_Dimension; }
    private:
        UINT64 m_Width{};///< 幅
        UINT m_Height{};///< 高さ
        UINT16 m_MipLevels{};///< ミップレベル数
        UINT16 m_ArraySize{};///< 配列サイズ
        DXGI_FORMAT m_Format{};///< フォーマット
        D3D12_RESOURCE_DIMENSION m_Dimension{};///< リソースの次元
    };

    /* TextureBufferを使いまわせるので不要
    class ColorBuffer : public TextureBuffer
    {
        public:
        ColorBuffer() = default;
        virtual ~ColorBuffer() = default;
    private:
    };
    */

    class DepthBuffer : public TextureBuffer
    {
    public:
        /// @brief コンストラクタ
        DepthBuffer() = default;
        DepthBuffer(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
            TextureBuffer(pResource, CurrentState)
        {
        }
        /// @brief デストラクタ
        virtual ~DepthBuffer() = default;
        /// @brief 破棄
        void Destroy() override
        {
            TextureBuffer::Destroy();
        }
        /// @brief バッファ作成
        void CreateBuffer(ID3D12Device* device, D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES state)
        {
            // 利用するHeapの設定
            D3D12_HEAP_PROPERTIES heapProperties{};
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;// DefaultHeapを使う
            // DepthStencilとして使う通知
            desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            // 深度値のクリア設定
            D3D12_CLEAR_VALUE clearValue{};
            clearValue.DepthStencil.Depth = 1.0f;// 1.0f（最大値）でクリア
            clearValue.Format = desc.Format;// フォーマット。Resourceと合わせる
            TextureBuffer::CreateBuffer(device, desc, &clearValue, state);
        }
    private:
    };
} // namespace Theatria::Graphics
