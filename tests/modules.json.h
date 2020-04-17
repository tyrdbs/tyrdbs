#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>


namespace tests::module1 {


namespace messages::func1 {


struct request_builder final : public tyrtech::message::struct_builder<2, 0>
{
    request_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct request_parser final : public tyrtech::message::struct_parser<2, 0>
{
    request_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    request_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

struct response_builder final : public tyrtech::message::struct_builder<2, 0>
{
    response_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct response_parser final : public tyrtech::message::struct_parser<2, 0>
{
    response_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    response_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

}

namespace messages::func2 {


struct request_builder final : public tyrtech::message::struct_builder<2, 0>
{
    request_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct request_parser final : public tyrtech::message::struct_parser<2, 0>
{
    request_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    request_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

struct response_builder final : public tyrtech::message::struct_builder<2, 0>
{
    response_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct response_parser final : public tyrtech::message::struct_parser<2, 0>
{
    response_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    response_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

}

DEFINE_SERVER_EXCEPTION(1, tyrtech::net::server_error, test1_error);
DEFINE_SERVER_EXCEPTION(2, tyrtech::net::server_error, test2_error);
DEFINE_SERVER_EXCEPTION(3, tyrtech::net::server_error, test3_error);

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
        case 1:
        {
            throw test1_error("{}", error.message());
        }
        case 2:
        {
            throw test2_error("{}", error.message());
        }
        case 3:
        {
            throw test3_error("{}", error.message());
        }
        default:
        {
            throw tyrtech::net::unknown_exception_error("#{}: unknown exception", error.code());
        }
    }
}

static constexpr uint16_t id{1234};

struct func1
{
    static constexpr uint16_t id{1};
    static constexpr uint16_t module_id{1234};

    using request_builder_t =
            messages::func1::request_builder;

    using request_parser_t =
            messages::func1::request_parser;

    using response_builder_t =
            messages::func1::response_builder;

    using response_parser_t =
            messages::func1::response_parser;

    static void throw_exception(const tyrtech::net::service::error_parser& error)
    {
        throw_module_exception(error);
    }
};

struct func2
{
    static constexpr uint16_t id{2};
    static constexpr uint16_t module_id{1234};

    using request_builder_t =
            messages::func2::request_builder;

    using request_parser_t =
            messages::func2::request_parser;

    using response_builder_t =
            messages::func2::response_builder;

    using response_parser_t =
            messages::func2::response_parser;

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
            case func1::id:
            {
                using request_parser_t =
                        typename func1::request_parser_t;

                using response_builder_t =
                        typename func1::response_builder_t;

                request_parser_t request(service_request.get_parser(),
                                         service_request.message());
                response_builder_t response(service_response->add_message());

                impl->func1(request, &response, ctx);

                break;
            }
            case func2::id:
            {
                using request_parser_t =
                        typename func2::request_parser_t;

                using response_builder_t =
                        typename func2::response_builder_t;

                request_parser_t request(service_request.get_parser(),
                                         service_request.message());
                response_builder_t response(service_response->add_message());

                impl->func2(request, &response, ctx);

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
namespace tests::module2 {


namespace messages::func1 {


struct request_builder final : public tyrtech::message::struct_builder<2, 0>
{
    request_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct request_parser final : public tyrtech::message::struct_parser<2, 0>
{
    request_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    request_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

struct response_builder final : public tyrtech::message::struct_builder<2, 0>
{
    response_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct response_parser final : public tyrtech::message::struct_parser<2, 0>
{
    response_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    response_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

}

namespace messages::func2 {


struct request_builder final : public tyrtech::message::struct_builder<2, 0>
{
    request_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct request_parser final : public tyrtech::message::struct_parser<2, 0>
{
    request_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    request_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

struct response_builder final : public tyrtech::message::struct_builder<2, 0>
{
    response_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct response_parser final : public tyrtech::message::struct_parser<2, 0>
{
    response_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    response_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

}

DEFINE_SERVER_EXCEPTION(1, tyrtech::net::server_error, test1_error);
DEFINE_SERVER_EXCEPTION(2, tyrtech::net::server_error, test2_error);
DEFINE_SERVER_EXCEPTION(3, tyrtech::net::server_error, test3_error);

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
        case 1:
        {
            throw test1_error("{}", error.message());
        }
        case 2:
        {
            throw test2_error("{}", error.message());
        }
        case 3:
        {
            throw test3_error("{}", error.message());
        }
        default:
        {
            throw tyrtech::net::unknown_exception_error("#{}: unknown exception", error.code());
        }
    }
}

static constexpr uint16_t id{1235};

struct func1
{
    static constexpr uint16_t id{1};
    static constexpr uint16_t module_id{1235};

    using request_builder_t =
            messages::func1::request_builder;

    using request_parser_t =
            messages::func1::request_parser;

    using response_builder_t =
            messages::func1::response_builder;

    using response_parser_t =
            messages::func1::response_parser;

    static void throw_exception(const tyrtech::net::service::error_parser& error)
    {
        throw_module_exception(error);
    }
};

struct func2
{
    static constexpr uint16_t id{2};
    static constexpr uint16_t module_id{1235};

    using request_builder_t =
            messages::func2::request_builder;

    using request_parser_t =
            messages::func2::request_parser;

    using response_builder_t =
            messages::func2::response_builder;

    using response_parser_t =
            messages::func2::response_parser;

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
            case func1::id:
            {
                using request_parser_t =
                        typename func1::request_parser_t;

                using response_builder_t =
                        typename func1::response_builder_t;

                request_parser_t request(service_request.get_parser(),
                                         service_request.message());
                response_builder_t response(service_response->add_message());

                impl->func1(request, &response, ctx);

                break;
            }
            case func2::id:
            {
                using request_parser_t =
                        typename func2::request_parser_t;

                using response_builder_t =
                        typename func2::response_builder_t;

                request_parser_t request(service_request.get_parser(),
                                         service_request.message());
                response_builder_t response(service_response->add_message());

                impl->func2(request, &response, ctx);

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
