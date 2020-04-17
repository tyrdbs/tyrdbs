#include <storage/file_writer.h>


namespace tyrtech::storage {


void file_writer::write(const char* data, uint32_t size)
{
    if (m_state->descriptor.size + size >= max_file_size)
    {
        throw error("file size limit reached");
    }

    assert(likely(m_commited == false));

    while (size != 0)
    {
        if (m_state->buffer == nullptr)
        {
            m_writer->allocate_page(m_state);
        }

        uint32_t part_size = std::min(page_size - m_state->buffer_offset, size);

        if (part_size != 0)
        {
            std::memcpy(m_state->buffer + m_state->buffer_offset, data, part_size);

            data += part_size;
            size -= part_size;

            m_state->buffer_offset += part_size;
            m_state->descriptor.size += part_size;
        }

        if (m_state->buffer_offset == page_size)
        {
            m_writer->write_page(m_state);
        }
    }
}

void file_writer::add_padding()
{
    assert(likely(m_commited == false));

    if ((m_state->buffer_offset & page_mask) == 0)
    {
        return;
    }

    uint32_t size = page_size - (m_state->buffer_offset & page_mask);

    std::memset(m_state->buffer + m_state->buffer_offset, 0, size);

    m_state->buffer_offset += size;
    m_state->descriptor.size += size;
}

void file_writer::flush()
{
    assert(likely(m_flushed == false));
    m_flushed = true;

    m_writer->write_page(m_state);
    m_writer->flush_pages(m_state);
}

file_descriptor file_writer::commit()
{
    assert(likely(m_flushed == true));
    assert(likely(m_commited == false));

    m_commited = true;

    return std::move(m_state->descriptor);
}

uint32_t file_writer::size() const
{
    return m_state->descriptor.size;
}

file_writer::file_writer(disk_writer* writer)
  : m_writer(writer)
  , m_state(m_writer->allocate())
{
}

file_writer::~file_writer()
{
    if (unlikely(m_commited == false))
    {
        m_writer->remove(m_state);
    }

    m_writer->free(m_state);
}

file_writer::file_writer(file_writer&& other)
{
    *this = std::move(other);
}

file_writer& file_writer::operator=(file_writer&& other)
{
    m_writer = other.m_writer;
    m_state = other.m_state;
    m_commited = other.m_commited;
    m_flushed = other.m_flushed;

    other.m_writer = nullptr;
    other.m_state = nullptr;
    other.m_commited = false;
    other.m_flushed = false;

    return *this;
}

}
