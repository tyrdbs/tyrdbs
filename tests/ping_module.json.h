#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>


namespace tests::ping {


namespace messages::ping {


struct request_builder final : public tyrtech::message::struct_builder<1, 0>
{
    request_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_sequence(const uint64_t& value)
    {
        set_offset<0>();
        struct_builder<1, 0>::add_value(value);
    }

    static constexpr uint16_t sequence_bytes_required()
    {
        return tyrtech::message::element<uint64_t>::size;
    }
};

struct request_parser final : public tyrtech::message::struct_parser<1, 0>
{
    request_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    request_parser() = default;

    bool has_sequence() const
    {
        return has_offset<0>();
    }

    decltype(auto) sequence() const
    {
        return tyrtech::message::element<uint64_t>().parse(m_parser, offset<0>());
    }
};

struct response_builder final : public tyrtech::message::struct_builder<1, 0>
{
    response_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_sequence(const uint64_t& value)
    {
        set_offset<0>();
        struct_builder<1, 0>::add_value(value);
    }

    static constexpr uint16_t sequence_bytes_required()
    {
        return tyrtech::message::element<uint64_t>::size;
    }
};

struct response_parser final : public tyrtech::message::struct_parser<1, 0>
{
    response_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    response_parser() = default;

    bool has_sequence() const
    {
        return has_offset<0>();
    }

    decltype(auto) sequence() const
    {
        return tyrtech::message::element<uint64_t>().parse(m_parser, offset<0>());
    }
};

}

void throw_module_exception(const tyrtech::net::service::error_parser& error)
{
    switch (error.code())
    {
        case -1:
        {
            throw tyrtech::net::unknown_module_error("{}", error.message());
        }
        case -2:
        {
            throw tyrtech::net::unknown_function_error("{}", error.message());
        }
        default:
        {
            throw tyrtech::net::unknown_exception_error("#{}: unknown exception", error.code());
        }
    }
}

static constexpr uint16_t id{1};

struct ping
{
    static constexpr uint16_t id{1};
    static constexpr uint16_t module_id{1};

    using request_builder_t =
            messages::ping::request_builder;

    using request_parser_t =
            messages::ping::request_parser;

    using response_builder_t =
            messages::ping::response_builder;

    using response_parser_t =
            messages::ping::response_parser;

    static void throw_exception(const tyrtech::net::service::error_parser& error)
    {
        throw_module_exception(error);
    }
};

template<typename Implementation>
struct module : private tyrtech::disallow_copy
{
    Implementation* impl{nullptr};

    module(Implementation* impl)
      : impl(impl)
    {
    }

    void process_message(const tyrtech::net::service::request_parser& service_request,
                         tyrtech::net::service::response_builder* service_response,
                         typename Implementation::context* ctx)
    {
        switch (service_request.function())
        {
            case ping::id:
            {
                using request_parser_t =
                        typename ping::request_parser_t;

                using response_builder_t =
                        typename ping::response_builder_t;

                request_parser_t request(service_request.get_parser(),
                                         service_request.message());
                response_builder_t response(service_response->add_message());

                impl->ping(request, &response, ctx);

                break;
            }
            default:
            {
                throw tyrtech::net::unknown_function_error("#{}: unknown function", service_request.function());
            }
        }
    }

    decltype(auto) create_context(const std::shared_ptr<tyrtech::io::channel>& remote)
    {
        return impl->create_context(remote);
    }
};

}
