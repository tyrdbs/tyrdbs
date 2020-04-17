#include <common/branch_prediction.h>
#include <common/system_error.h>
#include <common/clock.h>
#include <io/engine.h>
#include <io/channel.h>
#include <io/queue_flow.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cassert>


namespace tyrtech::io {


static thread_local std::unique_ptr<queue_flow> __queue_flow;


void channel::initialize(uint32_t queue_size)
{
    __queue_flow = std::make_unique<queue_flow>(queue_size);
}

uint32_t channel::recv(char* data, uint32_t size, uint64_t timeout)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::recv(m_fd, data, size, 0, timeout);

    if (likely(res > 0))
    {
        return static_cast<uint32_t>(res);
    }

    if (unlikely(res == 0))
    {
        throw disconnected_error("{}", uri());
    }

    auto e = system_error();

    switch (e.code)
    {
        case ECONNRESET:
        {
            throw disconnected_error("{}", uri());
        }
        case ECANCELED:
        {
            throw timeout_error("{}", uri());
        }
        default:
        {
            throw runtime_error("{}: {}", uri(), e.message);
        }
    }
}

uint32_t channel::send(const char* data, uint32_t size, uint64_t timeout)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::send(m_fd, data, size, 0, timeout);

    if (likely(res > 0))
    {
        return static_cast<uint32_t>(res);
    }

    if (unlikely(res == 0))
    {
        throw disconnected_error("{}", uri());
    }

    auto e = system_error();

    switch (e.code)
    {
        case ECONNRESET:
        {
            throw disconnected_error("{}", uri());
        }
        case ECANCELED:
        {
            throw timeout_error("{}", uri());
        }
        default:
        {
            throw runtime_error("{}: {}", uri(), e.message);
        }
    }
}

void channel::send_all(const char* data, uint32_t size, uint64_t timeout)
{
    while (size != 0)
    {
        uint64_t t1 = clock::now();

        uint32_t res = send(data, size, timeout);

        uint64_t send_took = clock::now() - t1;

        if (likely(timeout > send_took))
        {
            timeout -= send_took;
        }
        else
        {
            timeout = 1;
        }

        data += res;
        size -= res;
    }
}

void channel::disconnect()
{
    ::shutdown(m_fd, SHUT_RDWR);
}

std::string_view channel::uri() const
{
    return m_uri_view;
}

channel::channel(int32_t fd)
  : m_fd(fd)
{
}

channel::~channel()
{
    if (unlikely(m_fd == -1))
    {
        return;
    }

    io::close(m_fd);
    m_fd = -1;
}

void channel::connect(const void* address, uint32_t address_size, uint64_t timeout)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::connect(m_fd,
                           reinterpret_cast<const sockaddr*>(address),
                           address_size,
                           timeout);

    if (res == 0)
    {
        return;
    }

    assert(likely(res == -1));

    auto e = system_error();

    switch (e.code)
    {
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ECONNRESET:
        case ENOENT:
        {
            throw unable_to_connect_error("{}", uri());
        }
        case ECANCELED:
        {
            throw timeout_error("{}", uri());
        }
        default:
        {
            throw runtime_error("{}: {}", uri(), e.message);
        }
    }
}

void channel::listen(const void* address, uint32_t address_size)
{
    if (unlikely(::bind(m_fd,
                        reinterpret_cast<const sockaddr*>(address),
                        address_size) == -1))
    {
        auto error = system_error();

        if (error.code == EADDRINUSE)
        {
            throw address_in_use_error("{}: {}", uri(), error.message);
        }
        else
        {
            throw runtime_error("{}: {}", uri(), error.message);
        }
    }

    if (unlikely(::listen(m_fd, 1024) == -1))
    {
        throw runtime_error("{}: {}", uri(), system_error().message);
    }
}

void channel::accept(int32_t* fd, void* address, uint32_t address_size)
{
    queue_flow::resource r(*__queue_flow);

    socklen_t addr_size = address_size;
    std::memset(address, 0, addr_size);

    *fd = io::accept(m_fd, reinterpret_cast<sockaddr*>(address), &addr_size, 0);

    if (likely(*fd != -1))
    {
        return;
    }

    auto e = system_error();

    switch (e.code)
    {
        case EINVAL:
        {
            throw disconnected_error("{}", uri());
        }
        default:
        {
            throw runtime_error("{}: {}", uri(), e.message);
        }
    }
}

}
