#include <storage/file_reader.h>


namespace tyrtech::storage {


uint32_t file_reader::pread(uint64_t offset, char* data, uint32_t size) const
{
    assert(likely(offset < m_descriptor.size));

    if (offset + size > m_descriptor.size)
    {
        size = m_descriptor.size - offset;
    }

    uint32_t requested_size = size;

    while (size != 0)
    {
        auto page_data = m_reader->read(m_descriptor.cache_id,
                                        offset >> page_bits,
                                        m_descriptor.extents);

        if (page_data == nullptr)
        {
            break;
        }

        uint32_t page_offset = offset & page_mask;
        uint32_t part_size = std::min(page_size - page_offset, size);

        assert(likely(part_size != 0));

        std::memcpy(data, page_data + page_offset, part_size);

        data += part_size;
        offset += part_size;

        size -= part_size;
    }

    return requested_size - size;
}

void file_reader::unlink()
{
    m_reader->remove(m_descriptor.extents);
}

const file_descriptor& file_reader::descriptor() const
{
    return m_descriptor;
}

uint32_t file_reader::size() const
{
    return m_descriptor.size;
}

const extents_t& file_reader::extents() const
{
    return m_descriptor.extents;
}

file_reader::file_reader(disk_reader* reader, file_descriptor&& descriptor)
  : m_reader(reader)
  , m_descriptor(std::move(descriptor))
{
}

file_reader::file_reader(file_reader&& other)
{
    *this = std::move(other);
}

file_reader& file_reader::operator=(file_reader&& other)
{
    m_reader = other.m_reader;
    m_descriptor = std::move(other.m_descriptor);

    other.m_reader = nullptr;

    return *this;
}

}
