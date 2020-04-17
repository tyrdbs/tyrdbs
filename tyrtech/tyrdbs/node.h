#pragma once


#include <common/disallow_copy.h>
#include <common/disallow_move.h>

#include <string>
#include <array>


namespace tyrtech::tyrdbs {


class node : private disallow_copy, disallow_move
{
public:
    static constexpr uint32_t page_size{8192};
    static constexpr uint32_t node_size{(page_size - 32) * 255 / 256};
    static constexpr uint32_t max_key_size{1024};

public:
    void load(const char* source, uint32_t source_size);

    uint64_t get_next() const;
    void set_next(uint64_t next_node);

    uint16_t key_count() const;

    template<typename Attributes>
    const Attributes* attributes_at(uint16_t ndx) const
    {
        const entry* entry = entry_at(ndx);

        uint16_t offset = entry->key_offset;
        offset -= entry->value_size;
        offset -= sizeof(Attributes);

        return reinterpret_cast<const Attributes*>(m_data.data() + offset);
    }

    std::string_view key_at(uint16_t ndx) const;
    std::string_view value_at(uint16_t ndx) const;
    bool eor_at(uint16_t ndx) const;
    bool deleted_at(uint16_t ndx) const;

    uint16_t lower_bound(const std::string_view& key) const;

private:
    struct entry
    {
        uint16_t value_size;
        uint16_t key_offset;
        uint16_t key_size : 10;
        uint16_t eor      :  1;
        uint16_t deleted  :  1;
        uint16_t reserved :  4;
    } __attribute__ ((packed));

private:
    using data_t =
            std::array<char, node_size>;

private:
    data_t m_data;

    uint16_t m_key_count{static_cast<uint16_t>(-1)};
    uint64_t m_next_node{static_cast<uint64_t>(-1)};

private:
    const entry* entry_at(uint16_t ndx) const;

private:
    friend class node_writer;
} __attribute__ ((packed));

}
