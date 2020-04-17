#include <common/clock.h>
#include <common/logger.h>
#include <common/cpu_sched.h>

#include <liburing.h>
#include <pthread.h>

#include <array>
#include <cassert>


#define read_barrier()	__asm__ __volatile__("":::"memory")
#define write_barrier()	__asm__ __volatile__("":::"memory")


struct buffer
{
    uint64_t dummy1;
    uint64_t dummy2;
    uint64_t dummy3;
};


using array_t =
        std::array<buffer, 64>;

array_t queue;

uint32_t head = 0;
uint32_t tail = 0;
uint32_t mask = 0x3f;


buffer* next()
{
    read_barrier();

    if (head - tail == queue.size())
    {
        return nullptr;
    }

    return &queue[head & mask];
}

void push()
{
    head++;

    write_barrier();
}

buffer* peek()
{
    read_barrier();

    if (head == tail)
    {
        return nullptr;
    }

    return &queue[tail & mask];
}

void pop()
{
    tail++;

    write_barrier();
}


void* t1(void* arg)
{
    tyrtech::set_cpu(0);

    uint64_t i = 0;

    while (i < 1000000000)
    {
        buffer* b = next();

        if (b == nullptr)
        {
            continue;
        }

        b->dummy1 = i++;

        push();
    }

    return nullptr;
}

void* t2(void* arg)
{
    tyrtech::set_cpu(4);

    auto t1 = tyrtech::clock::now();

    uint64_t i = 0;

    while (i < 1000000000)
    {
        buffer* b = peek();

        if (b == nullptr)
        {
            continue;
        }

        assert(b->dummy1 == i++);

        pop();
    }

    auto t2 = tyrtech::clock::now();

    uint64_t duration = t2 - t1;

    tyrtech::logger::notice("got {} msgs in {:.2f} s, {:.2f} msg/s",
                            i, duration / 1000000000., i * 1000000000. / duration);

    return nullptr;
}

int main()
{
    pthread_t _t1;
    pthread_create(&_t1, nullptr, t1, nullptr);

    pthread_t _t2;
    pthread_create(&_t2, nullptr, t2, nullptr);

    pthread_join(_t1, nullptr);
    pthread_join(_t2, nullptr);

    return 0;
}
