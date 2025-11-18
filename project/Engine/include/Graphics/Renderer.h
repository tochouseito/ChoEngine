#pragma once
#include "include/Graphics/GPUCommand.h"

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
        void CreateDepthBuffer();

        // Debug用リソース作成
        void CreateDepthBufferDebug();

        RenderDevice* m_Device = nullptr;///> レンダーデバイス
        ResourceManager* m_ResourceManager = nullptr;///> リソースマネージャ
        std::unique_ptr<CommandPool> m_CommandPool = nullptr;///> コマンドプール
        
    };
};

