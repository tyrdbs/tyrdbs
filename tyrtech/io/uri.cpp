#include <common/branch_prediction.h>
#include <io/tcp_channel.h>
#include <io/unix_channel.h>
#include <io/uri.h>

#include <regex>
#include <cassert>


namespace tyrtech::io::uri {


enum class proto
{
    TCP = 1,
    UNIX
};


std::regex __uri("([^:]+)://([^:/@]+)?(:([^/@]+))?(.*)?",
                 std::regex::optimize | std::regex::ECMAScript);


decltype(auto) parse(const std::string_view& uri)
{
    struct
    {
        uri::proto proto{0};

        std::string_view host;
        std::string_view service;
        std::string_view path;
    } params;

    std::cmatch m;

    if (unlikely(std::regex_match(uri.data(), m, __uri) == false))
    {
        throw runtime_error("{}: malformed uri", uri);
    }

    if (unlikely(m.size() != 6))
    {
        throw runtime_error("{}: malformed uri", uri);
    }

    std::string_view proto(m[1].first, m[1].length());

    params.host = std::string_view(m[2].first, m[2].length());
    params.service = std::string_view(m[4].first, m[4].length());
    params.path = std::string_view(m[5].first, m[5].length());

    if (proto == "tcp")
    {
        if (unlikely(params.path.size() != 0))
        {
            throw runtime_error("{}: malformed uri", uri);
        }

        params.proto = proto::TCP;
    }
    else if (proto == "unix")
    {
        bool malformed_uri = false;

        malformed_uri |= params.host.size() != 0;
        malformed_uri |= params.service.size() != 0;

        if (unlikely(malformed_uri == true))
        {
            throw runtime_error("{}: malformed uri", uri);
        }

        params.proto = proto::UNIX;
    }
    else
    {
        throw runtime_error("{}: unknown protocol", uri);
    }

    return params;
}


std::shared_ptr<channel> connect(const std::string_view& uri, uint64_t timeout)
{
    auto params = parse(uri);

    switch (params.proto)
    {
        case proto::TCP:
        {
            return tcp::connect(params.host, params.service, timeout);
        }
        case proto::UNIX:
        {
            return unix::connect(params.path, timeout);
        }
        default:
        {
            assert(false);
        }
    }
}

std::shared_ptr<channel> listen(const std::string_view& uri)
{
    auto params = parse(uri);

    switch (params.proto)
    {
        case proto::TCP:
        {
            return tcp::listen(params.host, params.service);
        }
        case proto::UNIX:
        {
            return unix::listen(params.path);
        }
        default:
        {
            assert(false);
        }
    }
}

}
