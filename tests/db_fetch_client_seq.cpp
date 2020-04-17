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


uint64_t calc_avg(net::rpc_client<8192>* c, uint64_t min, uint64_t max, uint32_t ushard)
{
    uint64_t sum = 0;
    uint32_t count = 0;

    uint64_t handle = 0;

    std::string last_key;
    std::string value;

    while (true)
    {
        auto fetch_data = c->remote_call<tests::collections::fetch_data>();

        auto req = fetch_data.request();

        if (handle == 0)
        {
            uint64_t min_key = __builtin_bswap64(min);
            std::string_view min_key_str(reinterpret_cast<const char*>(&min_key), sizeof(min_key));

            uint64_t max_key = __builtin_bswap64(max);
            std::string_view max_key_str(reinterpret_cast<const char*>(&max_key), sizeof(max_key));

            req.add_min_key(min_key_str);
            req.add_max_key(max_key_str);
            req.add_ushard(ushard);
        }
        else
        {
            req.add_handle(handle);
        }

        fetch_data.execute();
        fetch_data.wait();

        auto res = fetch_data.response();

        assert(res.has_data() == true);

        tests::data_parser data(res.get_parser(), res.data());

        auto&& dbs = data.collections();

        assert(dbs.next() == true);
        auto&& db = dbs.value();

        auto&& entries = db.entries();

        while (entries.next() == true)
        {
            auto&& entry = entries.value();

            bool eor = entry.flags() & 0x0001;
            //bool deleted = entry.flags() & 0x0002;

            if (last_key.size() == 0)
            {
                last_key = entry.key();
            }

            assert(last_key == entry.key());
            value += entry.value();

            if (eor == true)
            {
                assert(value.size() == 8);

                sum += *reinterpret_cast<const uint64_t*>(value.data());
                count++;

                last_key.clear();
                value.clear();
            }
        }

        if (res.has_handle() == false)
        {
            assert((data.flags() & 0x01) == 0x01);

            break;
        }

        if (handle == 0)
        {
            handle = res.handle();
        }

        assert(handle == res.handle());
    }

    return count;
}


void fetch_thread(const std::string_view& uri,
                  uint32_t groups,
                  uint32_t group_bits,
                  uint32_t ushard_bits,
                  FILE* stats_fd)
{
    net::rpc_client<8192> c(io::uri::connect(uri, 0));

    auto s = std::make_unique<tests::stats>();

    uint32_t group = 0;
    uint32_t keys = 0;

    while (true)
    {
        {
            auto sw = s->stopwatch();

            for (int32_t i = static_cast<int32_t>(group) - groups;
                 i < static_cast<int32_t>(group);
                 i++)
            {
                if (i < 0)
                {
                    continue;
                }

                uint64_t min = static_cast<uint64_t>(i) << group_bits;
                uint64_t max = static_cast<uint64_t>(i) << group_bits;

                max |= (1UL << group_bits) - 1;

                uint32_t ushard = i & ((1UL << ushard_bits) - 1);

                keys += calc_avg(&c, min, max, ushard);
            }
        }

        if (s->n() == 100)
        {
            auto ts = clock::now();

            logger::notice("fetch ({}): {} keys in {:.2f} s, \033[1m{:.2f}\033[0m keys/s",
                           ts,
                           keys,
                           s->sum() / 1000000000.,
                           keys * 1000000000. / s->sum());
            logger::notice("  [ns] avg: \033[1m{:.2f}\033[0m, 99%: {:.2f}, "
                           "99.99%: \033[1m{:.2f}\033[0m, max: \033[31m{:.2f}\033[0m",
                           s->avg() / static_cast<float>(keys),
                           s->value_at(0.99) / static_cast<float>(keys),
                           s->value_at(0.9999) / static_cast<float>(keys),
                           s->max() / static_cast<float>(keys));
            logger::notice("");

            if (stats_fd != nullptr)
            {
                fmt::print(stats_fd,
                           "{},{},{},{:.2f},{:.2f},{}\n",
                           ts,
                           keys,
                           s->sum(),
                           s->value_at(0.99),
                           s->value_at(0.9999),
                           s->max());

                fflush(stats_fd);
            }

            s = std::make_unique<tests::stats>();
            keys = 0;
        }

        group++;
    }
}


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Network fetch client demo.", nullptr);

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

    cmd.add_param("groups",
                  nullptr,
                  "groups",
                  "num",
                  "30",
                  {"calculate the average of num last groups (default is 30)"});

    cmd.add_param("group-bits",
                  nullptr,
                  "group-bits",
                  "num",
                  "9",
                  {"number of bits to group values with (default: 9)"});

    cmd.add_param("ushard-bits",
                  nullptr,
                  "ushard-bits",
                  "num",
                  "5",
                  {"number of bits to use for usharding (default: 5)"});

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

        fmt::print(stats_fd, "#ts[us],keys,sum[ns],99.00%[ns],99.99%[ns],max[ns]\n");
    }

    gt::create_thread(fetch_thread,
                      cmd.get<std::string_view>("uri"),
                      cmd.get<uint32_t>("groups"),
                      cmd.get<uint32_t>("group-bits"),
                      cmd.get<uint32_t>("ushard-bits"),
                      stats_fd);

    gt::run();

    if (stats_fd != nullptr)
    {
        fclose(stats_fd);
    }

    return 0;
}
