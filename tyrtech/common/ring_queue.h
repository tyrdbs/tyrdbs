#pragma once


#include <common/disallow_copy.h>
#include <common/disallow_move.h>
#include <common/branch_prediction.h>

#include <cstdint>
#include <cassert>
#include <vector>
#include <algorithm>


namespace tyrtech {


template<typename T>
class ring_queue : private disallow_copy, disallow_move
{
public:
    template<typename Item>
    void push(Item&& item)
    {
        m_queue[m_head] = std::move(item);

        if (((m_head + 1) & m_mask) == m_tail)
        {
            reallocate();
        }

        m_head++;
        m_head &= m_mask;
    }

    T pop()
    {
        assert(likely(m_head != m_tail));

        T item = std::move(m_queue[m_tail]);

        m_tail++;
        m_tail &= m_mask;

        return item;
    }

    T& peek()
    {
        assert(likely(m_head != m_tail));
        return m_queue[m_tail];
    }

    bool empty()
    {
        if (m_head == m_tail)
        {
            return true;
        }

        return false;
    }

public:
    ring_queue()
    {
        m_queue.resize(4);
        m_mask = 3;
    }

private:
    using queue_t =
            std::vector<T>;

private:
    queue_t m_queue;

    uint32_t m_head{0};
    uint32_t m_tail{0};

    uint32_t m_mask{0};

private:
    void reallocate()
    {
        uint32_t size = m_queue.size();
        uint32_t old_size = size;

        size *= 2;

        m_queue.resize(size);
        m_mask = size - 1;

        if (m_tail == m_head + 1)
        {
            std::move_backward(m_queue.begin() + m_tail,
                               m_queue.begin() + old_size,
                               m_queue.end());

            m_tail += old_size;
        }
    }
};

}
