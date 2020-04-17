#include <common/branch_prediction.h>
#include <common/exception.h>
#include <tyrdbs/node.h>

#include <lz4.h>
#include <cassert>


namespace tyrtech::tyrdbs {


void node::load(const char* source, uint32_t source_size)
{
    assert(likely(m_key_count == static_cast<uint16_t>(-1)));

    int32_t r = LZ4_decompress_safe(source,
                                    m_data.data(),
                                    source_size,
                                    m_data.size());

    if (r != static_cast<int32_t>(m_data.size()))
    {
        throw runtime_error("unable to decompress node");
    }

    m_key_count = *reinterpret_cast<const uint16_t*>(m_data.data());
}

uint64_t node::get_next() const
{
    return m_next_node;
}

void node::set_next(uint64_t next_node)
{
    assert(likely(next_node != 0));
    m_next_node = next_node;
}

uint16_t node::key_count() const
{
    return m_key_count;
}

std::string_view node::key_at(uint16_t ndx) const
{
    const entry* entry = entry_at(ndx);

    return std::string_view(m_data.data() + entry->key_offset, entry->key_size);
}

std::string_view node::value_at(uint16_t ndx) const
{
    const entry* entry = entry_at(ndx);

    uint16_t offset = entry->key_offset;
    offset -= entry->value_size;

    return std::string_view(m_data.data() + offset, entry->value_size);
}

bool node::eor_at(uint16_t ndx) const
{
    return entry_at(ndx)->eor;
}

bool node::deleted_at(uint16_t ndx) const
{
    return entry_at(ndx)->deleted;
}

uint16_t node::lower_bound(const std::string_view& key) const
{
    uint16_t start = 0;
    uint16_t end = key_count();

    while (end - start > 8)
    {
        uint16_t ndx = ((end - start) >> 1) + start;

        int32_t cmp = key.compare(key_at(ndx));

        if (cmp <= 0)
        {
            end = ndx;
        }
        else
        {
            start = ndx;
        }
    }

    for (uint16_t ndx = start; ndx < end; ndx++)
    {
        if (key.compare(key_at(ndx)) <= 0)
        {
            return ndx;
        }
    }

    return end;
}

const node::entry* node::entry_at(uint16_t ndx) const
{
    const char* data = m_data.data();

    data += sizeof(m_key_count);
    data += ndx << 1;
    data += ndx << 2;

    return reinterpret_cast<const entry*>(data);
}

}
