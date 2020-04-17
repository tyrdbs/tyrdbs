#include <common/uuid.h>
#include <common/cpu_sched.h>
#include <gt/async.h>
#include <io/engine.h>
#include <tyrdbs/collection.h>
#include <tyrdbs/cache.h>

#include <crc32c.h>


using namespace tyrtech;


struct test_cb : public tyrdbs::ushard::meta_callback
{
    std::shared_ptr<tyrdbs::ushard> ushard;
    uint16_t ushard_id{0};

    void add(const tyrtech::tyrdbs::ushard::slice_ptr& slice) override
    {
    }

    void remove(const tyrtech::tyrdbs::ushard::slices_t& slices) override
    {
    }

    void merge(uint16_t tier) override
    {
    }
};


void test()
{
    storage::initialize(io::file::create("_test/{}", uuid()), 18, 14, false);

    auto c = std::make_shared<tyrdbs::collection>("test");

    test_cb cb;

    cb.ushard = c->get_ushard(1, true);
    cb.ushard_id = 0;

    tyrdbs::slice_writer w;

    w.flush();

    cb.ushard->add(w.commit(), &cb);
}


int main()
{
    set_cpu(0);

    assert(crc32c_initialize() == true);

    gt::initialize();
    gt::async::initialize();
    io::initialize(4096);
    io::file::initialize(32);

    tyrdbs::cache::initialize(12);

    gt::create_thread(test);
    gt::run();

    return 0;
}
