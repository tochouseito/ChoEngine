#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <array>
#include <optional>

namespace Theatria::Graphics
{
    /*前方宣言*/
    class RenderDevice;

    /// @brief ヒープタイプ
    enum class HeapType : uint8_t
    {
        CBV_SRV_UAV,
        SAMPLER,
        RTV,
        DSV,
        kCount
    };

    struct DescriptorHandleIndex final
    {
        std::array<std::optional<uint32_t>, static_cast<size_t>(HeapType::kCount)> indices = { 0 };
    };

    class DescriptorAllocator final
    {
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    public:
        /// @brief テーブルの種類
        enum class TableKind : uint8_t { Textures, Buffers, RenderTargets, DepthStencils };
        /// @brief テーブルID
        struct TableID final
        {
            TableKind kind;
            uint16_t  generation;  // テーブルの世代（ブロック移動で ++）
            uint32_t  index;       // テーブル内のローカルindex（0..capacity-1）
            static constexpr uint32_t Invalid = 0xFFFFFFFF;
            bool valid() const { return index != Invalid; }
        };



        DescriptorAllocator() = default;
        ~DescriptorAllocator() = default;

        /// @brief 初期化
        [[nodiscard]] bool Initialize(RenderDevice* pRenderDevice, uint32_t texCap, uint32_t bufCap, uint32_t rtCap = 32, uint32_t dsCap = 2);

        /// @brief テーブル別の割り当て/解放
        TableID Allocate(TableKind k);
        void    Free(const TableID& id);

        void CreateSRVTexture2D(TableID& id, ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        void CreateSRVBuffer(TableID& id, ID3D12Resource* res, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
        void CreateUAVBuffer(TableID& id, ID3D12Resource* res, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);

        void CreateRTV(TableID& id, ID3D12Resource* res, const D3D12_RENDER_TARGET_VIEW_DESC& desc);

        /// @brief テーブルベースアドレス取得
        D3D12_GPU_DESCRIPTOR_HANDLE GetTableBaseGPU(TableKind k);
        /// @brief Handle取得
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(TableID& id);
        /// @brief CPUハンドル取得
        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(TableID& id);
    private:
        RenderDevice* m_pRenderDevice = nullptr; ///< レンダーデバイス

        struct Table final
        {
            uint32_t baseIndex = 0;   ///< ヒープ内の先頭スロット
            uint32_t capacity = 0;
            uint16_t generation = 0;
            std::vector<uint32_t> freeList; ///< 空きスロット
            HeapType heapType = HeapType::CBV_SRV_UAV;
        };

        void EnsureCapacity(TableKind k, uint32_t needOneMore); ///< 足りなければ拡張

        void RecreateHeap(TableKind k, uint32_t newCap, uint32_t newBufCap = 0); ///< 再配置

        Table& GetTable(TableKind k);

        std::array<UINT, static_cast<size_t>(HeapType::kCount)> m_DescriptorSizes = { 0 };

        Table m_Textures;
        Table m_Buffers;
        Table m_RenderTargets;
        Table m_DepthStencils;

        // 旧ヒープの寿命管理（GPU完了まで保持）
        struct RetiredHeap { ComPtr<ID3D12DescriptorHeap> heap; uint64_t fence; };
        std::vector<RetiredHeap> m_retired;

        // 各種ディスクリプタヒープ初期サイズ
        static const uint32_t kMaxSUVDescriptorHeapSize = 1024;
        static const uint32_t kMaxRTVDescriptorHeapSize = 32;
        static const uint32_t kMaxDSVDescriptorHeapSize = 2;
        std::array<ComPtr<ID3D12DescriptorHeap>, static_cast<size_t>(HeapType::kCount)> m_DescriptorHeaps = { nullptr }; ///< 各種ディスクリプタヒープ
    };
};

