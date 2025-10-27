#pragma once
// === C++ Standard Library ===
#include <chrono>

namespace Theatria::Platform
{
    class Timer final
    {
    public:
        using Clock = std::chrono::steady_clock;///< 単調増加クロック
        using Time_Point = Clock::time_point;///< 時間点,記録用
        using Duration = std::chrono::duration<double>;///< 経過時間,秒単位
        using nanos = std::chrono::nanoseconds;// ナノ秒
        using micrs = std::chrono::microseconds;// マイクロ秒
        using millis = std::chrono::milliseconds;// ミリ秒
        using secs = std::chrono::seconds;// 秒

        /// @brief コンストラクタ
        constexpr Timer() noexcept
        {
            Reset();
        }

        /// @brief デストラクタ
        ~Timer() noexcept = default;

        /// @brief リセット
        void Reset() noexcept
        {
            m_Running = false;
            m_Elapsed = Duration::zero();
            m_Start = Time_Point{};
            m_End = Clock::now();
        }

        /// @brief 記録開始
        void Start() noexcept
        {
            if (!m_Running)
            {
                m_Start = Clock::now();
                m_Running = true;
            }
        }

        /// @brief 記録停止
        void Stop() noexcept
        {
            if (m_Running)
            {
                m_Elapsed += Clock::now() - m_Start;
                m_Running = false;
                m_End = Clock::now();
            }
        }

        /// @brief 開始からの経過時間を秒単位で取得
        /// @return 経過時間(秒) 
        double ElapsedSeconds() noexcept
        {
            auto total = m_Elapsed;
            if (m_Running) total += (Clock::now() - m_Start);
            return total.count();
        }

        /// @brief 経過時間をミリ秒単位で返します
        /// @return 経過時間をミリ秒単位で表す符号なし 64 ビット整数 (std::uint64_t)。
        std::uint64_t ElapsedMilliseconds() noexcept
        {
            auto total = m_Elapsed;
            if (m_Running) total += (Clock::now() - m_Start);
            return std::chrono::duration_cast<millis>(total).count();
        }

        /// @brief 動作中かどうか取得
        bool IsRunning() const noexcept { return m_Running; }

    private:
        Time_Point m_Start{};///< 開始時間点
        Time_Point m_End{};///< 終了時間点
        bool m_Running = false;///< 動作中フラグ
        Duration m_Elapsed{};///< 経過時間
    };
}

