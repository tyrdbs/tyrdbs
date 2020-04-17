#include <common/branch_prediction.h>
#include <tyrdbs/key_buffer.h>

#include <cstring>
#include <cassert>


namespace tyrtech::tyrdbs {


std::string_view key_buffer::data() const
{
    return std::string_view(m_data.data(), m_size);
}

uint32_t key_buffer::size() const
{
    return m_size;
}

void key_buffer::clear()
{
    m_size = 0;
}

void key_buffer::assign(const std::string_view& other)
{
    assert(likely(other.size() <= m_data.size()));

    std::memcpy(m_data.data(), other.data(), other.size());
    m_size = other.size();
}

}
