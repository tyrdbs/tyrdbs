#include <common/cmd_line.h>

#include <random>
#include <map>
#include <set>


using namespace tyrtech;


using map_t =
        std::map<uint64_t, uint64_t>;

using set_t =
        std::set<uint64_t>;


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Generate test keys.", nullptr);

    cmd.add_param("seed",
                  nullptr,
                  "seed",
                  "num",
                  "0",
                  {"pseudo random seed to use (default is 0)"});

    cmd.add_param("iterations",
                  nullptr,
                  "iterations",
                  "num",
                  "100",
                  {"number of iterations per thread to do (default is 100)"});

    cmd.add_param("keys",
                  nullptr,
                  "keys",
                  "num",
                  "5000",
                  {"number of keys per iteration (default 5000)"});

    cmd.add_param("input-data",
                  nullptr,
                  "input-data",
                  "file",
                  "input.data",
                  {"input data file (default input.data)"});

    cmd.add_param("test-data",
                  nullptr,
                  "test-data",
                  "file",
                  "test.data",
                  {"test data file (default test.data)"});

    cmd.parse(argc, argv);

    map_t map;

    std::default_random_engine generator(cmd.get<uint32_t>("seed"));
    std::uniform_int_distribution<uint64_t> distribution;

    FILE* fd = fopen(cmd.get<std::string_view>("input-data").data(), "w");
    assert(fd != nullptr);

    for (uint32_t i = 0; i < cmd.get<uint32_t>("iterations"); i++)
    {
        set_t set;

        while (set.size() != cmd.get<uint32_t>("keys"))
        {
            uint64_t key = distribution(generator);

            set.insert(key);
            map[key] = i;
        }

        for (auto&& e : set)
        {
            fprintf(fd, "%016lx %u\n", e, i);
        }

        fprintf(fd, "== 0\n");
    }

    fclose(fd);

    fd = fopen(cmd.get<std::string_view>("test-data").data(), "w");
    assert(fd != nullptr);

    for (auto&& it : map)
    {
        fprintf(fd, "%016lx %lu\n", it.first, it.second);
    }

    fprintf(fd, "== 0\n");

    fclose(fd);

    return 0;
}
