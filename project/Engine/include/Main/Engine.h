#pragma once
// === C++ Standard Library ===
#include <memory> // unique_ptr

namespace Theatria
{
    /// @brief メインエンジンクラス(すべての所有者)
    class Engine final
    {
    public:
        /// @brief コンストラクタ
        Engine();
        /// @brief デストラクタ
        ~Engine() noexcept;
        /// @brief 稼働処理
        void Operation();
    private:
        /// @brief エンジン初期化 戻り値無視禁止
        /// @return 初期化成功ならtrue、失敗ならfalse
        [[nodiscard]]
        bool Initialize();
        /// @brief エンジン終了処理
        void Shutdown();
    private:
        /// @brief 実装隠蔽クラス
        class Impl;
        std::unique_ptr<Impl> m_pImpl; ///< 実装隠蔽ポインタ

        bool m_Run = false; ///< エンジン稼働フラグ
    };
};
