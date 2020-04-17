#include <common/logger.h>
#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_client.h>
#include <tyrdbs/cache.h>

#include <crc32c.h>

#include <tests/db_server_service.json.h>
#include <tests/snapshot.json.h>


using namespace tyrtech;


void dump_thread(const std::string_view& uri,
                 const std::string_view& collection,
                 uint32_t ushard)
{
    net::rpc_client<8192> c(io::uri::connect(uri, 0));

    auto snapshot = c.remote_call<tests::collections::snapshot>();
    auto req = snapshot.request();

    req.add_collection(collection);
    req.add_ushard(ushard);

    snapshot.execute();
    snapshot.wait();

    auto&& res = snapshot.response();
    auto&& snap = tests::snapshot_parser(res.get_parser(), res.snapshot());

    logger::notice("storage-file: {}", snap.path());

    auto&& slices = snap.slices();

    while (slices.next() == true)
    {
        logger::notice("slice:");

        auto&& slice = slices.value();
        auto&& extents = slice.extents();

        while (extents.next() == true)
        {
            auto extent = extents.value();

            uint32_t page = extent >> 32;
            uint32_t pages = extent & 0xffffffffU;

            logger::notice(" page: {}, pages: {}", page, pages);
        }
    }
}


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Dump client demo.", nullptr);

    cmd.add_param("storage-queue-depth",
                  nullptr,
                  "storage-queue-depth",
                  "num",
                  "32",
                  {"storage queue depth to use (default is 32)"});

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

    cmd.add_param("cache-bits",
                  nullptr,
                  "cache-bits",
                  "bits",
                  "18",
                  {"cache size expressed as 2^bits (default is 18)"});

    cmd.add_param("write-cache-bits",
                  nullptr,
                  "write-cache-bits",
                  "bits",
                  "14",
                  {"write cache size expressed as 2^bits (default is 14)"});

    cmd.add_param("block-cache-bits",
                  nullptr,
                  "block-cache-bits",
                  "bits",
                  "12",
                  {"block cache size expressed as 2^bits (default is 12)"});

    cmd.add_param("uri",
                  "<uri>",
                  {"uri to connect to"});

    cmd.add_param("collection",
                  "<collection>",
                  {"collection to dump"});

    cmd.add_param("ushard",
                  "<ushard>",
                  {"ushard to dump"});

    cmd.parse(argc, argv);

    set_cpu(cmd.get<uint32_t>("cpu"));

    assert(crc32c_initialize() == true);

    gt::initialize();
    io::initialize(4096);
    io::file::initialize(cmd.get<uint32_t>("storage-queue-depth"));
    io::channel::initialize(cmd.get<uint32_t>("network-queue-depth"));

    tyrdbs::cache::initialize(cmd.get<uint32_t>("block-cache-bits"));

    gt::create_thread(dump_thread,
                      cmd.get<std::string_view>("uri"),
                      cmd.get<std::string_view>("collection"),
                      cmd.get<uint32_t>("ushard"));

    gt::run();

    return 0;
}
