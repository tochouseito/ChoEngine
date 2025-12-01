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

        void Tick() noexcept
        {
            using namespace Platform;

            using Clock = Timer::Clock;
            using TimePoint = Timer::Time_Point;
            using micrs = Timer::micrs;

            // 初回のみ
            if (!m_Initialized)
            {
                m_Timer.Start();
                m_Initialized = true;
                return;
            }

            if (m_MaxFPS > 0)
            {
                // フレームレートピッタリの時間
                const micrs frameUs = micrs(static_cast<int64_t>(1'000'000.0 / static_cast<double>(m_MaxFPS)));
                // スピン待ち時間（マイクロ秒）
                // ここが長いほどCPU負荷が上がるが、精度が上がる
                const micrs spinUs = micrs(2000);
                // フレーム開始時刻
                const auto start = m_Timer.StartTime();
                // 理想的な次フレーム開始時刻
                const TimePoint target = start + frameUs;

                auto now = Clock::now();
                auto elapsed = std::chrono::duration_cast<micrs>(now - start);

                // スピン待ち
                if (elapsed < frameUs)
                {
                    // 大半をsleep_untilで止める
                    TimePoint sleepUntil = target - spinUs;
                    if (sleepUntil > now)
                    {
                        std::this_thread::sleep_until(sleepUntil);
                        now = Clock::now();
                    }
                    // 最後の1msをスピン待ち
                    while (Clock::now() < target)
                    {
                        std::this_thread::yield();
                    }
                }
            }

            m_DeltaTime = m_Timer.ElapsedSeconds();
            m_FPS = (m_DeltaTime > 0.0) ? (1.0 / m_DeltaTime) : 0.0;
            m_TotalFrames++;

            // 計測開始
            m_Timer.Stop();
            m_Timer.Reset();
            m_Timer.Start();
        }

        /// @brief デルタタイム取得
        double DeltaTime() const noexcept
        {
            return m_DeltaTime;
        }

        /// @brief フレームレート取得
        double FPS() const noexcept
        {
            return m_FPS;
        }

        /// @brief 最大フレームレート設定
        /// @brief 0以下で無制限
        /// @param max_fps 
        void SetMaxFPS(uint32_t max_fps) noexcept
        {
            m_MaxFPS = max_fps;
        }

        void SetMaxLead(uint32_t max_lead) noexcept
        {
            m_MaxLead = max_lead;
        }
        uint32_t GetMaxLead() const noexcept
        {
            return m_MaxLead;
        }
    uint64_t m_TotalFrames{};///< 総フレーム数
    uint64_t m_ProduceFrame = 0; // Update/Render をキック
    private:
        Platform::Timer m_Timer;///< タイマー
        bool m_Initialized{ false };//< 初期化フラグ
        double m_DeltaTime{};///< 1 / フレーム時間(秒)
        double m_FPS{};///< フレームレート
        uint32_t m_MaxFPS{ 60 };//< 最大フレームレート
        uint32_t m_MaxLead = 0;  // 2枚→1, 3枚→2
    };
};
