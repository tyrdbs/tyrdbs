#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <gt/engine.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_client.h>

#include <tests/db_server_service.json.h>
#include <tests/data.json.h>
#include <tests/stats.h>

#include <random>


using namespace tyrtech;


void fetch_thread(const std::string_view& uri, uint32_t seed, FILE* stats_fd)
{
    net::rpc_client<8192> c(io::uri::connect(uri, 0));

    std::mt19937 generator(seed);
    std::uniform_int_distribution<uint32_t> distribution(0, static_cast<uint32_t>(-1));

    auto s = std::make_unique<tests::stats>();

    while (true)
    {
        auto fetch_data = c.remote_call<tests::collections::fetch_data>();

        auto req = fetch_data.request();

        uint64_t key = __builtin_bswap64(distribution(generator));
        std::string_view key_str(reinterpret_cast<const char*>(&key), sizeof(key));

        req.add_min_key(key_str);
        req.add_max_key(key_str);
        req.add_ushard(__builtin_bswap64(key));

        {
            auto sw = s->stopwatch();

            fetch_data.execute();
            fetch_data.wait();
        }

        auto res = fetch_data.response();

        assert(res.has_handle() == false);
        assert(res.has_data() == true);

        tests::data_parser data(res.get_parser(), res.data());
        assert((data.flags() & 0x01) == 0x01);

        auto&& dbs = data.collections();

        assert(dbs.next() == true);
        auto&& db = dbs.value();

        auto&& entries = db.entries();

        std::string value;

        while (entries.next() == true)
        {
            auto&& entry = entries.value();

            assert(entry.key().compare(key_str) == 0);

            value += entry.value();

            if ((entry.flags() & 0x0001) == 0x0001)
            {
                break;
            }
        }

        assert(value.size() == 8);

        key = __builtin_bswap64(key);
        assert(value.compare(std::string_view(reinterpret_cast<char*>(&key), 8)) == 0);

        if (s->n() == 10000)
        {
            auto ts = clock::now();

            logger::notice("fetch ({}): {} keys in {:.2f} s, \033[1m{:.2f}\033[0m keys/s",
                           ts,
                           s->n(),
                           s->sum() / 1000000000.,
                           s->n() * 1000000000. / s->sum());
            logger::notice("  [us] avg: \033[1m{:.2f}\033[0m, 99%: {:.2f}, "
                           "99.99%: \033[1m{:.2f}\033[0m, max: \033[31m{:.2f}\033[0m",
                           s->avg() / 1000.,
                           s->value_at(0.99) / 1000.,
                           s->value_at(0.9999) / 1000.,
                           s->max() / 1000.);
            logger::notice("");

            if (stats_fd != nullptr)
            {
                fmt::print(stats_fd,
                           "{},{},{},{:.2f},{:.2f},{}\n",
                           ts,
                           s->n(),
                           s->sum(),
                           s->value_at(0.99),
                           s->value_at(0.9999),
                           s->max());

                fflush(stats_fd);
            }

            s = std::make_unique<tests::stats>();
        }
    }
}


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Network fetch client demo.", nullptr);

    cmd.add_param("seed",
                  nullptr,
                  "seed",
                  "num",
                  "0",
                  {"pseudo random seed to use (default is 0)"});

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

    cmd.add_param("stats-output",
                  nullptr,
                  "stats-output",
                  "file",
                  nullptr,
                  {"output csv formatted stats to file"});

    cmd.add_param("uri",
                  "<uri>",
                  {"uri to connect to"});

    cmd.parse(argc, argv);

    set_cpu(cmd.get<uint32_t>("cpu"));

    gt::initialize();
    io::initialize(4096);
    io::channel::initialize(cmd.get<uint32_t>("network-queue-depth"));

    FILE* stats_fd = nullptr;

    if (cmd.has("stats-output") == true)
    {
        stats_fd = fopen(cmd.get<std::string_view>("stats-output").data(), "w");
        assert(stats_fd != nullptr);

        fmt::print(stats_fd, "#ts[us],n,sum[ns],99.00%[ns],99.99%[ns],max[ns]\n");
    }

    gt::create_thread(fetch_thread,
                      cmd.get<std::string_view>("uri"),
                      cmd.get<uint32_t>("seed"),
                      stats_fd);

    gt::run();

    if (stats_fd != nullptr)
    {
        fclose(stats_fd);
    }

    return 0;
}
