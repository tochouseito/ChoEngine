#pragma once
#include "include/Graphics/GPUCommand.h"
#include <array>

namespace Theatria::Graphics
{
    class RenderDevice;
    class ResourceManager;

    class Renderer final
    {
    public:
        /// @brief コンストラクタ
        Renderer() = default;
        /// @brief デストラクタ
        ~Renderer() = default;
        /// @brief 初期化
        [[nodiscard]]
        bool Initialize(RenderDevice* renderDevice, ResourceManager* resourceManager);
    private:

        RenderDevice* m_Device = nullptr;///> レンダーデバイス
        ResourceManager* m_ResourceManager = nullptr;///> リソースマネージャ
        std::unique_ptr<CommandPool> m_CommandPool = nullptr;///> コマンドプール

        uint32_t m_DepthBufferIndex = UINT32_MAX; ///> 深度バッファのインデックス
#ifndef NDEBUG // デバッグ、開発用
        uint32_t m_DebugDepthBufferIndex = UINT32_MAX; ///> デバッグ用深度バッファのインデックス
#endif
    };
};

