#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <common/clock.h>
#include <gt/engine.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_server.h>

#include <tests/ping_service.json.h>


using namespace tyrtech;


namespace module {


using namespace tests::ping;


struct impl : private disallow_copy
{
    struct context : private disallow_copy
    {
    };

    uint64_t requests{0};

    context create_context(const std::shared_ptr<io::channel>& remote)
    {
        return context();
    }

    void ping(const ping::request_parser_t& request,
              ping::response_builder_t* response,
              context* ctx)
    {
        response->add_sequence(request.sequence());
        requests++;
    }

    impl()
    {
        gt::create_thread(&impl::print_requests, this);
    }

    void print_requests()
    {
        uint64_t last_requests = requests;
        uint64_t last_timestamp = clock::now();

        while (true)
        {
            gt::sleep(1000);

            uint64_t cur_requests = requests;
            uint64_t cur_timestamp = clock::now();

            logger::notice("requests: {:.2f} req/s",
                           1000000000. * (cur_requests - last_requests) / (cur_timestamp - last_timestamp));

            last_requests = cur_requests;
            last_timestamp = cur_timestamp;
        }
    }
};

}

using ping_service_t =
        tests::ping_service<module::impl>;

using server_t =
        net::rpc_server<8192, ping_service_t>;


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Ping server.", nullptr);

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

    cmd.add_param("uri",
                  "<uri>",
                  {"uri to listen on"});

    cmd.parse(argc, argv);

    set_cpu(cmd.get<uint32_t>("cpu"));

    gt::initialize();
    io::initialize(4096);
    io::channel::initialize(cmd.get<uint32_t>("network-queue-depth"));

    module::impl p;
    ping_service_t srv(&p);

    server_t s(io::uri::listen(cmd.get<std::string_view>("uri")), &srv);

    gt::run();

    return 0;
}
