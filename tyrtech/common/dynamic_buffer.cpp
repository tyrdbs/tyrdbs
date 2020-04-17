#include <common/dynamic_buffer.h>
#include <common/branch_prediction.h>

#include <memory>
#include <cassert>


namespace tyrtech {


uint32_t dynamic_buffer::size() const
{
    return m_size;
}

char* dynamic_buffer::data()
{
    return m_data;
}

const char* dynamic_buffer::data() const
{
    return m_data;
}

void dynamic_buffer::set_size(uint32_t size)
{
    assert(likely(size <= m_size));
    m_size = size;
}

dynamic_buffer::dynamic_buffer(uint32_t size)
  : m_size(size)
{
    m_data = reinterpret_cast<char*>(std::malloc(size));

    if (m_data == nullptr)
    {
        throw std::bad_alloc();
    }
}

dynamic_buffer::~dynamic_buffer()
{
    delete m_data;
    m_data = nullptr;
}

dynamic_buffer::dynamic_buffer(dynamic_buffer&& other) noexcept
{
    *this = std::move(other);
}

dynamic_buffer& dynamic_buffer::operator=(dynamic_buffer&& other) noexcept
{
    m_data = other.m_data;
    other.m_data = nullptr;

    m_size = other.m_size;
    other.m_size = 0;

    return *this;
}

}
