#include <common/aligned_buffer.h>
#include <common/branch_prediction.h>

#include <memory>
#include <cassert>


namespace tyrtech {


uint32_t aligned_buffer::size() const
{
    return m_size;
}

char* aligned_buffer::data()
{
    return m_data;
}

const char* aligned_buffer::data() const
{
    return m_data;
}

aligned_buffer::aligned_buffer(uint32_t alignment, uint32_t size)
  : m_alignment(alignment)
  , m_size(size)
{
    m_data = reinterpret_cast<char*>(std::aligned_alloc(alignment, size));

    if (m_data == nullptr)
    {
        throw std::bad_alloc();
    }
}

aligned_buffer::~aligned_buffer()
{
    delete m_data;
    m_data = nullptr;
}

aligned_buffer::aligned_buffer(aligned_buffer&& other) noexcept
{
    *this = std::move(other);
}

aligned_buffer& aligned_buffer::operator=(aligned_buffer&& other) noexcept
{
    m_alignment = other.m_alignment;
    other.m_alignment = 0;

    m_data = other.m_data;
    other.m_data = nullptr;

    m_size = other.m_size;
    other.m_size = 0;

    return *this;
}

}
