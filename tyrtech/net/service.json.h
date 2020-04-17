#pragma once


#include <message/builder.h>
#include <message/parser.h>


namespace tyrtech::net::service {


struct error_builder final : public tyrtech::message::struct_builder<2, 0>
{
    error_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_code(const int16_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t code_bytes_required()
    {
        return tyrtech::message::element<int16_t>::size;
    }

    void add_message(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t message_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct error_parser final : public tyrtech::message::struct_parser<2, 0>
{
    error_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    error_parser() = default;

    bool has_code() const
    {
        return has_offset<0>();
    }

    decltype(auto) code() const
    {
        return tyrtech::message::element<int16_t>().parse(m_parser, offset<0>());
    }

    bool has_message() const
    {
        return has_offset<1>();
    }

    decltype(auto) message() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

struct request_builder final : public tyrtech::message::struct_builder<1, 4>
{
    request_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void set_module(uint16_t value)
    {
        *reinterpret_cast<uint16_t*>(m_static + 0) = value;
    }

    void set_function(uint16_t value)
    {
        *reinterpret_cast<uint16_t*>(m_static + 2) = value;
    }

    decltype(auto) add_message()
    {
        set_offset<0>();
        return m_builder;
    }
};

struct request_parser final : public tyrtech::message::struct_parser<1, 4>
{
    request_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    request_parser() = default;

    decltype(auto) module() const
    {
        return *reinterpret_cast<const uint16_t*>(m_static + 0);
    }

    decltype(auto) function() const
    {
        return *reinterpret_cast<const uint16_t*>(m_static + 2);
    }

    bool has_message() const
    {
        return has_offset<0>();
    }

    decltype(auto) message() const
    {
        return offset<0>();
    }
};

struct response_builder final : public tyrtech::message::struct_builder<2, 0>
{
    response_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    decltype(auto) add_error()
    {
        set_offset<0>();
        return error_builder(m_builder);
    }

    decltype(auto) add_message()
    {
        set_offset<1>();
        return m_builder;
    }
};

struct response_parser final : public tyrtech::message::struct_parser<2, 0>
{
    response_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    response_parser() = default;

    bool has_error() const
    {
        return has_offset<0>();
    }

    decltype(auto) error() const
    {
        return error_parser(m_parser, offset<0>());
    }

    bool has_message() const
    {
        return has_offset<1>();
    }

    decltype(auto) message() const
    {
        return offset<1>();
    }
};

}
