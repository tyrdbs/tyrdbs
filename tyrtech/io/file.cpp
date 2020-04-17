#include <common/branch_prediction.h>
#include <common/system_error.h>
#include <common/logger.h>
#include <gt/condition.h>
#include <io/engine.h>
#include <io/file.h>
#include <io/queue_flow.h>

#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>


namespace tyrtech::io {


static thread_local std::unique_ptr<queue_flow> __queue_flow;


void file::initialize(uint32_t queue_size)
{
    __queue_flow = std::make_unique<queue_flow>(queue_size);
}

void file::open(int32_t flags, int32_t mode)
{
    m_fd = ::open(m_path, flags, mode);

    if (unlikely(m_fd == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }
}

void file::mkostemp(int32_t flags)
{
    m_fd = ::mkostemp(m_path, flags);

    if (unlikely(m_fd == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }
}

void file::pread(uint64_t offset, char* data, uint32_t size)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::pread(m_fd, data, size, offset);

    if (unlikely(res == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }

    if (unlikely(res != static_cast<int32_t>(size)))
    {
        throw error("{}: unable to read", m_path);
    }
}

void file::pwrite(uint64_t offset, const char* data, uint32_t size)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::pwrite(m_fd, data, size, offset);

    if (unlikely(res == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }

    if (unlikely(res != static_cast<int32_t>(size)))
    {
        throw error("{}: unable to write", m_path);
    }
}

uint32_t file::preadv(uint64_t offset, iovec* iov, uint32_t size)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::preadv(m_fd, iov, size, offset);

    if (unlikely(res == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }

    return static_cast<uint32_t>(res);
}

uint32_t file::pwritev(uint64_t offset, iovec* iov, uint32_t size)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::pwritev(m_fd, iov, size, offset);

    if (unlikely(res == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }

    return static_cast<uint32_t>(res);
}

void file::allocate(int32_t mode, uint64_t offset, uint64_t size)
{
    queue_flow::resource r(*__queue_flow);

    auto res = io::allocate(m_fd, mode, offset, size);

    if (unlikely(res == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }
}

struct stat64 file::stat()
{
    struct stat64 stat;

    if (auto res = ::fstat64(m_fd, &stat); unlikely(res == -1))
    {
        throw error("{}: {}", m_path, system_error().message);
    }

    return stat;
}

bool file::try_lock()
{
    if (auto res = ::flock(m_fd, LOCK_NB | LOCK_EX); unlikely(res == -1))
    {
        if (auto e = system_error(); e.code != EWOULDBLOCK)
        {
            throw error("{}: {}", m_path, e.message);
        }

        return false;
    }

    return true;
}

void file::unlink()
{
    if (auto res = ::unlink(m_path); unlikely(res == -1))
    {
        logger::warning("{}: {}", m_path, system_error().message);
    }
}

std::string_view file::path() const
{
    return m_path_view;
}

int32_t file::fd() const
{
    return m_fd;
}

file::~file()
{
    if (unlikely(m_fd == -1))
    {
        return;
    }

    io::close(m_fd);
    m_fd = -1;
}

file::file(file&& other)
{
    *this = std::move(other);
}

file& file::operator=(file&& other)
{
    m_fd = other.m_fd;
    other.m_fd = -1;

    std::memcpy(m_path, other.m_path, other.m_path_view.size());
    other.m_path[0] = 0;

    m_path_view = std::string_view(m_path, other.m_path_view.size());
    other.m_path_view = std::string_view();

    return *this;
}

}
