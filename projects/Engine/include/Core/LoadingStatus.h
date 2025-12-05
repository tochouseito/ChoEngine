#pragma once
// === C++ Standard Library ===
#include <string>
#include <atomic>
#include <mutex>

namespace Theatria::Core
{
    /// @brief ローディングステータスクラス
    struct LoadingStatus final
    {
        std::atomic<bool> active{ false };   // ローディング中か
        std::atomic<float> progress{ 0.0f }; // 0.0f ～ 1.0f
        std::atomic<bool> done{ false };     // 完了したか

        std::mutex messageMutex;
        std::string message;                 // 「モデル読込中...」みたいな表示用

        void SetMessage(std::string text)
        {
            std::scoped_lock lock(messageMutex);
            message = std::move(text);
        }
    };
};
