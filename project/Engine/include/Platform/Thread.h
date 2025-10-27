#pragma once
// === C++ Standard Library ===
#include <thread>
#include <mutex>
#include <condition_variable>
#include <semaphore>        // C++20 counting_semaphore
#include <string>
#include <string_view>
#include <functional>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <stop_token>
#include <optional>
#include <memory>

namespace Theatria::Platform::Threading
{
    //======================= ユーティリティ =======================//
    /// @brief 現在のスレッドを指定ミリ秒だけ休止させる
    /// ms == 0 の場合は即座に復帰
    inline void SleepMillis(uint32_t ms) noexcept
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    /// @brief 現在スレッドを即座に譲る、同一プロセッサで動作可能な他スレッドがいればそれに切り替えます。いなければ即復帰。
    inline void ThisThreadYield() noexcept { std::this_thread::yield(); }

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

    //======================= Mutex / LockGuard =======================//
    // そのまま std::mutex / std::scoped_lock を使用
    using Mutex = std::mutex;
    using LockGuard = std::scoped_lock<Mutex>;

    //======================= ConditionVariable =======================//
    class ConditionVariable final
    {
    public:
        /// @brief 待機しているスレッドをひとつ起床させる
        void notify_one() noexcept { m_cv.notify_one(); }
        /// @brief 待機しているすべてのスレッドを起床させる
        void notify_all() noexcept { m_cv.notify_all(); }

        /// @brief すでにロック済みの mtx を受け取り、内部で一時的に解放→待機→復帰時に再ロックした状態で戻します
        bool wait(Mutex& mtx, uint32_t timeout_ms = (std::numeric_limits<uint32_t>::max)()) noexcept
        {
            if (timeout_ms == (std::numeric_limits<uint32_t>::max)())
            {
                std::unique_lock<Mutex> lk(mtx, std::adopt_lock);
                m_cv.wait(lk);          // 待機中は自動で unlock、起床時に再 lock される
                lk.release();          // ここで「unique_lock による unlock を無効化」して関数を抜ける
                return true;
            }
            else
            {
                // タイムアウト付き待機
                std::unique_lock<Mutex> lk(mtx, std::adopt_lock);
                bool ok = (m_cv.wait_for(lk, std::chrono::milliseconds(timeout_ms)) != std::cv_status::timeout);
                lk.release();
                return ok;
            }
        }
        // 内部用：通常の wait (predicate 付き) が必要な場合は直接 cv_ を使う設計にしてもOK
        std::condition_variable& native() noexcept { return m_cv; }

    private:
        std::condition_variable m_cv;
    };

    /* 実装予定
    //======================= Event（手動/自動リセットを純C++で） =======================//
    // 手動/自動リセットを切り替え可能な汎用イベント。
    // - manualReset=true: set() 後、reset() するまで全待機者が通る
    // - manualReset=false: set() で 1 スレッドだけ通し、直後に自動 reset
    class Event final
    {
    public:
        explicit Event(bool manualReset = true, bool initialState = false)
            : m_manual(manualReset), m_signaled(initialState), m_waiters(0)
        {
        }

        void set() noexcept
        {
            std::unique_lock lk(m_mutex);
            m_signaled = true;
            if (m_manual)
            {
                m_cv.notify_all();
            }
            else
            {
                // 自動リセット：1つだけ起こす
                if (m_waiters > 0) m_cv.notify_one();
            }
        }

        void reset() noexcept
        {
            std::unique_lock lk(m_mutex);
            m_signaled = false;
        }

        bool wait(uint32_t timeout_ms = (std::numeric_limits<uint32_t>::max)()) noexcept
        {
            std::unique_lock lk(m_mutex);
            ++m_waiters;
            auto on_exit = std::unique_ptr<void, void(*)(void*)>(nullptr, [this](void*) {
                std::scoped_lock sl(m_mutex);
                --m_waiters;
                });

            auto pred = [this] { return m_signaled; };
            bool ok = true;
            if (timeout_ms == (std::numeric_limits<uint32_t>::max)())
            {
                m_cv.wait(lk, pred);
            }
            else
            {
                ok = m_cv.wait_for(lk, std::chrono::milliseconds(timeout_ms), pred);
            }

            if (ok && !m_manual)
            {
                // 自動リセット：ここで消灯（残る待機者はブロック継続）
                m_signaled = false;
            }
            return ok;
        }

    private:
        bool m_manual;
        bool m_signaled;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        int m_waiters; // 自動リセット時の notify_one 管理用
    };

    //======================= Semaphore（C++20 counting_semaphore） =======================//
    class Semaphore final
    {
    public:
        explicit Semaphore(ptrdiff_t initialCount = 0)
            : sem_(static_cast<std::ptrdiff_t>(initialCount))
        {
        }

        bool release(ptrdiff_t update = 1) noexcept
        {
            for (ptrdiff_t i = 0; i < update; ++i) sem_.release();
            return true;
        }
        bool wait(uint32_t timeout_ms = (std::numeric_limits<uint32_t>::max)()) noexcept
        {
            if (timeout_ms == (std::numeric_limits<uint32_t>::max)())
            {
                sem_.acquire();
                return true;
            }
            // counting_semaphore にタイムアウト acquire は無いので自前で待つ
            // 簡易的に try_acquire + sleep を組み合わせる（必要なら改良）
            auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
            while (std::chrono::steady_clock::now() < deadline)
            {
                if (sem_.try_acquire()) return true;
                std::this_thread::yield();
            }
            return false;
        }

    private:
        std::counting_semaphore<> sem_{ 0 };
    };
    */

