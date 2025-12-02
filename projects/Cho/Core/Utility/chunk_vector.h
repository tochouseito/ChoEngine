#pragma once
/*--------------------------------------------------------------------
  chunk_vector.h — A general‑purpose fixed‑chunk dynamic container
  --------------------------------------------------------------------
  * Stable pointers/iterators: objects are never relocated after
  *   construction (except explicit erase()).
  * Fast O(1) indexed access:   obj = cv[idx];
  * Header‑only, C++17‑compatible, no external deps.
  * Customisable ChunkSize and Allocator.
  *
  *   Typical usage:
  *     cho::chunk_vector<MyType, 512> cv;
  *     auto* p   = cv.emplace_back(args...);      // returns T*
  *     auto  id  = cv.emplace_back_idx(args...);  // returns index
  *     cv.erase_swap_and_pop(id);                 // O(1) delete w/o holes
  *     for (auto& v : cv) { ... }
  *
  * Author: TheatriaEngine project (2025)
--------------------------------------------------------------------*/
#include <vector>
#include <memory>
#include <cstddef>
#include <cassert>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>

namespace theatria
{

    // ---------------------------------------------------------------
    //  chunk_vector  (primary template)　　
    // ---------------------------------------------------------------

    template <typename T,
        std::size_t ChunkSize = 1024,
        typename Allocator = std::allocator<std::byte>>
        class chunk_vector
    {
        static_assert(std::is_destructible_v<T>, "T must be destructible");
        static_assert((ChunkSize& (ChunkSize - 1)) == 0,
            "ChunkSize must be a power of two");

        using alloc_traits = std::allocator_traits<Allocator>;

        struct Chunk
        {
            using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
            storage_t data[ChunkSize];
        };

    public:
        // -------- public type aliases --------
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;
        using const_pointer = const value_type*;

    private:
        // -------- helper constants --------
        static constexpr size_type kMask = ChunkSize - 1;
        static constexpr size_type kShift = [] {
            size_type s = 0; while ((1ULL << s) < ChunkSize) ++s; return s; }();

            // -------- internal state --------
            std::vector<Chunk*>    m_chunks;    // owned memory blocks
            std::vector<size_type> m_free;      // recycled indices (holes)
            std::vector<uint8_t>   m_alive;     // occupancy flags
            allocator_type         m_alloc{};
            size_type              m_next = 0; // next new index (high‑water mark)
            size_type              m_active = 0;// live element count

            // -------- index math --------
            static constexpr size_type chunk_of(size_type idx)  noexcept { return idx >> kShift; }
            static constexpr size_type offset_of(size_type idx) noexcept { return idx & kMask; }

            // allocate new chunk(s) up to cidx inclusive
            void ensure_chunk(size_type cidx)
            {
                while (m_chunks.size() <= cidx)
                {
                    using chunk_alloc = typename alloc_traits::template rebind_alloc<Chunk>;
                    using chunk_traits = std::allocator_traits<chunk_alloc>;
                    chunk_alloc ca(m_alloc);
                    Chunk* raw = chunk_traits::allocate(ca, 1);
                    m_chunks.push_back(raw);
                    m_alive.resize(m_chunks.size() * ChunkSize, 0);
                }
            }

            pointer ptr_from_index(size_type idx) noexcept
            {
                size_type c = chunk_of(idx);
                return std::launder(reinterpret_cast<pointer>(&m_chunks[c]->data[offset_of(idx)]));
            }
            const_pointer ptr_from_index(size_type idx) const noexcept
            {
                size_type c = chunk_of(idx);
                return std::launder(reinterpret_cast<const_pointer>(&m_chunks[c]->data[offset_of(idx)]));
            }

            // shrink high‑water mark while last positions are vacant
            void shrink_tail()
            {
                while (m_next && !m_alive[m_next - 1]) --m_next;
            }

            // allocate a slot (reuse hole or append) and mark it alive
            size_type allocate_slot()
            {
                size_type idx;
                if (!m_free.empty())
                {
                    idx = m_free.back();
                    m_free.pop_back();
                }
                else
                {
                    idx = m_next++;
                    ensure_chunk(chunk_of(idx));
                }
                if (m_alive.size() <= idx) m_alive.resize(idx + 1, 0);
                m_alive[idx] = 1;
                ++m_active;
                return idx;
            }

    public:
        // -------- ctors / dtors --------
        chunk_vector() = default;
        explicit chunk_vector(const allocator_type& a) : m_alloc(a) {}
        chunk_vector(const chunk_vector&) = delete;
        chunk_vector& operator=(const chunk_vector&) = delete;

        chunk_vector(chunk_vector&& other) noexcept
            : m_chunks(std::move(other.m_chunks)),
            m_free(std::move(other.m_free)),
            m_alive(std::move(other.m_alive)),
            m_alloc(std::move(other.m_alloc)),
            m_next(other.m_next),
            m_active(other.m_active)
        {
            other.m_next = other.m_active = 0;
        }
        chunk_vector& operator=(chunk_vector&& rhs) noexcept
        {
            if (this == &rhs) return *this;
            clear();
            m_chunks = std::move(rhs.m_chunks);
            m_free = std::move(rhs.m_free);
            m_alive = std::move(rhs.m_alive);
            m_alloc = std::move(rhs.m_alloc);
            m_next = rhs.m_next;
            m_active = rhs.m_active;
            rhs.m_next = rhs.m_active = 0;
            return *this;
        }
        ~chunk_vector()
        {
            clear();
            using chunk_alloc = typename alloc_traits::template rebind_alloc<Chunk>;
            using chunk_traits = std::allocator_traits<chunk_alloc>;
            chunk_alloc ca(m_alloc);
            for (auto* ch : m_chunks) chunk_traits::deallocate(ca, ch, 1);
        }

