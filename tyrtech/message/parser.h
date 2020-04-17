#pragma once


#include <common/disallow_copy.h>
#include <message/element.h>

#include <memory>
#include <cstdint>


namespace tyrtech::message {


class parser
{
public:
    parser(const char* buffer, uint16_t size)
      : m_buffer(buffer)
      , m_size(size)
    {
    }

    parser(const std::string_view& buffer)
      : m_buffer(buffer.data())
      , m_size(buffer.size())
    {
    }

    parser() = default;

protected:
    const char* m_buffer{nullptr};
    uint16_t m_size{0};

private:
    template<typename> friend struct integral_type;
    template<typename> friend struct container_type;
    template<typename> friend struct element;
    friend class list_parser;
    template<uint16_t, uint16_t> friend class struct_parser;
};

template<uint16_t element_count, uint16_t static_size>
class struct_parser
{
public:
    const parser* get_parser() const
    {
        return m_parser;
    }

protected:
    static constexpr uint16_t offsets_size{element_count * sizeof(uint16_t)};
    static constexpr uint16_t min_size{sizeof(uint16_t) + offsets_size + static_size};

protected:
    struct_parser(const parser* parser, uint16_t offset)
      : m_parser(parser)
      , m_offset(offset)
    {
        if (unlikely(offset + min_size > parser->m_size))
        {
            throw malformed_message_error("read offset past message size");
        }

        offset += sizeof(uint16_t);

        m_offsets = reinterpret_cast<const uint16_t*>(m_parser->m_buffer + offset);
        offset += offsets_size;

        m_static = m_parser->m_buffer + offset;
        offset += static_size;
    }

    struct_parser() = default;

    virtual ~struct_parser() = default;

protected:
    const parser* m_parser{nullptr};
    const uint16_t* m_offsets{nullptr};
    const char* m_static{nullptr};

    uint16_t m_offset{0};

protected:
    template<uint16_t k>
    bool has_offset() const
    {
        static_assert(k < element_count, "invalid offset specified");

        return m_offsets[k] != 0;
    }

    template<uint16_t k>
    uint16_t offset() const
    {
        static_assert(k < element_count, "invalid offset specified");

        uint16_t offset = m_offsets[k];

        if (unlikely(offset == 0))
        {
            throw malformed_message_error("requested field not present");
        }

        if (unlikely(m_offset + offset > m_parser->m_size))
        {
            throw malformed_message_error("read offset past message size");
        }

        return offset + m_offset;
    }
};

class list_parser
{
protected:
    list_parser(const parser* parser, uint16_t offset)
      : m_parser(parser)
      , m_offset(offset)
    {
        if (unlikely(m_offset + sizeof(uint16_t) > parser->m_size))
        {
            throw malformed_message_error("read offset past message size");
        }

        m_elements = *reinterpret_cast<const uint16_t*>(m_parser->m_buffer + m_offset);
        m_offset += sizeof(uint16_t);
    }

    virtual ~list_parser() = default;

protected:
    const parser* m_parser{nullptr};

    uint16_t m_offset{0};
    uint16_t m_elements{0};
    uint16_t m_element_size{0};
};

}
