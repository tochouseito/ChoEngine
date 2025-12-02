#pragma once
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <stdexcept>
#include <memory>
#include <optional>

// フリーリスト付き可変長配列
template <typename T>
class FVector
{
    template <typename T>
    class FVectorIterator
    {
    public:
        FVectorIterator(FVector<T>* vector, size_t index)
            : vector(vector), index(index)
        {
            advanceToValid(); // 無効な要素をスキップ
        }

        T& operator*() { return (*vector)[index]; }

        FVectorIterator& operator++()
        {
            ++index;
            advanceToValid();
            return *this;
        }

        bool operator!=(const FVectorIterator& other) const
        {
            return index != other.index || vector != other.vector;
        }

    private:
        FVector<T>* vector;
        size_t index;

        void advanceToValid()
        {
            while (index < vector->nextIndex && !vector->isValid(index))
            {
                ++index;
            }
        }
    };
    template <typename T>
    class FVectorConstIterator
    {
    public:
        FVectorConstIterator(const FVector<T>* vector, size_t index)
            : vector(vector), index(index)
        {
            advanceToValid();
        }

        const T& operator*() const { return (*vector)[index]; }

        FVectorConstIterator& operator++()
        {
            ++index;
            advanceToValid();
            return *this;
        }

        bool operator!=(const FVectorConstIterator& other) const
        {
            return index != other.index || vector != other.vector;
        }

    private:
        const FVector<T>* vector;
        size_t index;

        void advanceToValid()
        {
            while (index < vector->nextIndex && !vector->isValid(index))
            {
                ++index;
            }
        }
    };
    using iterator = FVectorIterator<T>;
    friend class FVectorIterator<T>;
    using iterator = FVectorIterator<T>;
    using const_iterator = FVectorConstIterator<T>;
public:
    FVector() = default;

    // 新しい要素を追加し、インデックスを返す
    size_t push_back(T&& value)
    {
        if (!freeStack.empty())
        {
            size_t index = freeStack.back();
            freeStack.pop_back();

            data[index] = std::move(value);
            validFlags[index] = true;
            return index;
        }

        if (nextIndex >= data.size())
        {
            data.push_back(std::move(value));
            validFlags.push_back(true); // 新しい要素は有効
        }
        else
        {
            data[nextIndex] = std::move(value);
            validFlags[nextIndex] = true;
        }
        ++validCount; // 有効な要素数を更新
        return nextIndex++;
    }

    template <typename... Args>
    size_t emplace_back(Args&&... args)
    {
        if (!freeStack.empty())
        {
            size_t index = freeStack.back();
            freeStack.pop_back();

            data[index].emplace(std::forward<Args>(args)...);
            validFlags[index] = true;
            return index;
        }

        if (nextIndex >= data.size())
        {
            data.emplace_back(std::in_place, std::forward<Args>(args)...);
            validFlags.push_back(true);
        }
        else
        {
            data[nextIndex].emplace(std::forward<Args>(args)...);
            validFlags[nextIndex] = true;
        }
        ++validCount; // 有効な要素数を更新
        return nextIndex++;
    }

    // 指定インデックスの要素を削除（フリーリストに追加）
    void erase(size_t index)
    {
        if (index >= nextIndex || !isValid(index)) return;

        data[index].reset();
        validFlags[index] = false;
        freeStack.push_back(index);
        --validCount; // 有効な要素数を更新
    }

    // インデックスアクセス
    T& operator[](size_t index)
    {
        if (index >= nextIndex || !isValid(index))
        {
            throw std::out_of_range("Index out of range or invalid in FVector::operator[]");
        }
        return data[index].value();
    }

    const T& operator[](size_t index) const
    {
        if (index >= nextIndex || !isValid(index))
        {
            throw std::out_of_range("Index out of range or invalid in FVector::operator[]");
        }
        return data[index].value();
    }

    // 要素が有効かどうか
    [[nodiscard]] inline bool isValid(size_t index) const noexcept
    {
        return index < validFlags.size() && validFlags[index] != 0;
    }

    // 予約容量
    size_t capacity() const
    {
        return data.capacity();
    }

    // クリア
    void clear()
    {
        data.clear();
        validFlags.clear();
        freeStack.clear();
        nextIndex = 0;
        validCount = 0;
    }

	// 空かどうか
    bool empty() const
    {
        return validCount == 0;
	}

    // 予約
    void reserve(size_t newCapacity)
    {
        data.reserve(newCapacity);
        validFlags.reserve(newCapacity);
    }

    T& back()
    {
        for (size_t i = nextIndex; i-- > 0; )
        {
            if (isValid(i))
            {
                return data[i].value();
            }
        }
        throw std::out_of_range("FVector::back() called on empty or invalid container");
    }

    const T& back() const
    {
        for (size_t i = nextIndex; i-- > 0; )
        {
            if (isValid(i))
            {
                return data[i].value();
            }
        }
        throw std::out_of_range("FVector::back() called on empty or invalid container");
    }

    // 有効な要素数を返す
    size_t size() const
    {
        return validCount;
    }

    // 非const iterator
    iterator begin()
    {
        return iterator(this, 0);
    }

    iterator end()
    {
        return iterator(this, nextIndex);
    }

    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, nextIndex);
    }


private:
    std::vector<std::optional<T>> data;     // データ本体
    std::vector<size_t> freeStack;          // 再利用候補（順序管理用）
    std::vector<uint8_t> validFlags;           // 有効/無効フラグ
    size_t nextIndex = 0;
    size_t validCount = 0; // 有効な要素の数（size()で使用）
};