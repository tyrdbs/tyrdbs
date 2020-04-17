#pragma once


#include <common/disallow_copy.h>
#include <common/branch_prediction.h>

#include <memory>
#include <array>
#include <vector>
#include <algorithm>

#include <cstdint>
#include <cassert>


namespace tyrtech {


template<typename T>
class slabs : private disallow_copy
{
public:
    class slab : private disallow_copy
    {
    public:
        uint16_t allocate()
        {
            assert(likely(m_size != m_capacity));

            uint16_t ndx = m_next;

            m_next = m_alloc_table[ndx];
            m_alloc_table[ndx] = static_cast<uint16_t>(-1);

            m_size++;

            return ndx;
        }

        void free(uint16_t ndx)
        {
            assert(likely(m_size != 0));
            assert(likely(ndx < m_capacity));

            if (m_next == static_cast<uint16_t>(-1))
            {
                m_next = ndx;
            }
            else
            {
                assert(likely(m_alloc_table[ndx] == static_cast<uint16_t>(-1)));

                m_alloc_table[ndx] = m_next;
                m_next = ndx;
            }

            m_size--;
        }

        bool full() const
        {
            return m_size == m_capacity;
        }

        uint16_t capacity() const
        {
            return m_capacity;
        }

        T& get(uint16_t ndx)
        {
            assert(likely(ndx < m_capacity));
            assert(likely(m_alloc_table[ndx] == static_cast<uint16_t>(-1)));

            if (m_object_array == nullptr)
            {
                m_object_array = std::make_unique<T[]>(m_capacity);
            }

            return m_object_array[ndx];
        }

        T& init(uint16_t ndx)
        {
            assert(likely(ndx < m_capacity));

            if (m_object_array == nullptr)
            {
                m_object_array = std::make_unique<T[]>(m_capacity);
            }

            return m_object_array[ndx];
        }

    public:
        slab(uint16_t capacity = 32768)
          : m_capacity(capacity)
        {
            assert(likely(capacity > 0));
            assert(likely(capacity <= 32768));

            m_alloc_table = std::make_unique<uint16_t[]>(m_capacity);

            for (uint16_t i = 0; i < m_capacity - 1; i++)
            {
                m_alloc_table[i] = i + 1;
            }

            m_alloc_table[m_capacity - 1] = static_cast<uint16_t>(-1);
        }

    private:
        using alloc_table_t =
                std::unique_ptr<uint16_t[]>;

        using object_table_t =
                std::unique_ptr<T[]>;

    private:
        alloc_table_t m_alloc_table;
        object_table_t m_object_array;

        uint16_t m_capacity{0};
        uint16_t m_size{0};

        uint16_t m_next{0};
        uint32_t m_slab_ndx{0};

    private:
        friend class slabs;
    };

private:
    using slab_ptr =
            std::unique_ptr<slab>;

    using slabs_t =
            std::vector<slab_ptr>;

    using list_t =
            std::vector<uint16_t>;

public:
    uint32_t allocate()
    {
        assert(likely(m_partial.empty() == false));

        uint16_t slab_ndx = m_partial.front();
        assert(likely(slab_ndx < m_slabs.size()));

        slab* s = m_slabs[slab_ndx].get();
        assert(likely(s->full() == false));

        uint32_t handle = 0;

        handle |= s->m_slab_ndx << 16;
        handle |= s->allocate();

        if (s->full() == true)
        {
            move_to_full(slab_ndx);
        }

        return handle;
    }

    void free(uint32_t handle)
    {
        uint16_t slab_ndx = slab_index(handle);
        uint16_t entry_ndx = handle & 0xffffU;

        slab* s = m_slabs[slab_ndx].get();

        bool was_full = s->full();
        s->free(entry_ndx);

        if (was_full == true)
        {
            move_to_partial(slab_ndx);
        }
    }

    T& get(uint32_t handle)
    {
        slab* s = m_slabs[slab_index(handle)].get();

        return s->get(entry_index(handle));
    }

    T& init(uint32_t handle)
    {
        slab* s = m_slabs[slab_index(handle)].get();

        return s->init(entry_index(handle));
    }

    void extend(std::unique_ptr<slab>&& slab)
    {
        slab->m_slab_ndx = m_slab_ndx++;

        m_partial.push_back(m_slabs.size());
        m_slabs.push_back(std::move(slab));
    }

    bool full() const
    {
        return m_partial.size() == 0;
    }

    uint16_t slab_index(uint32_t handle)
    {
        uint16_t slab_ndx = handle >> 16;
        assert(likely(slab_ndx < m_slabs.size()));

        return slab_ndx;
    }

    uint16_t entry_index(uint32_t handle)
    {
        uint16_t entry_ndx = handle & 0xffff;

        return entry_ndx;
    }

private:
    slabs_t m_slabs;

    list_t m_partial;
    list_t m_full;

    uint16_t m_slab_ndx{0};

private:
    void move_to_full(uint16_t ndx)
    {
        auto it = std::remove(m_partial.begin(), m_partial.end(), ndx);
        m_partial.erase(it, m_partial.end());

        m_full.push_back(ndx);
    }

    void move_to_partial(uint16_t ndx)
    {
        auto it = std::remove(m_full.begin(), m_full.end(), ndx);
        m_full.erase(it, m_full.end());

        m_partial.push_back(ndx);
    }
};

}
