#include <common/disallow_copy.h>
#include <common/disallow_move.h>
#include <common/system_error.h>
#include <common/exception.h>
#include <gt/engine.h>
#include <gt/condition.h>
#include <io/engine.h>
#include <io/queue_flow.h>

#include <sys/file.h>
#include <limits.h>
#include <liburing.h>


namespace tyrtech::io::io_uring {


struct request
{
    gt::context_t context{gt::current_context()};
    int32_t res{-1};
};

class engine : private disallow_copy, disallow_move
{
public:
    io_uring_sqe* get_sqe();

public:
    engine(uint32_t queue_size);
    ~engine();

private:
    queue_flow m_queue_flow;
    ::io_uring m_io_uring;

private:
    void io_uring_thread();
};

engine::engine(uint32_t queue_size)
  : m_queue_flow(queue_size)
{
    auto res = io_uring_queue_init(queue_size, &m_io_uring, 0);

    if (unlikely(res < 0))
    {
        throw runtime_error("io_uring_init(): {}", system_error(-res).message);
    }

    gt::create_system_thread(&engine::io_uring_thread, this);
}

engine::~engine()
{
    io_uring_queue_exit(&m_io_uring);
}

io_uring_sqe* engine::get_sqe()
{
    m_queue_flow.acquire();

    io_uring_sqe* sqe = io_uring_get_sqe(&m_io_uring);
    assert(likely(sqe != nullptr));

    return sqe;
}

void engine::io_uring_thread()
{
    uint32_t last_enqueued = 0;

    while (true)
    {
        if (unlikely(m_queue_flow.enqueued() == 0))
        {
            if (unlikely(gt::terminated() == true))
            {
                break;
            }
        }
        else
        {
            uint32_t sleep = (gt::user_contexts_waiting() > 0) ? 0 : 1;

            if (last_enqueued != m_queue_flow.enqueued() || sleep == 1)
            {
                auto res = io_uring_submit_and_wait(&m_io_uring, sleep);

                if (unlikely(res < 0))
                {
                    throw runtime_error("io_uring_submit_and_wait(): {}",
                                        system_error(-res).message);
                }
            }

            io_uring_cqe* cqe;

            uint32_t head;
            uint32_t count = 0;

            io_uring_for_each_cqe(&m_io_uring, head, cqe)
            {
                request* req = reinterpret_cast<request*>(io_uring_cqe_get_data(cqe));

                if (req != nullptr)
                {
                    req->res = cqe->res;
                    enqueue(req->context);
                }

                count++;
            }

            io_uring_cq_advance(&m_io_uring, count);

            m_queue_flow.release(count);
            last_enqueued = m_queue_flow.enqueued();
        }

        gt::yield();
    }
}

}


namespace tyrtech::io {


thread_local std::unique_ptr<io_uring::engine> __io_uring;


void initialize(uint32_t queue_size)
{
    __io_uring = std::make_unique<io_uring::engine>(queue_size);
}

io_uring_sqe* get_sqe()
{
    return __io_uring->get_sqe();
}

void add_timeout_to(io_uring_sqe* sqe, uint64_t timeout)
{
    sqe->flags |= IOSQE_IO_LINK;

    timeout *= 1000000;

    __kernel_timespec ts;

    ts.tv_sec = static_cast<int64_t>(timeout / 1000000000);
    ts.tv_nsec = static_cast<int64_t>(timeout % 1000000000);

    io_uring_prep_link_timeout(get_sqe(), &ts, 0);
}

int32_t wait_for(io_uring::request* request)
{
    gt::yield(false);

    if (unlikely(request->res < 0))
    {
        errno = -request->res;
        return -1;
    }

    return request->res;
}

int32_t preadv(int32_t fd, iovec* iov, uint32_t size, int64_t offset)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_readv(sqe, fd, iov, size, offset);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    return wait_for(&request);
}

int32_t pwritev(int32_t fd, iovec* iov, uint32_t size, int64_t offset)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_writev(sqe, fd, iov, size, offset);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    return wait_for(&request);
}

int32_t pread(int32_t fd, void* buffer, uint32_t size, int64_t offset)
{
    iovec iov[1];

    iov[0].iov_base = buffer;
    iov[0].iov_len = size;

    return io::preadv(fd, iov, 1, offset);
}

int32_t pwrite(int32_t fd, const void* buffer, uint32_t size, int64_t offset)
{
    iovec iov[1];

    iov[0].iov_base = const_cast<void*>(buffer);
    iov[0].iov_len = size;

    return io::pwritev(fd, iov, 1, offset);
}

int32_t send(int32_t fd, const char* buffer, uint32_t size, int32_t flags, uint64_t timeout)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_send(sqe, fd, buffer, size, flags);
    io_uring_sqe_set_data(sqe, &request);

    if (timeout != 0)
    {
        add_timeout_to(sqe, timeout);
    }

    return wait_for(&request);
}

int32_t recv(int32_t fd, char* buffer, uint32_t size, int32_t flags, uint64_t timeout)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_recv(sqe, fd, buffer, size, flags);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    if (timeout != 0)
    {
        add_timeout_to(sqe, timeout);
    }

    return wait_for(&request);
}

int32_t accept(int32_t fd, sockaddr* address, uint32_t* address_size, uint64_t timeout)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_accept(sqe, fd, address, address_size, 0);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    if (timeout != 0)
    {
        add_timeout_to(sqe, timeout);
    }

    return wait_for(&request);
}

int32_t connect(int32_t fd, const sockaddr* address, uint32_t address_size, uint64_t timeout)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_connect(sqe, fd, const_cast<sockaddr*>(address), address_size);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    if (timeout != 0)
    {
        add_timeout_to(sqe, timeout);
    }

    return wait_for(&request);
}

int32_t allocate(int32_t fd, int32_t mode, uint64_t offset, uint64_t size)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_fallocate(sqe, fd, mode, offset, size);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    return wait_for(&request);
}

int32_t close(int32_t fd)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_close(sqe, fd);
    io_uring_sqe_set_data(sqe, &request);

    return wait_for(&request);
}

int32_t sync(int32_t fd, uint32_t flags)
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_fsync(sqe, fd, flags);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    return wait_for(&request);
}

int32_t nop()
{
    io_uring::request request;
    io_uring_sqe* sqe = get_sqe();

    io_uring_prep_nop(sqe);
    io_uring_sqe_set_data(sqe, &request);

    return wait_for(&request);
}

}


namespace tyrtech::gt {


int32_t sleep(uint64_t msec)
{
    msec *= 1000000;

    __kernel_timespec ts;

    ts.tv_sec = static_cast<int64_t>(msec / 1000000000);
    ts.tv_nsec = static_cast<int64_t>(msec % 1000000000);

    io::io_uring::request request;
    io_uring_sqe* sqe = io::get_sqe();

    io_uring_prep_timeout(sqe, &ts, 0, 0);
    io_uring_sqe_set_data(sqe, &request);
    io_uring_sqe_set_flags(sqe, IOSQE_ASYNC);

    return io::wait_for(&request);
}

}
