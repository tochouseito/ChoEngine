#pragma once
#include "include/Platform/Thread.h"
#include "include/Platform/Timer.h"

namespace Theatria::Core
{
    /// @brief Update,Render用フレームジョブクラス
    class FrameJob final
    {
        using Thread = Platform::Threading::Thread;
        using Mutex = Platform::Threading::Mutex;
        using CV = Platform::Threading::ConditionVariable;
    public:
        /// @brief コンストラクタ
        FrameJob() = default;
        /// @brief デストラクタ
        ~FrameJob() = default;
        /// @brief ループジョブ開始
        template<class Func>
        void Start(Func&& func)
        {
            m_Thread.SetThread(std::jthread(
                [this, fn = std::forward<Func>(func)](std::stop_token st)
                {
                    uint64_t currentFrame = 0;

                    while (true)
                    {
                        std::unique_lock lock(m_Mutex);
                        m_Cv.wait(lock, [&]
                            {
                                return m_Exit || m_RequestedFrame > currentFrame;
                            });

                        if (m_Exit || st.stop_requested())
                        {
                            break;
                        }

                        // 今回処理すべきフレーム・インデックスを取り出す
                        currentFrame = m_RequestedFrame;
                        const uint32_t index = m_ParamIndex;
                        lock.unlock();

                        m_Timer.Reset();
                        m_Timer.Start();
                        // 実処理（Update / Render / Present）
                        fn(currentFrame, index);
                        m_Timer.Stop();

                        lock.lock();
                        m_FinishedFrame = currentFrame;
                        lock.unlock();
                        m_Cv.notify_all();
                    }
                }));
        }

        /// @brief フレーム frameNo で、インデックス index を使って処理しろ と依頼
        void Kick(uint64_t frameNo, uint32_t index)
        {
            {
                std::lock_guard lock(m_Mutex);
                m_RequestedFrame = frameNo;
                m_ParamIndex = index;
            }
            m_Cv.notify_one();
        }

        /// @brief そのジョブが frameNo まで完了するのを待つ
        void Wait(uint64_t frameNo)
        {
            std::unique_lock lock(m_Mutex);
            m_Cv.wait(lock, [&]
                {
                    return m_FinishedFrame >= frameNo;
                });
        }

        void Stop()
        {
            {
                std::lock_guard lock(m_Mutex);
                m_Exit = true;
            }
            m_Cv.notify_all();
            // std::jthread なので join は自動
        }

        const Platform::Timer& GetTimer() const noexcept
        {
            return m_Timer;
        }
    private:
        Thread m_Thread;
        Mutex m_Mutex;
        CV m_Cv;
        uint64_t m_RequestedFrame = 0;  ///< 「ここまでやってくれ」と依頼されたフレーム番号
        uint64_t m_FinishedFrame = 0;  ///< 実際に終わったフレーム番号
        uint32_t m_ParamIndex = 0;  ///< このジョブ用のバッファインデックス
        bool m_Exit = false;
        Platform::Timer m_Timer;///< タイマー
    };
}

