#define _SDT_HAS_SEMAPHORES 1
#include <sys/sdt.h>


__extension__ unsigned short dtrace_test_test1_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes"))) __attribute__ ((visibility ("hidden")));
__extension__ unsigned short dtrace_test_test2_semaphore __attribute__ ((unused)) __attribute__ ((section (".probes"))) __attribute__ ((visibility ("hidden")));



namespace tests {



struct dtrace_test_test1_probe
{
    uint64_t arg1;
    int32_t arg2;
    int32_t arg3;
    const char* arg4;

    dtrace_test_test1_probe(const uint64_t& arg1, const int32_t& arg2, const int32_t& arg3, const char*& arg4)
      : arg1(arg1)
      , arg2(arg2)
      , arg3(arg3)
      , arg4(arg4)
    {
    }

    dtrace_test_test1_probe() noexcept = default;

    void fire() const
    {
        DTRACE_PROBE4(dtrace_test, test1, arg1, arg2, arg3, arg4);
    }

    static inline bool is_enabled()
    {
        return __builtin_expect(dtrace_test_test1_semaphore, 0);
    }
};

struct dtrace_test_test2_probe
{
    uint64_t duration;
    int32_t arg1;

    dtrace_test_test2_probe(const uint64_t& duration, const int32_t& arg1)
      : duration(duration)
      , arg1(arg1)
    {
    }

    dtrace_test_test2_probe() noexcept = default;

    void fire() const
    {
        DTRACE_PROBE2(dtrace_test, test2, duration, arg1);
    }

    static inline bool is_enabled()
    {
        return __builtin_expect(dtrace_test_test2_semaphore, 0);
    }
};

}
