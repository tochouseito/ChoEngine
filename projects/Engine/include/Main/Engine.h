#pragma once
// === C++ Standard Library ===
#include <memory> // unique_ptr
// === Theatria Engine Include ===
#include "include/Utility/APIExportsMacro.h"

namespace Theatria
{
    enum class RuntimeMode : uint8_t
    {
        RuntimeMode_Editor,		// エディタモード
        RuntimeMode_Release,		// リリースモード
    };

    /// @brief メインエンジンクラス(すべての所有者)
    class Engine final
    {
    public:
        /// @brief コンストラクタ
        Engine(RuntimeMode mode);
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
        void Update([[maybe_unused]] uint32_t frameIdx);
        void Render([[maybe_unused]] uint32_t frameIdx);

        /// @brief 実装隠蔽クラス
        class Impl;
        std::unique_ptr<Impl> m_pImpl; ///< 実装隠蔽ポインタ

        bool m_Run = false; ///< エンジン稼働フラグ
        RuntimeMode m_RuntimeMode = RuntimeMode::RuntimeMode_Release; ///< ランタイムモード
    };
};
