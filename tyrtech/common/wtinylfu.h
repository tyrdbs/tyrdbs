#pragma once


#include <common/branch_prediction.h>
#include <common/disallow_copy.h>

#include <unordered_map>
#include <vector>
#include <list>

#include <cstdint>
#include <cassert>


namespace tyrtech {


template<typename Key, typename Item, typename KeyHasher = std::hash<Key>>
class wtinylfu : private disallow_copy
{
public:
    Item get(const Key& key)
    {
        assert(likely(key != Key()));
        auto it = find(key);

        if (it == iterator_t())
        {
            return m_empty_item;
        }

        touch(it);

        assert(likely(m_window.size() <= m_window.max_items));
        assert(likely(m_main_a1.size() <= m_main_a1.max_items));
        assert(likely(m_main_a2.size() <= m_main_a2.max_items));

        return it->item;
    }

    Item set(const Key& key, const Item& item)
    {
        assert(likely(key != Key()));
        assert(likely(find(key) == iterator_t()));

        Item old_item = evict_if_necessary();

        auto it = create(key);
        it->item = item;

        assert(likely(m_window.size() <= m_window.max_items));
        assert(likely(m_main_a1.size() <= m_main_a1.max_items));
        assert(likely(m_main_a2.size() <= m_main_a2.max_items));

        return old_item;
    }

    Item evict_if_necessary()
    {
        if (m_window.is_full() == false)
        {
            return m_empty_item;
        }

        if (m_main_a1.is_full() == false)
        {
            auto it = m_window.eviction_candidate();

            it->location = item::location::MAIN_A1;
            m_main_a1.move(it, &m_window);

            return m_empty_item;
        }

        return evict_worse_candidate();
    }

    Item evict()
    {
        if (m_window.size() != 0 || m_main_a1.size() != 0)
        {
            if (m_window.size() == 0)
            {
                auto it = m_main_a1.eviction_candidate();
                Item item = it->item;

                m_map.erase(it->key);
                m_main_a1.evict(it);

                return item;
            }

            if (m_main_a1.size() == 0)
            {
                auto it = m_window.eviction_candidate();
                Item item = it->item;

                m_map.erase(it->key);
                m_window.evict(it);

                return item;
            }

            return evict_worse_candidate();
        }

        if (m_main_a2.size() == 0)
        {
            return m_empty_item;
        }

        auto it = m_main_a2.eviction_candidate();
        Item item = it->item;

        m_map.erase(it->key);
        m_main_a2.evict(it);

        return item;
    }

    void set_empty_item(const Item& empty_item)
    {
        m_empty_item = empty_item;
    }

public:
    wtinylfu(uint32_t max_items)
    {
        assert(likely(max_items >= 100));

        m_window.initialize(max_items / 100);
        m_main_a1.initialize(2 * (max_items - m_window.max_items) / 10);
        m_main_a2.initialize(max_items - m_window.max_items - m_main_a1.max_items);

        m_map.reserve(max_items);

        for (auto&& counter : m_counters)
        {
            counter.initialize(max_items / m_counters.size());
        }
    }

private:
    struct item
    {
        enum class location
        {
            WINDOW,
            MAIN_A1,
            MAIN_A2
        };

        location location{location::WINDOW};

        Key  key;
        Item item;
    };

private:
    using list_t =
            std::list<item>;

    using iterator_t =
            typename list_t::iterator;

    using map_t =
            std::unordered_map<Key, iterator_t, KeyHasher>;

private:
    struct lru
    {
        list_t list;

        uint32_t items{0};
        uint32_t max_items{0};

        void initialize(uint32_t max_items)
        {
            this->max_items = max_items;
        }

        iterator_t create(const Key& key)
        {
            assert(likely(items != max_items));
            items++;

            item item;
            item.key = key;

            list.push_front(item);

            return list.begin();
        }

        iterator_t eviction_candidate()
        {
            return --list.end();
        }

        void evict(const iterator_t& it)
        {
            assert(likely(items != 0));
            items--;

            list.erase(it);
        }

        void move(const iterator_t& it, lru* source)
        {
            items++;
            source->items--;

            list.splice(list.begin(), source->list, it);
        }

        uint32_t size() const
        {
            return items;
        }

        bool is_full() const
        {
            if (items == max_items)
            {
                return true;
            }

            return false;
        }
    };

    struct frequency_sketch
    {
        void initialize(uint32_t max_items)
        {
            max_samples = max_items * 10;

            table.resize(1U << (32 - __builtin_clz(max_items)));
            table_mask = table.size() - 1;
        }

