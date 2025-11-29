#pragma once
#include <include/Platform/Timer.h>
#include <thread>

namespace Theatria::Core
{
    /// @brief フレームカウンター
    class FrameCounter final
    {
    public:
        /// @brief コンストラクタ
        FrameCounter() noexcept = default;
        /// @brief デストラクタ
        ~FrameCounter() noexcept = default;

        void BeginFrame() noexcept
        {
            m_Timer.Stop();
            m_DeltaTime = m_Timer.ElapsedSeconds();
            if(m_DeltaTime > 0.0)
            {
                m_FPS = 1.0 / m_DeltaTime;
            }
            else
            {
                m_FPS = 0.0;
            }
            m_Timer.Start();
        }

        /// @brief デルタタイム取得
        double DeltaTime() const noexcept
        {
            return m_DeltaTime;
        }

        /// @brief フレームレート取得
        uint32_t FPS() const noexcept
        {
            return static_cast<uint32_t>(m_FPS);
        }

        /// @brief 最大フレームレート設定
        /// @brief 0以下で無制限
        /// @param max_fps 
        void SetMaxFPS(uint32_t max_fps) noexcept
        {
            m_MaxFPS = max_fps;
        }

        void SleepFrame() noexcept
        {
            if (m_MaxFPS <= 0.0) { return; } // 無制限

            using Clock = Platform::Timer::Clock;
            // フレーム開始時刻
            const auto start = m_Timer.StartTime();

            // 目標フレーム間隔（秒）→ Clock::duration に変換
            const auto target_sec = Platform::Timer::Duration(1.0 / static_cast<double>(m_MaxFPS));
            const auto target = std::chrono::duration_cast<Clock::duration>(target_sec);

            // 次フレームの目標開始時刻
            const auto next = start + target;

            const auto now = Clock::now();
            if (next > now)
            {
                std::this_thread::sleep_until(next);
            }
        }

    private:
        Platform::Timer m_Timer;///< タイマー
        double m_DeltaTime{};///< 1 / フレーム時間(秒)
        double m_FPS{};///< フレームレート
        uint32_t m_MaxFPS{ 60 };//< 最大フレームレート
    };
};
