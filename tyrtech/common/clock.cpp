#include <common/clock.h>

#include <time.h>


namespace tyrtech::clock {


uint64_t now() noexcept
{
    timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    return static_cast<uint64_t>(tp.tv_sec) * 1000000000 + tp.tv_nsec;
}

}
