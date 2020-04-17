#include <io/file_writer.h>


namespace tyrtech::io {


uint32_t file_writer::write(const char* data, uint32_t size)
{
    m_file->pwrite(m_offset, data, size);
    m_offset += size;

    return size;
}

uint64_t file_writer::offset() const
{
    return m_offset;
}

void file_writer::set_offset_to(uint64_t offset)
{
    m_offset = offset;
}

file_writer::file_writer(file* file, uint64_t offset)
  : m_file(file)
  , m_offset(offset)
{
}

}
