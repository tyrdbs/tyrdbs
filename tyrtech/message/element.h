#pragma once


#include <common/exception.h>

#include <memory>
#include <cstdint>


namespace tyrtech::message {


DEFINE_EXCEPTION(runtime_error, error);
DEFINE_EXCEPTION(error, malformed_message_error);
DEFINE_EXCEPTION(error, insufficient_space_error);


template<typename T>
struct integral_type
{
    using value_type = T;

    static constexpr uint16_t size{sizeof(value_type)};

    template<typename Builder>
    void serialize(Builder* builder, const value_type& value)
    {
        if (unlikely(builder->m_offset + size > builder->m_max_size))
        {
            throw insufficient_space_error("target buffer too small");
        }

        std::memcpy(builder->m_buffer + builder->m_offset, &value, size);
        builder->m_offset += size;
    }

    template<typename Parser>
    value_type parse(const Parser* parser, value_type offset)
    {
        if (unlikely(offset + size > parser->m_size))
        {
            throw malformed_message_error("invalid offset");
        }

        return *reinterpret_cast<const value_type*>(parser->m_buffer + offset);
    }
};

template<typename T>
struct container_type
{
    using value_type = T;

    static constexpr uint16_t size{sizeof(uint16_t)};

    template<typename Builder>
    void serialize(Builder* builder, const value_type& value)
    {
        assert(value.size() < 65536);
        uint16_t size = value.size();

        if (unlikely(builder->m_offset + sizeof(uint16_t) + size > builder->m_max_size))
        {
            throw insufficient_space_error("target buffer too small");
        }

        std::memcpy(builder->m_buffer + builder->m_offset, &size, sizeof(size));
        builder->m_offset += sizeof(size);

        std::memcpy(builder->m_buffer + builder->m_offset, value.data(), size);
        builder->m_offset += size;
    }

    template<typename Parser>
    value_type parse(const Parser* parser, uint16_t offset)
    {
        if (unlikely(offset + sizeof(uint16_t) > parser->m_size))
        {
            throw malformed_message_error("invalid offset");
        }

        uint16_t size = *reinterpret_cast<const uint16_t*>(parser->m_buffer + offset);
        offset += sizeof(size);

        if (unlikely(offset + size > parser->m_size))
        {
            throw malformed_message_error("invalid offset");
        }

        return value_type(parser->m_buffer + offset, size);
    }
};

template<typename T>
struct element : public integral_type<T>
{
};

template<>
struct element<char> : public integral_type<char>
{
};

template<>
struct element<int8_t> : public integral_type<int8_t>
{
};

template<>
struct element<uint8_t> : public integral_type<uint8_t>
{
};

template<>
struct element<int16_t> : public integral_type<int16_t>
{
};

template<>
struct element<uint16_t> : public integral_type<uint16_t>
{
};

template<>
struct element<int32_t> : public integral_type<int32_t>
{
};

template<>
struct element<uint32_t> : public integral_type<uint32_t>
{
};

template<>
struct element<int64_t> : public integral_type<int64_t>
{
};

template<>
struct element<uint64_t> : public integral_type<uint64_t>
{
};

template<>
struct element<std::string_view> : public container_type<std::string_view>
{
};

}
