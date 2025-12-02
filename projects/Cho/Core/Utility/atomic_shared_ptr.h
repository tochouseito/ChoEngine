#pragma once
#include <memory>
#include <atomic>
#include <type_traits>

namespace theatria
{
    template <typename T>
    class atomic_shared_ptr
    {
    public:
        atomic_shared_ptr() = default;

        // shared_ptr<T> からの構築
        atomic_shared_ptr(const std::shared_ptr<T>& p) { ptr.store(p); }

        // shared_ptr<U> -> shared_ptr<T> の変換付き構築
        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        atomic_shared_ptr(const std::shared_ptr<U>& p)
        {
            ptr.store(std::static_pointer_cast<T>(p));
        }

        // コピー禁止
        atomic_shared_ptr(const atomic_shared_ptr&) = delete;
        atomic_shared_ptr& operator=(const atomic_shared_ptr&) = delete;

        // ムーブ許可
        atomic_shared_ptr(atomic_shared_ptr&& other) noexcept
        {
            ptr.store(other.ptr.exchange(nullptr));
        }

        atomic_shared_ptr& operator=(atomic_shared_ptr&& other) noexcept
        {
            if (this != &other)
            {
                ptr.store(other.ptr.exchange(nullptr));
            }
            return *this;
        }

        // アトミックに取得
        std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const
        {
            return ptr.load(order);
        }

        // アトミックに設定
        void store(const std::shared_ptr<T>& p, std::memory_order order = std::memory_order_seq_cst)
        {
            ptr.store(p, order);
        }

        // アトミックに交換
        std::shared_ptr<T> exchange(const std::shared_ptr<T>& p, std::memory_order order = std::memory_order_seq_cst)
        {
            return ptr.exchange(p, order);
        }

        // アトミックに比較して交換（成功したかどうかを返す）
        bool compare_exchange_strong(std::shared_ptr<T>& expected, const std::shared_ptr<T>& desired,
            std::memory_order order = std::memory_order_seq_cst)
        {
            return ptr.compare_exchange_strong(expected, desired, order);
        }

        // スマートポインタ風アクセス（非const）
        T* operator->()
        {
            return load().get();
        }

        // スマートポインタ風アクセス（const）
        const T* operator->() const
        {
            return load().get();
        }

        // Dereference
        T& operator*()
        {
            return *load();
        }

        const T& operator*() const
        {
            return *load();
        }

    private:
        std::atomic<std::shared_ptr<T>> ptr;
    };
}