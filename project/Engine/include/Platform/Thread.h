#pragma once
// === C++ Standard Library ===
#include <thread>
#include <stop_token>
#include <chrono>
#include <cstdint>
#include <string>
#include <functional>
#include <mutex>
#include <future>
#include <atomic>
#include <condition_variable>
#include <type_traits>
#include <utility>

namespace Theatria::Platform::Threading
{
    //======================= ユーティリティ =======================//
    /// @brief 現在のスレッドを指定ミリ秒だけ休止させる
    /// @param ms 休止ミリ秒数
    inline void SleepMillis(std::chrono::milliseconds ms) noexcept
    {
        std::this_thread::sleep_for(ms);
    }

    /// @brief 現在のスレッドを指定秒だけ休止させる
    /// @param s 休止秒数
    inline void SleepSeconds(std::chrono::seconds s) noexcept
    {
        std::this_thread::sleep_for(s);
    }

    /// @brief 現在スレッドを即座に譲る、同一プロセッサで動作可能な他スレッドがいればそれに切り替えます。いなければ即復帰。
    inline void ThisThreadYield() noexcept
    {
        std::this_thread::yield();
    }

    /// @brief ハードウェアスレッド数を取得
    inline uint32_t GetHardwareThreadCount() noexcept
    {
        unsigned n = std::thread::hardware_concurrency();
        return n ? n : 1;
    }

    /// @brief 現在のスレッドIDを取得
    inline uint32_t GetCurrentThreadIdU32() noexcept
    {
        // 規格は整数IDを提供しないので、ハッシュで代替（長期キーには使わないこと）
        return static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }

    /// @brief 現在のスレッドを指定時刻まで休止させる
    /// @param timePoint 
    inline void SleepUntil(std::chrono::steady_clock::time_point timePoint)
    {
        std::this_thread::sleep_until(timePoint);
    }

    //======================= mutex =======================//
    using Mutex = std::mutex;

    //======================= lock =======================//
    using LockGuard = std::lock_guard<Mutex>;
    using UniqueLock = std::unique_lock<Mutex>;
    using ScopedLock = std::scoped_lock<Mutex>;

    //======================= atomic =======================//
    using AtomicBool = std::atomic_bool;
    template<typename T>
    using Atomic = std::atomic<T>;

    //======================= token =======================//
    using StopToken = std::stop_token;

    //======================= token =======================//
    using ConditionVariable = std::condition_variable;

    //======================= jtreadのラッパー =======================//
    class Thread final
    {
    public:
        Thread() = default;

        // jthreadはムーブ可能・コピー不可
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
        Thread(Thread&&) noexcept = default;
        Thread& operator=(Thread&&) noexcept = default;

        /// @brief コンストラクタ
        // std::stop_token を受け取る関数オブジェクト
        template<class Fn>
            requires (!std::same_as<std::decay_t<Fn>, Thread>&&
        std::invocable<std::decay_t<Fn>&, std::stop_token>)
            explicit Thread(Fn&& fn)
        {
            using F = std::decay_t<Fn>;
            m_thread = std::jthread(
                [fn_ = F(std::forward<Fn>(fn))](std::stop_token st) mutable {
                    std::invoke(fn_, st);
                });
        }

        // 引数なしの関数オブジェクト
        template<class Fn>
            requires (!std::same_as<std::decay_t<Fn>, Thread> &&
        !std::invocable<std::decay_t<Fn>&, std::stop_token>&&
            std::invocable<std::decay_t<Fn>&>)
            explicit Thread(Fn&& fn)
        {
            using F = std::decay_t<Fn>;
            m_thread = std::jthread(
                [fn_ = F(std::forward<Fn>(fn))](std::stop_token) mutable {
                    std::invoke(fn_);
                });
        }
        /// @brief デストラクタ
        ~Thread() = default;

        void SetThread(std::jthread&& thread) noexcept
        {
            m_thread = std::move(thread);
        }

        /// @brief スレッド停止要求
        void RequestStop() noexcept
        {
            if (!m_thread.joinable()) { return; }// 未起動
            m_thread.request_stop();
        }

        bool Joinable() const noexcept
        {
            return m_thread.joinable();
        }

        void Join()
        {
            if (!m_thread.joinable()) { return; } // 未起動
            m_thread.join();
        }
    private:
        std::jthread m_thread;
    };
};

