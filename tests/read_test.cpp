#include <common/cpu_sched.h>
#include <common/cmd_line.h>
#include <common/uuid.h>
#include <common/system_error.h>
#include <gt/engine.h>
#include <io/engine.h>
#include <io/file.h>

#include <tests/stats.h>

#include <random>
#include <sys/mount.h>
#include <sys/ioctl.h>


using namespace tyrtech;


enum class operation
{
    READ = 1,
    NOP,
    YIELD
};

void read_test(operation op,
               uint32_t seed,
               io::file* f,
               int32_t pages,
               uint32_t iterations,
               tests::stats* s)
{
    std::mt19937 generator(seed);
    std::uniform_int_distribution distribution(0, pages - 1);

    using buffer_t =
            std::array<char, 4096>;

    buffer_t buffer __attribute__ ((aligned(4096)));
    assert((reinterpret_cast<uint64_t>(buffer.data()) & 0xfff) == 0);

    for (uint32_t i = 0; i < iterations; i++)
    {
        uint32_t block = distribution(generator);

        auto sw = s->stopwatch();

        switch (op)
        {
            case operation::READ:
            {
                f->pread(block * buffer.size(),
                         buffer.data(),
                         buffer.size());

                break;
            }
            case operation::NOP:
            {
                io::nop();

                break;
            }
            case operation::YIELD:
            {
                gt::yield();

                break;
            }
        }
    }
}

int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Measure I/O subsystem read latencies.", nullptr);

    cmd.add_param("operation",
                  nullptr,
                  "operation",
                  "{read,nop,yield}",
                  "read",
                  {"what operation to do:",
                   " read - reads the file from the disk (the default)",
                   " nop - calls io_uring NOP function",
                   " yield - do nothing; just yield to another thread"
                  });

    cmd.add_param("seed",
                  nullptr,
                  "seed",
                  "num",
                  "0",
                  {"pseudo random seed to use (default is 0)"});

    cmd.add_param("storage-queue-depth",
                  nullptr,
                  "storage-queue-depth",
                  "num",
                  "32",
                  {"storage queue depth to use (default is 32)"});

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

    cmd.add_param("file",
                  "<file>",
                  {"file to read from; can be a block device"});

    cmd.parse(argc, argv);

    auto op_str = cmd.get<std::string_view>("operation");
    operation op;

    if (op_str == "read")
    {
        op = operation::READ;
    }
    else if (op_str == "nop")
    {
        op = operation::NOP;
    }
    else if (op_str == "yield")
    {
        op = operation::YIELD;
    }
    else
    {
        throw cmd_line::error("{}: invalid argument value",
                              cmd.get<std::string_view>("operation"));
    }

    set_cpu(cmd.get<uint32_t>("cpu"));

    gt::initialize();
    io::initialize(4096);
    io::file::initialize(cmd.get<uint32_t>("storage-queue-depth"));

    auto f = io::file::open(io::file::access::read, cmd.get<std::string_view>("file"));
    auto stat = f.stat();

    uint32_t pages = 0;

    if (S_ISBLK(stat.st_mode) == true)
    {
        uint64_t size;

        if (ioctl(f.fd(), BLKGETSIZE64, &size) == -1)
        {
            throw runtime_error("{}: {}", f.path(), system_error().message);
        }

        pages = size >> 12;
    }
    else
    {
        pages = stat.st_size >> 12;
    }

    assert(pages != 0);

    std::vector<tests::stats> s;

    for (uint32_t i = 0; i < cmd.get<uint32_t>("threads"); i++)
    {
        s.push_back(tests::stats());
    }

    for (uint32_t i = 0; i < s.size(); i++)
    {
        gt::create_thread(read_test,
                          op,
                          cmd.get<uint32_t>("seed") + i,
                          &f,
                          pages,
                          cmd.get<uint32_t>("iterations"),
                          &s[i]);
    }

    auto t1 = clock::now();

    gt::run();

    auto t2 = clock::now();

    logger::notice("pages: {}", pages);
    logger::notice("");

    for (uint32_t i = 0; i < s.size(); i++)
    {
        logger::notice("thread {} read latency [ns]:", i);
        logger::notice("");
        s[i].report();
        logger::notice("");
    }

    float iops = cmd.get<uint32_t>("iterations") *
            cmd.get<uint32_t>("threads") /
            ((t2 - t1) / 1000000000.);

    logger::notice("avg iops: {:.2f}", iops);
    logger::notice("avg read: {:.2f} MiB/s", iops / 256);

    return 0;
}
