#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <gt/engine.h>
#include <gt/async.h>
#include <io/engine.h>
#include <tyrdbs/ushard.h>
#include <tyrdbs/cache.h>

#include <tests/stats.h>

#include <crc32c.h>


using namespace tyrtech;


struct test_cb : public tyrdbs::ushard::meta_callback
{
    void add(const tyrtech::tyrdbs::ushard::slice_ptr& slice) override
    {
    }

    void remove(const tyrtech::tyrdbs::ushard::slices_t& slices) override
    {
        for (auto&& slice : slices)
        {
            slice->unlink();
        }
    }

    std::vector<uint32_t> merge_requests;
    gt::condition merge_cond;

    std::shared_ptr<tyrdbs::ushard> ushard;

    bool compact{false};
    bool exit_merge{false};

    bool merge_done{false};
    gt::condition done_cond;

    void merge(uint16_t tier) override
    {
        merge_requests.push_back(tier);
        merge_cond.signal();
    }
};


struct thread_data
{
    uint32_t thread_id{0};

    uint64_t duration{0};

    tests::stats vs_stats;
    tests::stats vr_stats;

    thread_data(uint32_t thread_id)
      : thread_id(thread_id)
    {
    }
};


std::string string_storage;


using string_t =
        std::pair<uint64_t, uint16_t>;

using data_point_t =
        std::pair<string_t, uint64_t>;

using data_set_t =
        std::vector<data_point_t>;

using data_sets_t =
        std::vector<data_set_t>;


void insert(const data_set_t& data, test_cb* cb)
{
    tyrdbs::slice_writer w;

    auto t1 = clock::now();

    for (auto&& it : data)
    {
        std::string_view key(string_storage.data() + it.first.first, it.first.second);
        w.add(key, key, true, false, it.second);
    }

    w.flush();

    cb->ushard->add(w.commit(), cb);

    auto t2 = clock::now();

    uint64_t duration = t2 - t1;

    logger::notice("added {} keys in {:.6f} s, {:.2f} keys/s",
                   data.size(),
                   duration / 1000000000.,
                   data.size() * 1000000000. / duration);
}

void verify_sequential(const data_set_t& data, tyrdbs::ushard* ushard, tests::stats* s)
{
    auto&& db_it = ushard->begin();
    auto&& data_it = data.begin();

    std::string value;

    uint32_t output_size = 0;

    while (data_it != data.end())
    {
        std::string_view key(string_storage.data() + data_it->first.first,
                             data_it->first.second);

        {
            auto sw = s->stopwatch();
            assert(db_it->next() == true);
        }

        while (true)
        {
            assert(db_it->key().compare(key) == 0);
            assert(db_it->deleted() == false);
            assert(db_it->idx() == data_it->second);

            auto&& value_part = db_it->value();
            value.append(value_part.data(), value_part.size());

            if (db_it->eor() == true)
            {
                break;
            }

            assert(db_it->next() == true);
        }

        assert(key.compare(value) == 0);

        value.clear();

        ++data_it;

        output_size += key.size() + value.size();

        if (output_size > 8192)
        {
            output_size = 0;
            gt::yield();
        }
    }

    assert(db_it->next() == false);
}

void verify_range(const data_set_t& data, tyrdbs::ushard* ushard, tests::stats* s)
{
    auto&& data_it = data.begin();

    std::string value;

    while (data_it != data.end())
    {
        std::string_view key(string_storage.data() + data_it->first.first,
                             data_it->first.second);

        std::unique_ptr<tyrdbs::iterator> db_it;

        {
            auto sw = s->stopwatch();
            db_it = ushard->range(key, key);
        }

        assert(db_it->next() == true);

        while (true)
        {
            assert(db_it->key().compare(key) == 0);
            assert(db_it->deleted() == false);
            assert(db_it->idx() == data_it->second);

            auto&& value_part = db_it->value();
            value.append(value_part.data(), value_part.size());

            if (db_it->eor() == true)
            {
                break;
            }

            assert(db_it->next() == true);
        }

        assert(key.compare(value) == 0);

        value.clear();

        ++data_it;

        gt::yield();
    }
}

void merge_thread(test_cb* cb)
{
    while (true)
    {
        if (cb->merge_requests.size() != 0)
        {
            uint32_t tier = cb->merge_requests[0];
            cb->merge_requests.erase(cb->merge_requests.begin());

            auto t1 = clock::now();
            uint64_t merged_keys = cb->ushard->merge(tier, cb);
            auto t2 = clock::now();

            if (merged_keys != 0)
            {
                uint64_t duration = t2 - t1;

                logger::notice("merged {} keys in {:.6f} s, {:.2f} keys/s",
                               merged_keys,
                               duration / 1000000000.,
                               merged_keys * 1000000000. / duration);
            }

            gt::yield();
        }
        else
        {
            if (cb->exit_merge == true)
            {
                break;
            }
            else
            {
                cb->merge_cond.wait();
            }
        }
    }

    if (cb->compact == true)
    {
        auto t1 = clock::now();
        uint64_t compacted_keys = cb->ushard->compact(cb);
        auto t2 = clock::now();

        if (compacted_keys != 0)
        {
            uint64_t duration = t2 - t1;

            logger::notice("compacted {} keys in {:.6f} s, {:.2f} keys/s",
                           compacted_keys,
                           duration / 1000000000.,
                           compacted_keys * 1000000000. / duration);
        }
    }

    cb->merge_done = true;
    cb->done_cond.signal();
}


