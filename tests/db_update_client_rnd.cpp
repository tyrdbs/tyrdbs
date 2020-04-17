#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <common/clock.h>
#include <common/logger.h>
#include <gt/engine.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_client.h>

#include <tests/db_server_service.json.h>
#include <tests/data.json.h>

#include <random>
#include <set>


using namespace tyrtech;


struct reader
{
    std::set<uint64_t>::iterator iterator;
    std::set<uint64_t>::iterator end;
    uint64_t value{0};
    std::string value_part;
};


bool fill_entries(reader* r, message::builder* builder)
{
    uint16_t data_flags = 0;

    auto data = tests::data_builder(builder);

    auto&& dbs = data.add_collections();
    auto&& db = dbs.add_value();
    auto&& entries = db.add_entries();

    while (true)
    {
        uint64_t key = __builtin_bswap64(*r->iterator);
        std::string_view key_str(reinterpret_cast<const char*>(&key), sizeof(key));

        uint8_t entry_flags = 0;

        entry_flags |= 0x01;
        //flags |= 0x02;

        uint16_t bytes_required = 0;

        bytes_required += tests::entry_builder::bytes_required();
        bytes_required += tests::entry_builder::key_bytes_required();
        bytes_required += tests::entry_builder::value_bytes_required();
        bytes_required += tests::entry_builder::ushard_bytes_required();
        bytes_required += sizeof(key);

        if (builder->available_space() <= bytes_required)
        {
            break;
        }

        uint16_t part_size = builder->available_space();
        part_size -= bytes_required;

        part_size = std::min(part_size, static_cast<uint16_t>(r->value_part.size()));

        if (part_size != r->value_part.size())
        {
            entry_flags &= ~0x01;
        }

        auto&& entry = entries.add_value();

        entry.set_flags(entry_flags);
        entry.add_key(key_str);
        entry.add_value(r->value_part.substr(0, part_size));
        entry.add_ushard(__builtin_bswap64(key));

        if (part_size != r->value_part.size())
        {
            r->value_part = r->value_part.substr(part_size);
            break;
        }

        if (++r->iterator == r->end)
        {
            data_flags |= 0x01;
            break;
        }

        r->value = *r->iterator;
        r->value_part = std::string_view(reinterpret_cast<const char*>(&r->value),
                                         sizeof(r->value));
    }

    data.set_flags(data_flags);

    return (data_flags & 0x01) == 0x01;
}


void update_thread(const std::string_view& uri,
                   uint32_t seed,
                   uint32_t iterations,
                   uint32_t keys,
                   uint32_t target_rate,
                   FILE* stats_fd)
{
    net::rpc_client<8192> c(io::uri::connect(uri, 0));

    std::mt19937 generator(seed);
    std::uniform_int_distribution<uint32_t> distribution(0, static_cast<uint32_t>(-1));

    auto t0 = clock::now();

    uint64_t inserted_keys = 0;

    for (uint32_t i = 0; i < iterations; i++)
    {
        std::set<uint64_t> key_set;

        while (key_set.size() != keys)
        {
            key_set.insert(distribution(generator));
        }

        reader r;
        r.iterator = key_set.begin();
        r.end = key_set.end();

        r.value = *r.iterator;
        r.value_part = std::string_view(reinterpret_cast<const char*>(&r.value),
                                        sizeof(r.value));

        uint64_t handle = 0;

        auto t1 = clock::now();

        while (true)
        {
            auto update_data = c.remote_call<tests::collections::update_data>();

            auto req = update_data.request();

            if (handle != 0)
            {
                req.add_handle(handle);
            }

            bool done = fill_entries(&r, req.add_data());

            update_data.execute();
            update_data.wait();

            if (handle == 0)
            {
                assert(update_data.response().has_handle() == true);
                handle = update_data.response().handle();
            }
            else
            {
                assert(update_data.response().has_handle() == false);
            }

            if (done == true)
            {
                break;
            }

            assert(handle != 0);
        }

        auto commit_update = c.remote_call<tests::collections::commit_update>();

        auto req = commit_update.request();

        req.add_handle(handle);

        commit_update.execute();
        commit_update.wait();

        auto t2 = clock::now();

        inserted_keys += keys;

        uint64_t iter_t = t2 - t1;
        uint64_t total_t = t2 - t0;

        logger::notice("#{} updated {} keys in {:.2f} ms, {:.2f} keys/s",
                       i,
                       keys,
                       iter_t / 1000000.,
                       inserted_keys * 1000000000. / total_t);

        if (stats_fd != nullptr)
        {
            fmt::print(stats_fd,
                       "{},{},{}\n",
                       t2 / 1000,
                       keys,
                       iter_t);

            fflush(stats_fd);
        }

        if (target_rate != 0)
        {
            uint64_t target_t = inserted_keys * 1000000000 / target_rate;

            if (target_t > total_t)
            {
                uint64_t delay = target_t - total_t;
                gt::sleep(delay / 1000000);
            }
        }
    }
}


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Network update client demo.", nullptr);

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

    cmd.add_param("iterations",
                  nullptr,
                  "iterations",
                  "num",
                  "10000",
                  {"iterations to run (default is 10000)"});

    cmd.add_param("keys",
                  nullptr,
                  "keys",
                  "num",
                  "10000",
                  {"keys to update in one iteration (default: 10000)"});

    cmd.add_param("update-rate",
                  nullptr,
                  "update-rate",
                  "num",
                  "0",
                  {"update rate limit in keys per second (default: unlimited)"});

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

        fmt::print(stats_fd, "#ts[us],keys,duration[ns]\n");
    }

    gt::create_thread(update_thread,
                      cmd.get<std::string_view>("uri"),
                      cmd.get<uint32_t>("seed"),
                      cmd.get<uint32_t>("iterations"),
                      cmd.get<uint32_t>("keys"),
                      cmd.get<uint32_t>("update-rate"),
                      stats_fd);

    gt::run();

    if (stats_fd != nullptr)
    {
        fclose(stats_fd);
    }

    return 0;
}
