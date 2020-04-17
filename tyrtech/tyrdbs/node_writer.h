#pragma once


#include <common/branch_prediction.h>
#include <tyrdbs/node.h>

#include <cassert>


namespace tyrtech::tyrdbs {


class node_writer : private disallow_copy, disallow_move
{
public:
    template<typename Attributes>
    int32_t add(const std::string_view& key,
                const std::string_view& value,
                bool eor,
                bool deleted,
                const Attributes& attributes,
                bool no_split)
    {
        assert(likely(key.size() != 0));
        assert(likely(key.size() < node::max_key_size));

        if (m_node == nullptr)
        {
            internal_reset();
        }

        if (has_enough_space<Attributes>(key, value, no_split) == false)
        {
            return -1;
        }

        node::entry* entry = next_entry();

        auto&& copied_key = copy(key);
        auto&& copied_value = copy(value, sizeof(attributes));

        copy(attributes);

        entry->value_size = copied_value.size();
        entry->key_offset = copied_key.data() - m_data->data();
        entry->key_size = copied_key.size();
        entry->eor = eor && value.size() == copied_value.size();
        entry->deleted = deleted;

        return copied_value.size();
    }

    uint32_t flush(char* sink, uint32_t sink_size);
    std::shared_ptr<node> reset();

private:
    std::shared_ptr<node> m_node;

    node::data_t* m_data{nullptr};

    uint16_t m_entry_offset{0};
    uint16_t m_data_offset{0};

    uint16_t m_key_count{0};

private:
    template<typename Attributes>
    bool has_enough_space(const std::string_view& key,
                          const std::string_view& value,
                          bool no_split)
    {
        uint16_t required_size = 0;

        required_size += sizeof(node::entry);
        required_size += key.size();
        required_size += sizeof(Attributes);

        if (no_split == false)
        {
            required_size += std::max(16UL, value.size() >> 3);
        }
        else
        {
            required_size += value.size();
        }

        if (m_data_offset - m_entry_offset < required_size)
        {
            return false;
        }

        return true;
    }

    template<typename Attributes>
    void copy(const Attributes& attributes)
    {
        allocate(sizeof(Attributes));

        std::memcpy(m_data->data() + m_data_offset, &attributes, sizeof(Attributes));
    }

    void internal_reset();

    node::entry* next_entry();

    void allocate(uint16_t size);

    std::string_view copy(const std::string_view& key);
    std::string_view copy(const std::string_view& value, uint16_t reserved_space);
};

}
