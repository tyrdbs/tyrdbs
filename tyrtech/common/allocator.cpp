#include <common/allocator.h>
#include <common/branch_prediction.h>

#include <cassert>


namespace tyrtech {


uint32_t allocator::allocate(uint32_t size)
{
    auto it = m_free_set.lower_bound(static_cast<uint64_t>(size) << 32);

    if (it == m_free_set.end())
    {
        return static_cast<uint32_t>(-1);
    }

    uint32_t idx = *it & 0xffffffffU;
    uint32_t available_size = *it >> 32;

    m_free_set.erase(it);
    m_idx_set.erase((static_cast<uint64_t>(idx) << 32) | available_size);

    if (available_size > size)
    {
        m_free_set.insert((static_cast<uint64_t>(available_size - size) << 32) | (idx + size));
        m_idx_set.insert((static_cast<uint64_t>(idx + size) << 32) | (available_size - size));
    }

    m_size += size;

    return idx;
}

void allocator::free(uint32_t idx, uint32_t size)
{
    assert(likely(m_size >= size));
    m_size -= size;

    if (m_idx_set.size() == 0)
    {
        m_free_set.insert((static_cast<uint64_t>(size) << 32) | idx);
        m_idx_set.insert((static_cast<uint64_t>(idx) << 32) | size);

        return;
    }

    auto it = m_idx_set.lower_bound(static_cast<uint64_t>(idx) << 32);

    auto prev_it = m_idx_set.end();
    auto next_it = m_idx_set.end();

    if (it != m_idx_set.end())
    {
        next_it = it;

        if (it != m_idx_set.begin())
        {
            prev_it = --it;
        }
    }
    else
    {
        prev_it = --it;
    }

    if (prev_it != m_idx_set.end())
    {
        uint32_t prev_idx = *prev_it >> 32;
        uint32_t prev_size = *prev_it & 0xffffffffU;

        if (prev_idx + prev_size == idx)
        {
            idx = prev_idx;
            size += prev_size;

            m_idx_set.erase(prev_it);
            m_free_set.erase((static_cast<uint64_t>(prev_size) << 32) | prev_idx);
        }
    }

    if (next_it != m_idx_set.end())
    {
        uint32_t next_idx = *next_it >> 32;
        uint32_t next_size = *next_it & 0xffffffffU;

        if (idx + size == next_idx)
        {
            size += next_size;

            m_idx_set.erase(next_it);
            m_free_set.erase((static_cast<uint64_t>(next_size) << 32) | next_idx);
        }
    }

    m_free_set.insert((static_cast<uint64_t>(size) << 32) | idx);
    m_idx_set.insert((static_cast<uint64_t>(idx) << 32) | size);
}

void allocator::extend(uint32_t size)
{
    m_size += size;
    free(m_max_idx, size);

    m_max_idx += size;
}

uint32_t allocator::capacity() const
{
    return m_max_idx;
}

uint32_t allocator::size() const
{
    return m_size;
}

bool allocator::full() const
{
    return m_free_set.size() == 0;
}

}
