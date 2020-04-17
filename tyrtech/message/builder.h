#pragma once


#include <common/disallow_copy.h>
#include <message/element.h>

#include <memory>
#include <cstdint>


namespace tyrtech::message {


class builder : private disallow_copy
{
public:
    uint16_t available_space() const
    {
        return m_max_size - m_offset;
    }

    uint16_t size() const
    {
        return m_offset;
    }

public:
    builder(char* buffer, uint16_t max_size)
      : m_buffer(buffer)
      , m_max_size(max_size)
    {
    }

protected:
    char* m_buffer{nullptr};
    uint16_t m_max_size{0};
    uint16_t m_offset{0};

protected:
    template<typename T>
    void add_value(const T& value)
    {
        element<T>().serialize(this, value);
    }

private:
    template<typename> friend struct integral_type;
    template<typename> friend struct container_type;
    template<typename> friend struct element;
    friend class list_builder;
    template<uint16_t, uint16_t> friend class struct_builder;
};

template<uint16_t element_count, uint16_t static_size>
class struct_builder
{
public:
    void finalize()
    {
        assert(likely(m_builder != nullptr));

        *m_size = m_builder->m_offset - m_offset - sizeof(uint16_t);
        m_builder = nullptr;
    }

public:
    static constexpr uint16_t bytes_required()
    {
        return min_size;
    }

protected:
    static constexpr uint16_t offsets_size{element_count * sizeof(uint16_t)};
    static constexpr uint16_t min_size{sizeof(uint16_t) + offsets_size + static_size};

protected:
    struct_builder(builder* builder)
      : m_builder(builder)
    {
        assert(likely(m_builder->m_offset + min_size <= m_builder->m_max_size));

        m_offset = m_builder->m_offset;

        m_size = reinterpret_cast<uint16_t*>(m_builder->m_buffer + m_builder->m_offset);
        m_builder->m_offset += sizeof(uint16_t);

        m_offsets = reinterpret_cast<uint16_t*>(m_builder->m_buffer + m_builder->m_offset);
        m_builder->m_offset += offsets_size;

        m_static = m_builder->m_buffer + m_builder->m_offset;
        m_builder->m_offset += static_size;

        std::memset(m_size, 0, min_size);
    }

    virtual ~struct_builder()
    {
        if (likely(m_builder != nullptr))
        {
            finalize();
        }
    }

protected:
    builder* m_builder{nullptr};
    uint16_t* m_size{nullptr};
    uint16_t* m_offsets{nullptr};
    char* m_static{nullptr};

    uint16_t m_offset{0};

protected:
    template<uint16_t k>
    void set_offset()
    {
        static_assert(k < element_count, "invalid offset specified");

        assert(likely(m_offsets[k] == 0));
        m_offsets[k] = m_builder->m_offset - m_offset;
    }

    template<typename T>
    void add_value(const T& value)
    {
        assert(likely(m_builder != nullptr));
        m_builder->add_value(value);
    }
};

class list_builder
{
public:
    void finalize()
    {
        assert(likely(m_builder != nullptr));
        m_builder = nullptr;
    }

public:
    static constexpr uint16_t bytes_required()
    {
        return min_size;
    }

protected:
    static constexpr uint16_t min_size{sizeof(uint16_t)};

protected:
    list_builder(builder* builder)
      : m_builder(builder)
    {
        assert(likely(m_builder->m_offset + min_size <= m_builder->m_max_size));

        m_elements = reinterpret_cast<uint16_t*>(m_builder->m_buffer + m_builder->m_offset);
        m_builder->m_offset += sizeof(uint16_t);

        *m_elements = 0;
    }

    virtual ~list_builder()
    {
        if (likely(m_builder != nullptr))
        {
            finalize();
        }
    }

protected:
    builder* m_builder{nullptr};
    uint16_t* m_elements{nullptr};

protected:
    void add_element()
    {
        assert(likely(m_builder != nullptr));
        (*m_elements)++;
    }

    template<typename T>
    void add_value(const T& value)
    {
        m_builder->add_value(value);
    }
};

}
