#include <common/cpu_sched.h>
#include <gt/engine.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_server.h>
#include <net/rpc_client.h>

#include <tests/services.json.h>


using namespace tyrtech;


namespace module1 {


using namespace tests::module1;


struct impl : private disallow_copy
{
    struct context : private disallow_copy
    {
    };

    context create_context(const std::shared_ptr<io::channel>& remote)
    {
        return context();
    }

    void func1(const func1::request_parser_t& request,
               func1::response_builder_t* response,
               context* ctx)
    {
    using namespace tests::module1;

        response->add_param1(request.param1());
        response->add_param2(request.param2());

        logger::debug("module1::func1 request: {} {}", request.param1(), request.param2());
    }

    void func2(const func2::request_parser_t& request,
               func2::response_builder_t* response,
               context* ctx)
    {
        response->add_param1(request.param1());
        response->add_param2(request.param2());

        logger::debug("module1::func2 request: {} {}", request.param1(), request.param2());
    }
};

}

namespace module2 {


using namespace tests::module2;


struct impl : private disallow_copy
{
    struct context : private disallow_copy
    {
    };

    context create_context(const std::shared_ptr<io::channel>& remote)
    {
        return context();
    }

    void func1(const func1::request_parser_t& request,
               func1::response_builder_t* response,
               context* ctx)
    {
        response->add_param1(request.param1());
        response->add_param2(request.param2());

        logger::debug("module2::func1 request: {} {}", request.param1(), request.param2());
    }

    void func2(const func2::request_parser_t& request,
               func2::response_builder_t* response,
               context* ctx)
    {
        response->add_param1(request.param1());
        response->add_param2(request.param2());

        throw test3_error("this is a test");

        logger::debug("module2::func2 request: {} {}", request.param1(), request.param2());
    }
};

}

using service1_t =
        tests::service1<module1::impl, module2::impl>;

using server_t =
        net::rpc_server<8192, service1_t>;


void client(server_t* s)
{
    for (uint32_t i = 0; i < 1000; i++)
    {
        logger::debug("iteration: {}", i);

        net::rpc_client<8192> c(io::uri::connect(s->uri(), 0));

        {
            auto func1 = c.remote_call<tests::module1::func1>();

            auto req = func1.request();
            req.add_param1(1);
            req.add_param2("test1");
            req.finalize();

            func1.execute();
            func1.wait();

            auto res = func1.response();

            logger::debug("module1::func1 response: {} {}", res.param1(), res.param2());
        }

        {
            auto func2 = c.remote_call<tests::module2::func2>();

            auto req = func2.request();
            req.add_param1(2);
            req.add_param2("test2");
            req.finalize();

            func2.execute();
            func2.wait();

            auto res = func2.response();

            logger::debug("module2::func2 response: {} {}", res.param1(), res.param2());
        }
    }

    s->terminate();
}


int main()
{
    set_cpu(0);

    gt::initialize();
    io::initialize(4096);
    io::channel::initialize(64);

    logger::set(logger::level::debug);

    //std::string_view uri("tcp://localhost:1234");
    std::string_view uri("unix://@/test.sock");

    module1::impl m1;
    module2::impl m2;

    service1_t srv(&m1, &m2);

    server_t s(io::uri::listen(uri), &srv);

    gt::create_thread(&client, &s);

    gt::run();

    return 0;
}
