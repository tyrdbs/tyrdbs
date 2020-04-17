#include <common/cpu_sched.h>
#include <common/exception.h>
#include <common/system_error.h>

#include <pthread.h>


namespace tyrtech {


void set_cpu(int32_t cpu)
{
    cpu_set_t cpu_set;

    CPU_ZERO(&cpu_set);
    CPU_SET(cpu, &cpu_set);

    auto res = pthread_setaffinity_np(pthread_self(),
                                      sizeof(cpu_set),
                                      &cpu_set);

    if (res != 0)
    {
        throw runtime_error("pthread_setaffinity_np({}): {}",
                            cpu,
                            system_error(res).message);
    }
}

}
