#include <common/clock.h>

#include <utility>


namespace tyrtech {


template<typename T>
struct timer_probe
{
    T probe;
    uint64_t timestamp;

    template<typename... Arguments>
    timer_probe(Arguments&&... arguments)
      : probe(std::forward<Arguments>(arguments)...)
      , timestamp(clock::now())
    {
        static_assert(std::is_same<decltype(probe.duration), uint64_t>(), "duration has to be uint64_t");
    }

    ~timer_probe()
    {
        if (T::is_enabled() == true)
        {
            probe.duration = clock::now() - timestamp;
            probe.fire();
        }
    }
};

}
