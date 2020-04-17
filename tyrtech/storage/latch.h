#pragma once


#include <gt/condition.h>

#include <unordered_map>


namespace tyrtech::storage {


template<typename Key, typename Value, typename KeyHasher = std::hash<Key>>
class latch : private disallow_copy
{
public:
    Value& acquire(const Key& key)
    {
        auto it = m_entries.find(key);

        if (it == m_entries.end())
        {
            entry e;
            e.value = m_empty_value;

            it = m_entries.insert(typename entries_t::value_type(key, std::move(e))).first;
        }

        it->second.ref_count++;

        return it->second.value;
    }

    void release(const Key& key)
    {
        auto it = m_entries.find(key);
        assert(likely(it != m_entries.end()));

        it->second.released = true;
        it->second.cond.signal_all();
    }

    void wait_for(const Key& key)
    {
        auto it = m_entries.find(key);
        assert(likely(it != m_entries.end()));

        while (it->second.released == false)
        {
            it->second.cond.wait();
        }
    }

    bool remove(const Key& key)
    {
        auto it = m_entries.find(key);
        assert(likely(it != m_entries.end()));

        assert(likely(it->second.ref_count != 0));
        it->second.ref_count--;

        if (it->second.ref_count == 0)
        {
            m_entries.erase(it);
            return true;
        }

        return false;
    }

    void set(const Key& key, const Value& value)
    {
        auto it = m_entries.find(key);
        assert(likely(it != m_entries.end()));

        it->second.value = value;
    }

    void set_empty_value(const Value& value)
    {
        m_empty_value = value;
    }

private:
    struct entry
    {
        uint32_t ref_count{0};

        bool released{false};
        gt::condition cond;

        Value value;
    };

private:
    using entries_t =
            std::unordered_map<Key, entry, KeyHasher>;

private:
    entries_t m_entries;
    Value m_empty_value;
};

}