        uint32_t frequency(uint32_t hash)
        {
            uint32_t start = (hash & 3) << 2;

            uint32_t frequency = std::numeric_limits<uint32_t>::max();

            for (uint32_t i = 0; i < 4; i++)
            {
                uint32_t index = index_of(hash, i);
                uint32_t count = static_cast<uint32_t>(table[index] >>
                                                       (((start + i) << 2) & 0x0fU));

                frequency = std::min(frequency, count);
            }

            return frequency;
        }

        void increment(uint32_t hash)
        {
            uint32_t start = (hash & 3) << 2;

            bool added = true;

            for (uint32_t i = 0; i < 4; i++)
            {
                uint32_t index = index_of(hash, i);
                added |= increment_at(index, start + i);
            }

            if (added == true)
            {
                samples++;

                if (samples == max_samples)
                {
                    reset();
                }
            }
        }

        bool increment_at(uint32_t i, uint32_t j)
        {
            uint32_t offset = j << 2;
            uint64_t mask = (0x0fUL << offset);

            if ((table[i] & mask) != mask)
            {
                table[i] += 1UL << offset;

                return true;
            }

            return false;
        }

        void reset()
        {
            uint32_t count = 0;

            for (auto&& counters : table)
            {
                count += __builtin_popcountll(counters & 0x1111111111111111UL);
                counters = (counters >> 1) & 0x7777777777777777UL;
            }

            samples = (samples >> 1) - (count >> 2);
        }

        uint32_t index_of(uint32_t item, uint32_t i)
        {
            static uint64_t seeds[] = {
                0xc3a5c85c97cb3127UL,
                0xb492b66fbe98f273UL,
                0x9ae16a3b2f90404fUL,
                0xcbf29ce484222325UL
            };

            uint64_t hash = seeds[i] * item;
            hash += hash >> 32;

            return static_cast<uint32_t>(hash) & table_mask;
        }

        uint32_t spread(uint32_t x)
        {
            x = ((x >> 16) ^ x) * 0x45d9f3bU;
            x = ((x >> 16) ^ x) * 0x45d9f3bU;

            return (x >> 16) ^ x;
        }

        using table_t =
                std::vector<uint64_t>;

        table_t table;

        uint64_t table_mask{0};

        uint32_t max_samples{0};
        uint32_t samples{0};
    };

private:
    using counters_t =
            std::array<frequency_sketch, 4096>;

private:
    lru m_window;
    lru m_main_a1;
    lru m_main_a2;

    map_t m_map;

    counters_t m_counters;

    Item m_empty_item;

private:
    iterator_t create(const Key& key)
    {
        auto it = m_window.create(key);

        assert(likely(m_map.find(key) == m_map.end()));
        m_map[key] = it;

        increment(key);

        return it;
    }

    void touch(iterator_t& it)
    {
        increment(it->key);

        switch (it->location)
        {
            case item::location::WINDOW:
            {
                m_window.move(it, &m_window);

                break;
            }
            case item::location::MAIN_A1:
            {
                if (m_main_a2.is_full() == true)
                {
                    it->location = item::location::MAIN_A2;
                    m_main_a2.move(it, &m_main_a1);

                    auto it1 = m_main_a2.eviction_candidate();

                    it1->location = item::location::MAIN_A1;
                    m_main_a1.move(it1, &m_main_a2);
                }
                else
                {
                    it->location = item::location::MAIN_A2;
                    m_main_a2.move(it, &m_main_a1);
                }

                break;
            }
            case item::location::MAIN_A2:
            {
                m_main_a2.move(it, &m_main_a2);

                break;
            }
        }
    }

    Item evict_worse_candidate()
    {
        Item item = m_empty_item;

        auto it1 = m_window.eviction_candidate();
        auto it2 = m_main_a1.eviction_candidate();

        if (frequency(it1->key) > frequency(it2->key))
        {
            item = it2->item;

            m_map.erase(it2->key);
            m_main_a1.evict(it2);

            it1->location = item::location::MAIN_A1;
            m_main_a1.move(it1, &m_window);
        }
        else
        {
            item = it1->item;

            m_map.erase(it1->key);
            m_window.evict(it1);
        }

        return item;
    }

    iterator_t find(const Key& key)
    {
        auto it = m_map.find(key);

        if (it == m_map.end())
        {
            return iterator_t();
        }

        return it->second;
    }

    uint32_t frequency(const Key& key)
    {
        auto hash = KeyHasher{}(key);
        return m_counters[hash & 0xfff].frequency(hash >> 12);
    }

    void increment(const Key& key)
    {
        auto hash = KeyHasher{}(key);
        m_counters[hash & 0xfff].increment(hash >> 12);
    }
};

}