        // -------- element modifiers --------
        template<class... Args>
        pointer emplace_back(Args&&... args)
        {
            size_type idx = allocate_slot();
            pointer p = ptr_from_index(idx);
            std::construct_at(p, std::forward<Args>(args)...);
            return p;
        }
        pointer push_back(const T& v) { return emplace_back(v); }
        pointer push_back(T&& v) { return emplace_back(std::move(v)); }

        // return index instead of pointer
        template<class... Args>
        size_type emplace_back_idx(Args&&... args)
        {
            size_type idx = allocate_slot();
            pointer p = ptr_from_index(idx);
            std::construct_at(p, std::forward<Args>(args)...);
            return idx;
        }

        // -------- erase (keep hole) --------
        void erase(size_type idx)
        {
            if (idx >= m_alive.size() || !m_alive[idx]) return; // invalid / already empty
            std::destroy_at(ptr_from_index(idx));
            m_alive[idx] = 0;
            m_free.push_back(idx);
            --m_active;
            if (idx == m_next - 1) shrink_tail();
        }

        // -------- erase_swap_and_pop --------
        size_type erase_swap_and_pop(size_type idx)
        {
            if (idx >= m_alive.size() || !m_alive[idx]) return idx;
            size_type last = m_next;
            do { --last; } while (last && !m_alive[last]);
            if (idx != last)
            {
                pointer dst = ptr_from_index(idx);
                pointer src = ptr_from_index(last);
                std::destroy_at(dst);
                std::construct_at(dst, std::move(*src));
                std::destroy_at(src);
                m_alive[last] = 0;
                m_free.push_back(last);
            }
            else
            {
                std::destroy_at(ptr_from_index(idx));
                m_alive[idx] = 0;
            }
            --m_active;
            if (last == m_next - 1) shrink_tail();
            return last; // index that became vacant
        }

        // -------- clear --------
        void clear()
        {
            for (size_type i = 0; i < m_next; ++i) if (m_alive[i]) std::destroy_at(ptr_from_index(i));
            m_free.clear();
            m_alive.assign(m_alive.size(), 0);
            m_next = m_active = 0;
        }

        // -------- capacity / size --------
        [[nodiscard]] size_type size()     const noexcept { return m_active; }
        [[nodiscard]] size_type capacity() const noexcept { return m_chunks.size() * ChunkSize; }
        [[nodiscard]] bool      empty()   const noexcept { return m_active == 0; }

        // reserve element count (same semantics as std::vector)
        void reserve(size_type desired)
        {
            size_type need = (desired + ChunkSize - 1) >> kShift;
            while (m_chunks.size() < need) ensure_chunk(m_chunks.size());
        }
        // reserve by chunk count
        void reserve_chunks(size_type chunkCount)
        {
            if (chunkCount && chunkCount > m_chunks.size())
                ensure_chunk(chunkCount - 1); // allocate up to chunkCount‑1 inclusively
        }

        // -------- element access --------
        reference       operator[](size_type idx)       noexcept { assert(m_alive[idx]); return *ptr_from_index(idx); }
        const_reference operator[](size_type idx) const noexcept { assert(m_alive[idx]); return *ptr_from_index(idx); }

        pointer       get(size_type idx)       noexcept { return (idx < m_alive.size() && m_alive[idx]) ? ptr_from_index(idx) : nullptr; }
        const_pointer get(size_type idx) const noexcept { return (idx < m_alive.size() && m_alive[idx]) ? ptr_from_index(idx) : nullptr; }

        // -------- iterator --------
        class iterator
        {
            chunk_vector* m_par{}; size_type m_idx{};
            void advance() { while (m_idx < m_par->m_next && !m_par->m_alive[m_idx]) ++m_idx; }
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;
            iterator(chunk_vector* p, size_type i) :m_par(p), m_idx(i) { advance(); }
            reference operator*() const { return (*m_par)[m_idx]; }
            pointer   operator->()const { return &(*m_par)[m_idx]; }
            iterator& operator++() { ++m_idx; advance(); return *this; }
            iterator  operator++(int) { iterator tmp = *this; ++*this; return tmp; }
            friend bool operator==(const iterator& a, const iterator& b) { return a.m_idx == b.m_idx; }
            friend bool operator!=(const iterator& a, const iterator& b) { return !(a == b); }
        };

        class const_iterator
        {
            const chunk_vector* m_par{}; size_type m_idx{};
            void advance() { while (m_idx < m_par->m_next && !m_par->m_alive[m_idx]) ++m_idx; }
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;
            const_iterator(const chunk_vector* p, size_type i) :m_par(p), m_idx(i) { advance(); }
            reference operator*() const { return (*m_par)[m_idx]; }
            pointer   operator->()const { return &(*m_par)[m_idx]; }
            const_iterator& operator++() { ++m_idx; advance(); return *this; }
            const_iterator  operator++(int) { const_iterator tmp = *this; ++*this; return tmp; }
            friend bool operator==(const const_iterator& a, const const_iterator& b) { return a.m_idx == b.m_idx; }
            friend bool operator!=(const const_iterator& a, const const_iterator& b) { return !(a == b); }
        };

        iterator begin() { return iterator(this, 0); }
        iterator end() { return iterator(this, m_next); }
        const_iterator begin() const { return const_iterator(this, 0); }
        const_iterator end()   const { return const_iterator(this, m_next); }
        const_iterator cbegin()const { return begin(); }
        const_iterator cend()  const { return end(); }
    };

} // namespace cho
