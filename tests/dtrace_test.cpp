#include <common/timer_probe.h>
#include <common/logger.h>

#include <tests/dtrace_test_probes.json.h>

#include <unistd.h>


using namespace tyrtech;



int main()
{
    int32_t i = 0;

    while (true)
    {
        if (tests::dtrace_test_test1_probe::is_enabled() == true)
        {
            logger::notice("firing test1");

            tests::dtrace_test_test1_probe p;

            p.arg1 = i;
            p.arg2 = 1;
            p.arg3 = 2;
            p.arg4 = "test";

            p.fire();
        }

        {
            timer_probe<tests::dtrace_test_test2_probe> t;
            t.probe.arg1 = 3;

            sleep(1);
        }

        i++;
    }

    return 0;
}
