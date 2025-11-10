#pragma once
#include <d3d12.h>
#include <wrl.h>

#include <cstdint>
#include <array>

namespace Theatria::Graphics
{
    /*前方宣言*/
    class RenderDevice;

    class DescriptorAllocator final
    {
        template<typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;
    public:
        enum class HeapType : uint8_t
        {
            CBV_SRV_UAV,
            SAMPLER,
            RTV,
            DSV,
            kCount
        };

        DescriptorAllocator() = default;
        ~DescriptorAllocator() = default;

        /// @brief 初期化
        [[nodiscard]] bool Initialize(RenderDevice* pRenderDevice);
    private:
        RenderDevice* m_pRenderDevice = nullptr; ///< レンダーデバイス
        // 各種ディスクリプタヒープ初期サイズ
        static const uint32_t kMaxSUVDescriptorHeapSize = 1024;
        static const uint32_t kMaxRTVDescriptorHeapSize = 20;
        static const uint32_t kMaxDSVDescriptorHeapSize = 2;
        std::array<ComPtr<ID3D12DescriptorHeap>, static_cast<size_t>(HeapType::kCount)> m_DescriptorHeaps = { nullptr }; ///< 各種ディスクリプタヒープ
    };
};

