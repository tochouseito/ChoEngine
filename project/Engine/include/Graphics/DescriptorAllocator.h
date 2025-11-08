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
        std::array<ComPtr<ID3D12DescriptorHeap>, static_cast<size_t>(HeapType::kCount)> m_DescriptorHeaps = { nullptr }; ///< 各種ディスクリプタヒープ
    };
};

