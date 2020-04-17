#pragma once


#include <common/slabs.h>


namespace tyrtech {


template<typename T, uint16_t extend_size>
class slab_list : private disallow_copy
{
public:
    static constexpr uint32_t invalid_handle{static_cast<uint32_t>(-1)};

public:
    struct entry
    {
        uint32_t prev;
        uint32_t next;
        T item;
    };

public:
    using entry_pool_t =
            slabs<entry>;

public:
    uint32_t push_front()
    {
        return insert_before(m_front);
    }

    uint32_t push_back()
    {
        return insert_after(m_back);
    }

    uint32_t push_front(const T& item)
    {
        uint32_t e = push_front();
        get_entry(e).item = item;

        return e;
    }

    uint32_t push_back(const T& item)
    {
        uint32_t e = push_back();
        get_entry(e).item = item;

        return e;
    }

    void pop_front()
    {
        erase(m_front);
    }

    void pop_back()
    {
        erase(m_back);
    }

    uint32_t insert_before(uint32_t p)
    {
        uint32_t e = allocate_entry();
        auto& _e = get_entry(e);

        if (m_front == invalid_handle)
        {
            assert(likely(m_back == invalid_handle));
            assert(likely(p == invalid_handle));

            _e.prev = invalid_handle;
            _e.next = invalid_handle;

            m_front = e;
            m_back = e;
        }
        else
        {
            assert(likely(m_back != invalid_handle));
            assert(likely(p != invalid_handle));

            auto& _p = get_entry(p);

            _e.prev = _p.prev;
            _p.prev = e;

            _e.next = p;

            if (p == m_front)
            {
                m_front = e;
            }
        }

        m_size++;

        return e;
    }

    uint32_t insert_after(uint32_t p)
    {
        uint32_t e = allocate_entry();
        m_size++;

        auto& _e = get_entry(e);

        if (m_front == invalid_handle)
        {
            assert(likely(m_back == invalid_handle));
            assert(likely(p == invalid_handle));

            _e.prev = invalid_handle;
            _e.next = invalid_handle;

            m_front = e;
            m_back = e;
        }
        else
        {
            assert(likely(m_back != invalid_handle));
            assert(likely(p != invalid_handle));

            auto& _p = get_entry(p);

            _e.next = _p.next;
            _p.next = e;

            _e.prev = p;

            if (p == m_back)
            {
                m_back = e;
            }
        }

        return e;
    }

    void erase(uint32_t e)
    {
        auto& _e = get_entry(e);

        if (_e.prev != invalid_handle)
        {
            get_entry(_e.prev).next = _e.next;
        }

        if (_e.next != invalid_handle)
        {
            get_entry(_e.next).prev = _e.prev;
        }

        if (m_front == e)
        {
            m_front = _e.next;
        }

        if (m_back == e)
        {
            m_back = _e.prev;
        }

        free_entry(e);
        m_size--;
    }

    uint32_t begin()
    {
        return m_front;
    }

    uint32_t next(uint32_t e)
    {
        return get_entry(e).next;
    }

    uint32_t front()
    {
        return m_front;
    }

    uint32_t back()
    {
        return m_back;
    }

    T* front_item()
    {
        return &get_entry(m_front).item;
    }

    T* back_item()
    {
        return &get_entry(m_back).item;
    }

    T* item(uint32_t e)
    {
        return &get_entry(e).item;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    uint32_t size() const
    {
        return m_size;
    }

public:
    slab_list(entry_pool_t* entry_pool)
      : m_entry_pool(entry_pool)
    {
    }

public:
    slab_list(slab_list&& other) noexcept
    {
        *this = std::move(other);
    }

    slab_list& operator=(slab_list&& other) noexcept
    {
        m_entry_pool = other.m_entry_pool;
        m_front = other.m_front;
        m_back = other.m_back;
        m_size = other.m_size;

        other.m_entry_pool = nullptr;
        other.m_front = invalid_handle;
        other.m_back = invalid_handle;
        other.m_size = 0;

        return *this;
    }

private:
    entry_pool_t* m_entry_pool{nullptr};

    uint32_t m_front{invalid_handle};
    uint32_t m_back{invalid_handle};

    uint32_t m_size{0};

private:
    uint32_t allocate_entry()
    {
        if (unlikely(m_entry_pool->full() == true))
        {
            m_entry_pool->extend(std::make_unique<typename entry_pool_t::slab>(extend_size));
        }

        return m_entry_pool->allocate();
    }

    void free_entry(uint32_t handle)
    {
        assert(likely(handle != invalid_handle));
        m_entry_pool->free(handle);
    }

    entry& get_entry(uint32_t handle)
    {
        assert(likely(handle != invalid_handle));
        return m_entry_pool->get(handle);
    }
};

}
