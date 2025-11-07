#pragma once

namespace Theatria::Graphics
{
    /// @brief リソースリークチェッカー
    class ResourceLeakChecker
    {
    public:
        /// @brief コンストラクタ
        ResourceLeakChecker() = default;
        /// @brief デストラクタ
        ~ResourceLeakChecker();
    };
}

