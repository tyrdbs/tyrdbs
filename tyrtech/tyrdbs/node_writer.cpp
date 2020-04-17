#include <common/exception.h>
#include <tyrdbs/node_writer.h>

#include <memory>
#include <cstring>
#include <lz4.h>


namespace tyrtech::tyrdbs {


uint32_t node_writer::flush(char* sink, uint32_t sink_size)
{
    if (unlikely(m_node == nullptr))
    {
        internal_reset();
    }

    assert(likely(sink_size >= LZ4_COMPRESSBOUND(node::node_size)));

    *reinterpret_cast<uint16_t*>(m_data->data()) = m_key_count;

    int32_t r = LZ4_compress_default(m_data->data(),
                                     sink,
                                     m_data->size(),
                                     sink_size);

    if (r == 0)
    {
        throw runtime_error("unable to compress node");
    }

    return r;
}

std::shared_ptr<node> node_writer::reset()
{
    assert(likely(m_node != nullptr));
    m_node->m_key_count = m_key_count;

    return std::move(m_node);
}

void node_writer::internal_reset()
{
    m_node = std::make_shared<node>();

    m_data = &m_node->m_data;
    m_data->fill(0);

    m_entry_offset = sizeof(node::m_key_count);
    m_data_offset = m_data->size();

    m_key_count = 0;
}

node::entry* node_writer::next_entry()
{
    node::entry* entry = reinterpret_cast<node::entry*>(m_data->data() + m_entry_offset);
    m_entry_offset += sizeof(node::entry);

    m_key_count++;

    return entry;
}

void node_writer::allocate(uint16_t size)
{
    assert(likely(m_entry_offset + size <= m_data_offset));
    m_data_offset -= size;
}

std::string_view node_writer::copy(const std::string_view& key)
{
    allocate(key.size());

    std::memcpy(m_data->data() + m_data_offset, key.data(), key.size());

    return std::string_view(m_data->data() + m_data_offset, key.size());
}

std::string_view node_writer::copy(const std::string_view& value, uint16_t reserved_space)
{
    uint32_t available_space;

    available_space = m_data_offset;
    available_space -= m_entry_offset + reserved_space;

    uint16_t partial_size = std::min(static_cast<size_t>(available_space), value.size());

    assert(likely(partial_size < 16384));
    allocate(partial_size);

    std::memcpy(m_data->data() + m_data_offset, value.data(), partial_size);

    return std::string_view(m_data->data() + m_data_offset, partial_size);
}

}