    //======================= 停止トークン（標準C++版） =======================//
    using StopToken = std::stop_token;
    using StopSource = std::stop_source;

    //======================= Thread 本体（std::jthread ラップ） =======================//
    enum class ThreadPriority : int8_t
    {
        Low, BelowNormal, Normal, AboveNormal, High, TimeCritical
    };

    struct ThreadDesc
    {
        std::wstring name;           // 規格に名前APIなし：保持のみ
        size_t       stackSize = 0;  // 規格に指定APIなし：保持のみ
        uint64_t     affinityMask = 0; // 規格に指定APIなし：保持のみ
        ThreadPriority priority = ThreadPriority::Normal; // 規格に指定APIなし：保持のみ
        bool         startSuspended = false; // 規格にサスペンド開始なし：未対応（無視）
        int          idealProcessor = -1;    // 規格に指定APIなし：保持のみ
    };

    class Thread final
    {
    public:
        Thread() = default;
        ~Thread() { Join(); } // jthread はデストラで自動joinするが、明示的に止める

        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;
        Thread(Thread&& rhs) noexcept { move_from(std::move(rhs)); }
        Thread& operator=(Thread&& rhs) noexcept
        {
            if (this != &rhs) { Join(); move_from(std::move(rhs)); }
            return *this;
        }

        // エントリ：StopToken を受け取る関数
        bool Start(std::function<void(StopToken)> entry, const ThreadDesc& desc = {})
        {
            if (m_joinable) return false;
            m_entry = std::move(entry);
            m_desc = desc;

            // 規格では startSuspended を提供しないため、即開始のみ
            m_jth.emplace([this](std::stop_token st) { if (m_entry) m_entry(st); });
            m_joinable = true;
            return true;
        }

        void RequestStop() noexcept { if (m_jth) m_jth->request_stop(); }
        bool StopRequested() const noexcept { return m_jth ? m_jth->get_stop_token().stop_requested() : false; }
        StopToken GetStopToken() const noexcept { return m_jth ? m_jth->get_stop_token() : StopToken{}; }

        void Join() noexcept
        {
            if (m_jth)
            {
                // jthread はデストラで join されるが、明示 join も可能
                // 先に停止要求だけは出しておくと終了が速い
                m_jth->request_stop();
                // join は jthread には無い → reset() で破棄（デストラクタでjoin）
                m_jth.reset();
            }
            m_joinable = false;
            m_entry = {};
        }

        void Detach() noexcept
        {
            // jthread は detach を提供しない（所有＝joinで待つ設計）
            // detach したい場合は std::thread ベースの別クラスを用意すること
            m_jth.reset(); // 破棄（joinして終了を待つ）
            m_joinable = false;
            m_entry = {};
        }

        bool IsRunning() const noexcept { return m_joinable; }
        bool joinable()   const noexcept { return m_joinable; }

        // 規格外の属性は全てノーオペ（将来プラットフォーム別後付け用フック）
        bool SetPriority(ThreadPriority) noexcept { return false; }
        bool SetAffinity(uint64_t) noexcept { return false; }
        bool SetIdealProcessor(int) noexcept { return false; }
        bool SetName(std::wstring_view) noexcept { return false; }

        // 情報（規格は整数スレッドIDを提供しないため、ハッシュで代替）
        uint32_t GetId() const noexcept
        {
            if (!m_jth) return 0;
            return static_cast<uint32_t>(std::hash<std::thread::id>{}(m_jth->get_id()));
        }

    private:
        void move_from(Thread&& rhs) noexcept
        {
            m_jth = std::move(rhs.m_jth);
            m_joinable = rhs.m_joinable; rhs.m_joinable = false;
            m_entry = std::move(rhs.m_entry);
            m_desc = std::move(rhs.m_desc);
        }

    private:
        std::optional<std::jthread> m_jth;
        bool m_joinable = false;
        std::function<void(StopToken)> m_entry;
        ThreadDesc m_desc;
    };
};

