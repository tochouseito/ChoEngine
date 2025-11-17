#pragma once
#include "include/Graphics/GPUCommand.h"

namespace Theatria::Graphics
{
    class RenderDevice;

    class Renderer final
    {
    public:
        /// @brief コンストラクタ
        Renderer() = default;
        /// @brief デストラクタ
        ~Renderer() = default;
    /// @brief 初期化
    [[nodiscard]]
    bool Initialize(RenderDevice* renderDevice);
    private:
        RenderDevice* m_Device = nullptr;
        std::unique_ptr<CommandPool> m_CommandPool = nullptr;
    };
};

