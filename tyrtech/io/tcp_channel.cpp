#include <common/branch_prediction.h>
#include <common/system_error.h>
#include <io/tcp_channel.h>

#include <cassert>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>


namespace tyrtech::io::tcp {


class channel : public io::channel
{
public:
    static std::shared_ptr<io::channel> connect(const std::string_view& host,
                                                const std::string_view& service,
                                                uint64_t timeout);

    static std::shared_ptr<io::channel> listen(const std::string_view& host,
                                               const std::string_view& service);

public:
    std::shared_ptr<io::channel> accept() override;

public:
    channel(int32_t fd, const sockaddr_in& addr);

private:
    void to_uri(const sockaddr_in& addr);
};


int32_t create_socket()
{
    int32_t s = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);

    if (unlikely(s == -1))
    {
        throw runtime_error("socket(): {}", system_error().message);
    }

    return s;
}

sockaddr_in resolve(const std::string_view& host, const std::string_view& service)
{
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addrinfo hints;

    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    addrinfo* returned_addrs{nullptr};

    char host_str[128];
    format_to(host_str, sizeof(host_str), "{}", host);

    char service_str[128];
    format_to(service_str, sizeof(service_str), "{}", service);

    auto res = getaddrinfo(host_str, service_str, &hints, &returned_addrs);

    if (res != 0)
    {
        throw channel::address_not_found_error("tcp://{}:{}", host, service);
    }

    assert(likely(sizeof(addr) == returned_addrs->ai_addrlen));
    addr = *reinterpret_cast<sockaddr_in*>(returned_addrs->ai_addr);

    freeaddrinfo(returned_addrs);

    return addr;
}

std::shared_ptr<io::channel> channel::connect(const std::string_view& host,
                                              const std::string_view& service,
                                              uint64_t timeout)
{
    auto addr = resolve(host, service);

    auto c = std::make_shared<channel>(create_socket(), addr);

    c->io::channel::connect(&addr, sizeof(addr), timeout);

    return std::static_pointer_cast<io::channel>(c);
}

std::shared_ptr<io::channel> channel::listen(const std::string_view& host,
                                             const std::string_view& service)
{
    auto addr = resolve(host, service);

    auto c = std::make_shared<channel>(create_socket(), addr);

    c->io::channel::listen(&addr, sizeof(addr));

    return std::static_pointer_cast<io::channel>(c);
}

std::shared_ptr<io::channel> channel::accept()
{
    int32_t fd{-1};
    sockaddr_in addr;

    io::channel::accept(&fd, &addr, sizeof(addr));

    return std::make_shared<channel>(fd, addr);
}

channel::channel(int32_t fd, const sockaddr_in& addr)
  : io::channel(fd)
{
    to_uri(addr);

    int32_t val = 1;

    if (unlikely(::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val))) == -1)
    {
        throw runtime_error("{}: {}", uri(), system_error().message);
    }

    if (unlikely(::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val))) == -1)
    {
        throw runtime_error("{}: {}", uri(), system_error().message);
    }
}

void channel::to_uri(const sockaddr_in& addr)
{
    m_uri_view = format_to(m_uri, sizeof(m_uri),
                           "tcp://{}:{}",
                           inet_ntoa(addr.sin_addr),
                           ntohs(addr.sin_port));
}

std::shared_ptr<io::channel> connect(const std::string_view& host, const std::string_view& service, uint64_t timeout)
{
    return channel::connect(host, service, timeout);
}

std::shared_ptr<io::channel> listen(const std::string_view& host, const std::string_view& service)
{
    return channel::listen(host, service);
}

}