void test(const data_sets_t* data,
          const data_set_t* test_data,
          thread_data* t,
          bool compact)
{
    auto t1 = clock::now();

    test_cb cb;

    cb.ushard = std::make_shared<tyrdbs::ushard>();

    gt::create_thread(merge_thread, &cb);

    for (auto&& set : *data)
    {
        insert(set, &cb);
    }

    cb.compact = compact;

    cb.exit_merge = true;
    cb.merge_cond.signal();

    while (cb.merge_done == false)
    {
        cb.done_cond.wait();
    }

    verify_sequential(*test_data, cb.ushard.get(), &t->vs_stats);
    verify_range(*test_data, cb.ushard.get(), &t->vr_stats);

    auto t2 = clock::now();

    t->duration = t2 - t1;

    //cb.ushard->drop();
}

data_set_t load_data(FILE* fp)
{
    data_set_t data;

    uint64_t idx;
    char key[4096];

    while (fscanf(fp, "%s %lu", key, &idx) == 2)
    {
        bool batch_done = true;

        batch_done &= idx == 0;
        batch_done &= key[0] == '=';
        batch_done &= key[1] == '=';
        batch_done &= key[2] == '\0';

        if (batch_done == true)
        {
            break;
        }

        uint64_t offset = string_storage.size();
        uint16_t size = strlen(key);

        string_storage.append(key, size);

        data.push_back(data_point_t(string_t(offset, size), idx));
    }

    return data;
}

data_sets_t load_data(const std::string_view& path)
{
    data_sets_t data;

    FILE* fp = fopen(path.data(), "r");
    assert(fp != nullptr);

    while (true)
    {
        data_set_t set = load_data(fp);

        if (set.size() == 0)
        {
            break;
        }

        data.push_back(std::move(set));
    }

    fclose(fp);

    return data;
}

data_set_t load_test_data(const std::string_view& path)
{
    FILE* fp = fopen(path.data(), "r");
    assert(fp != nullptr);

    data_set_t set = load_data(fp);

    fclose(fp);

    return set;
}

void report(thread_data* t)
{
    logger::notice("");
    logger::notice("thread #{} took \033[1m{:.6f} s\033[0m",
                   t->thread_id,
                   t->duration / 1000000000.);

    logger::notice("");
    logger::notice("sequential read [ns]:");
    t->vs_stats.report();

    logger::notice("");
    logger::notice("random read [ns]:");
    t->vr_stats.report();
}

int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Ushard layer test.", nullptr);

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
                  "4",
                  {"number of threads to use (default is 4)"});

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

    cmd.add_flag("preallocate-space",
                 nullptr,
                 "preallocate-space",
                 {"preallocate space on disk"});

    cmd.add_flag("compact",
                 nullptr,
                 "compact",
                 {"compact ushard before reading"});

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

    cmd.add_param("storage-file",
                  nullptr,
                  "storage-file",
                  "file",
                  "storage.dat",
                  {"storage file to use (default storage.dat)"});

    cmd.parse(argc, argv);

    set_cpu(cmd.get<uint32_t>("cpu"));

    auto&& data = load_data(cmd.get<std::string_view>("input-data"));
    auto&& test_data = load_test_data(cmd.get<std::string_view>("test-data"));

    assert(crc32c_initialize() == true);

    gt::initialize();
    gt::async::initialize();
    io::initialize(4096);
    io::file::initialize(cmd.get<uint32_t>("storage-queue-depth"));

    tyrdbs::cache::initialize(cmd.get<uint32_t>("block-cache-bits"));

    storage::initialize(io::file::create(cmd.get<std::string_view>("storage-file")),
                        cmd.get<uint32_t>("cache-bits"),
                        cmd.get<uint32_t>("write-cache-bits"),
                        cmd.flag("preallocate-space"));

    std::vector<thread_data> td;

    for (uint32_t i = 0; i < cmd.get<uint32_t>("threads"); i++)
    {
        td.push_back(thread_data(i));
    }

    for (uint32_t i = 0; i < td.size(); i++)
    {
        gt::create_thread(test, &data, &test_data, &td[i], cmd.flag("compact"));
    }

    auto t1 = clock::now();

    gt::run();

    auto t2 = clock::now();

    logger::notice("");
    logger::notice("execution took \033[1m{:.6f} s\033[0m",
                   (t2 - t1) / 1000000000.);

    for (auto&& t : td)
    {
        report(&t);
    }

    uint32_t capacity = storage::capacity();
    uint32_t size = storage::size();

    logger::notice("");
    logger::notice("capacity:    {}", capacity);
    logger::notice("used blocks: {}", size);
    logger::notice("free blocks: {}", capacity - size);

    return 0;
}
