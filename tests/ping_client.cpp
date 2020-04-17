#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <gt/engine.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_client.h>

#include <tests/stats.h>
#include <tests/ping_module.json.h>


using namespace tyrtech;


void client(uint32_t iterations, const std::string_view& uri, tests::stats* s)
{
    net::rpc_client<8192> c(io::uri::connect(uri, 0));

    for (uint32_t i = 0; i < iterations; i++)
    {
        auto ping = c.remote_call<tests::ping::ping>();

        auto req = ping.request();
        req.add_sequence(i);

        {
            auto sw = s->stopwatch();

            ping.execute();
            ping.wait();
        }

        auto res = ping.response();
        assert(i == res.sequence());
    }
}


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Ping client.", nullptr);

    cmd.add_param("network-queue-depth",
                  nullptr,
                  "network-queue-depth",
                  "num",
                  "512",
                  {"network queue depth to use (default is 512)"});

    cmd.add_param("cpu",
                  nullptr,
                  "cpu",
                  "index",
                  "0",
                  {"cpu index to run the program on (default is 0)"});

    cmd.add_param("threads",
                  nullptr,
                  "threads",
                  "num",
                  "32",
                  {"number of threads to use (default is 32)"});

    cmd.add_param("iterations",
                  nullptr,
                  "iterations",
                  "num",
                  "10000",
                  {"number of iterations per thread to do (default is 10000)"});

    cmd.add_param("uri",
                  "<uri>",
                  {"uri to connect to"});

    cmd.parse(argc, argv);

    set_cpu(cmd.get<uint32_t>("cpu"));

    gt::initialize();
    io::initialize(4096);
    io::channel::initialize(cmd.get<uint32_t>("network-queue-depth"));

    std::vector<tests::stats> s;

    for (uint32_t i = 0; i < cmd.get<uint32_t>("threads"); i++)
    {
        s.push_back(tests::stats());
    }

    for (uint32_t i = 0; i < cmd.get<uint32_t>("threads"); i++)
    {
        gt::create_thread(&client,
                          cmd.get<uint32_t>("iterations"),
                          cmd.get<std::string_view>("uri"),
                          &s[i]);
    }

    gt::run();

    for (uint32_t i = 0; i < s.size(); i++)
    {
        logger::notice("thread {} RTT [ns]:", i);
        logger::notice("");
        s[i].report();
        logger::notice("");
    }

    return 0;
}
