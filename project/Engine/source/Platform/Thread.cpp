#include "pch.h"
#include "include/Platform/Thread.h"

/// @brief すでにロック済みの mtx を受け取り、内部で一時的に解放→待機→復帰時に再ロックした状態で戻します
bool Theatria::Platform::Threading::ConditionVariable::wait(Mutex& mtx, uint32_t timeout_ms) noexcept
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

void Theatria::Platform::Threading::Thread::Join() noexcept
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

void Theatria::Platform::Threading::Thread::Detach() noexcept
{
    // jthread は detach を提供しない（所有＝joinで待つ設計）
    // detach したい場合は std::thread ベースの別クラスを用意すること
    m_jth.reset(); // 破棄（joinして終了を待つ）
    m_joinable = false;
    m_entry = {};
}

bool Theatria::Platform::Threading::Thread::SetName(std::wstring_view n) noexcept
{
    // 規格にスレッド名設定APIなし：名前は保持するだけ
    m_desc.name = n;
    return true;
}

void Theatria::Platform::Threading::Thread::move_from(Thread&& rhs) noexcept
{
    m_jth = std::move(rhs.m_jth);
    m_joinable = rhs.m_joinable; rhs.m_joinable = false;
    m_entry = std::move(rhs.m_entry);
    m_desc = std::move(rhs.m_desc);
}
