#pragma once
#include "include/Graphics/GPUCommand.h"
#include "include/Graphics/FrameGraph.h"

namespace Theatria::Graphics
{
    class RenderDevice;
    class ResourceManager;
    class DescriptorAllocator;

    class Renderer final
    {
    public:
        /// @brief コンストラクタ
        Renderer() = default;
        /// @brief デストラクタ
        ~Renderer() = default;
        /// @brief 初期化
        [[nodiscard]]
        bool Initialize(RenderDevice* device, ResourceManager* rm, DescriptorAllocator* da);

        /// @brief 描画開始
        /// @return 
        GraphicsCommandContext* BeginGraphicsPass() noexcept;
        /// @brief 描画終了
        void EndGraphicsPass(GraphicsCommandContext* cmd) noexcept;
        /// @brief 計算パス開始
        ComputeCommandContext* BeginComputePass() noexcept;
        /// @brief 計算パス終了
        void EndComputePass(ComputeCommandContext* cmd) noexcept;
        /// @brief コピーパス開始
        CopyCommandContext* BeginCopyPass() noexcept;
        /// @brief コピーパス終了
        void EndCopyPass(CopyCommandContext* cmd) noexcept;
        /// @brief バリア挿入
        void ApplyBarriers(FrameGraph& fg, CommandContext* cmd, const std::vector<BarrierInfo>& barriers);
        /// @brief Present
        void Present();
    private:

        RenderDevice* m_Device = nullptr;///> レンダーデバイス
        ResourceManager* m_ResourceManager = nullptr;///> リソースマネージャ
        DescriptorAllocator* m_DescriptorAllocator = nullptr;///> ディスクリプタアロケータ
        std::unique_ptr<CommandPool> m_CommandPool = nullptr;///> コマンドプール

        uint32_t m_DepthBufferIndex = UINT32_MAX; ///> 深度バッファのインデックス
#ifndef NDEBUG // デバッグ、開発用
        uint32_t m_DebugDepthBufferIndex = UINT32_MAX; ///> デバッグ用深度バッファのインデックス
#endif
    };
};

