#include <common/cmd_line.h>
#include <common/cpu_sched.h>
#include <common/ring_queue.h>
#include <gt/engine.h>
#include <gt/async.h>
#include <io/engine.h>
#include <io/uri.h>
#include <net/rpc_server.h>
#include <tyrdbs/ushard.h>
#include <tyrdbs/cache.h>

#include <tests/db_server_service.json.h>

#include <tests/data.json.h>
#include <tests/snapshot.json.h>

#include <crc32c.h>


using namespace tyrtech;


namespace module {


using namespace tests::collections;


struct impl : private disallow_copy
{
    struct context : private disallow_copy
    {
        struct impl* impl;

        tyrdbs::ushard::slices_t snapshot;

        context(struct impl* impl)
          : impl(impl)
        {
        }

        ~context()
        {
            if (impl != nullptr)
            {
                impl->print_stats();
            }
        }

        context(context&& other) noexcept
        {
            *this = std::move(other);
        }

        context& operator=(context&& other) noexcept
        {
            impl = other.impl;
            other.impl = nullptr;

            return *this;
        }
    };

    context create_context(const std::shared_ptr<io::channel>& remote)
    {
        return context(this);
    }

    using merge_request_t =
            std::pair<uint32_t, uint32_t>;

    struct cb : public tyrdbs::ushard::meta_callback
    {
        uint32_t ushard;
        struct impl* impl{nullptr};

        cb(uint32_t ushard, struct impl* impl)
          : ushard(ushard)
          , impl(impl)
        {
        }

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

        void merge(uint16_t tier) override
        {
            impl->merge_requests.push(merge_request_t(ushard, tier));
            impl->merge_cond.signal();
        }
    };

    void merge_thread()
    {
        while (true)
        {
            if (merge_requests.empty() == false)
            {
                auto req = merge_requests.pop();

                if (tier_locks[req.first]->find(req.second) != tier_locks[req.first]->end())
                {
                    continue;
                }

                tier_locks[req.first]->insert(req.second);

                auto&& ushard = ushards[req.first];
                cb cb(req.first, this);

                ushard->merge(req.second, &cb);

                tier_locks[req.first]->erase(req.second);

                gt::yield();
            }
            else
            {
                if (gt::terminated() == true)
                {
                    break;
                }
                else
                {
                    merge_cond.wait();
                }
            }
        }
    }

    template<typename T>
    uint64_t id(const T& obj)
    {
        return reinterpret_cast<uint64_t>(obj.get());
    }

    void update_data(const update_data::request_parser_t& request,
                     update_data::response_builder_t* response,
                     context* ctx)
    {
        if (request.has_handle() == false)
        {
            while (tyrdbs::slice::count() > max_slices)
            {
                gt::yield();
            }

            writer w;
            w.idx = idx++;

            update_entries(request.get_parser(), request.data(), &w);

            writers[idx] = std::move(w);
            response->add_handle(idx);
        }
        else
        {
            auto& w = writers[request.handle()];
            update_entries(request.get_parser(), request.data(), &w);
        }
    }

    void commit_update(const commit_update::request_parser_t& request,
                       commit_update::response_builder_t* response,
                       context* ctx)
    {
        auto& w = writers[request.handle()];

        for (auto&& slice : w.slices)
        {
            cb cb(slice.first, this);

            ushards[slice.first]->add(slice.second->commit(), &cb);
        }

        writers.erase(request.handle());
    }

    void rollback_update(const rollback_update::request_parser_t& request,
                         rollback_update::response_builder_t* response,
                         context* ctx)
    {
        writers.erase(request.handle());
    }

    void fetch_data(const fetch_data::request_parser_t& request,
                    fetch_data::response_builder_t* response,
                    context* ctx)
    {
        if (request.has_handle() == false)
        {
            auto&& ushard = ushards[request.ushard() % ushards.size()];

            auto&& it = ushard->range(request.min_key(), request.max_key());
            uint64_t handle = id(it);

            if (it->next() == true)
            {
                reader r;
                r.iterator = std::move(it);
                r.value_part = r.iterator->value();

                if (fetch_entries(&r, response->add_data()) == false)
                {
                    readers[handle] = std::move(r);
                    response->add_handle(handle);
                }
            }
        }
        else
        {
            auto& r = readers[request.handle()];
            uint64_t handle = id(r.iterator);

            if (fetch_entries(&r, response->add_data()) == true)
            {
                readers.erase(handle);
            }
            else
            {
                response->add_handle(handle);
            }
        }
    }

    void abort_fetch(const abort_fetch::request_parser_t& request,
                     abort_fetch::response_builder_t* response,
                     context* ctx)
    {
        readers.erase(request.handle());
    }

    void snapshot(const snapshot::request_parser_t& request,
                  snapshot::response_builder_t* response,
                  context* ctx)
    {
        auto& ushard = ushards[request.ushard()];
        ctx->snapshot = ushard->get_slices();

        auto&& snapshot = tests::snapshot_builder(response->add_snapshot());
        snapshot.add_path(storage::path());

        auto&& slices = snapshot.add_slices();

        for (auto&& c : ctx->snapshot)
        {
            auto&& slice = slices.add_value();
            auto&& extents = slice.add_extents();

            for (auto&& e : c->extents())
            {
                extents.add_value(e);
            }
        }
    }

    void print_stats()
    {
        logger::notice("capacity:    {}", storage::capacity());
        logger::notice("used blocks: {}", storage::size());
        logger::notice("free blocks: {}", storage::capacity() - storage::size());
    }

    impl(uint32_t merge_threads,
         uint32_t ushards_num,
         uint32_t max_slices)
      : max_slices(max_slices)
    {
        for (uint32_t i = 0; i < ushards_num; i++)
        {
            ushards[i] = std::make_shared<tyrdbs::ushard>();
            tier_locks[i] = std::make_shared<tier_locks_t>();
        }

        for (uint32_t i = 0; i < merge_threads; i++)
        {
            gt::create_thread(&impl::merge_thread, this);
        }
    }

private:
    using slice_ptr =
            std::shared_ptr<tyrdbs::slice_writer>;

    using slices_t =
            std::unordered_map<uint32_t, slice_ptr>;

    struct writer
    {
        slices_t slices;
        uint64_t idx{0};
    };

    struct reader
    {
        std::unique_ptr<tyrdbs::iterator> iterator;
        std::string_view value_part;
    };

    using writers_t =
            std::unordered_map<uint64_t, writer>;

    using readers_t =
            std::unordered_map<uint64_t, reader>;

    uint32_t max_slices{0};

    uint64_t idx{0};

    writers_t writers;
    readers_t readers;

    ring_queue<merge_request_t> merge_requests;
    gt::condition merge_cond;

    using tier_locks_t =
            std::unordered_set<uint32_t>;

    using tier_locks_ptr =
            std::shared_ptr<tier_locks_t>;

    using tier_locks_map =
            std::unordered_map<uint32_t, tier_locks_ptr>;

    tier_locks_map tier_locks;

    using ushard_ptr =
            std::shared_ptr<tyrdbs::ushard>;

    using ushards_t =
            std::unordered_map<uint32_t, ushard_ptr>;

    ushards_t ushards;

private:
    bool fetch_entries(reader* r, message::builder* builder)
    {
        uint8_t data_flags = 0;

        auto data = tests::data_builder(builder);

        auto&& dbs = data.add_collections();
        auto&& db = dbs.add_value();
        auto&& entries = db.add_entries();

        while (true)
        {
            auto&& key = r->iterator->key();

            uint8_t entry_flags = 0;

            if (r->iterator->eor() == true)
            {
                entry_flags |= 0x01;
            }

            if (r->iterator->deleted() == true)
            {
                entry_flags |= 0x02;
            }

            uint16_t bytes_required = 0;

            bytes_required += tests::entry_builder::bytes_required();
            bytes_required += tests::entry_builder::key_bytes_required();
            bytes_required += tests::entry_builder::value_bytes_required();
            bytes_required += key.size();

            bytes_required += fetch_data::response_builder_t::handle_bytes_required();

            if (builder->available_space() <= bytes_required)
            {
                break;
            }

            assert(r->value_part.size() < 65536);

            uint16_t part_size = builder->available_space();
            part_size -= bytes_required;

            part_size = std::min(part_size, static_cast<uint16_t>(r->value_part.size()));

            if (part_size != r->value_part.size())
            {
                entry_flags &= ~0x01;
            }

            auto&& entry = entries.add_value();

            entry.set_flags(entry_flags);
            entry.add_key(key);
            entry.add_value(r->value_part.substr(0, part_size));

            if (part_size != r->value_part.size())
            {
                r->value_part = r->value_part.substr(part_size);
                break;
            }

            if (r->iterator->next() == false)
            {
                data_flags |= 0x01;
                break;
            }

            r->value_part = r->iterator->value();
        }

        data.set_flags(data_flags);

        return (data_flags & 0x01) == 0x01;
    }

    void update_entries(const message::parser* p, uint16_t off, writer* w)
    {
        tests::data_parser data(p, off);

        auto&& dbs = data.collections();

        assert(dbs.next() == true);
        auto&& db = dbs.value();

        auto&& entries = db.entries();

        while (entries.next() == true)
        {
            auto&& entry = entries.value();

            bool eor = entry.flags() & 0x01;
            bool deleted = entry.flags() & 0x02;

            auto&& slice = w->slices[entry.ushard() % ushards.size()];

            if (slice == nullptr)
            {
                slice = std::make_unique<tyrdbs::slice_writer>();
            }

            slice->add(entry.key(), entry.value(), eor, deleted, w->idx);
        }

        if ((data.flags() & 0x01) == 0x01)
        {
            auto jobs = gt::async::create_jobs();

            for (auto&& slice : w->slices)
            {
                auto f = [slice]
                {
                    slice.second->flush();
                };

                jobs.run(std::move(f));
            }

            jobs.wait();
        }
    }
};

}


using db_server_service_t =
        tests::db_server_service<module::impl>;

using server_t =
        net::rpc_server<8192, db_server_service_t>;


int main(int argc, const char* argv[])
{
    cmd_line cmd(argv[0], "Network server demo.", nullptr);

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

    cmd.add_param("merge-threads",
                  nullptr,
                  "merge-threads",
                  "num",
                  "2",
                  {"number of merge threads to use (default is 2)"});

    cmd.add_param("ushards",
                  nullptr,
                  "ushards",
                  "num",
                  "16",
                  {"number of ushards to use (default is 16)"});

    cmd.add_param("max-slices",
                  nullptr,
                  "max-slices",
                  "num",
                  "4096",
                  {"maximum number of slices allowed (default is 4096)"});

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

    cmd.add_param("storage-file",
                  nullptr,
                  "storage-file",
                  "file",
                  "storage.dat",
                  {"storage file to use (default storage.dat)"});

    cmd.add_param("uri",
                  "<uri>",
                  {"uri to listen on"});

    cmd.parse(argc, argv);

    set_cpu(cmd.get<uint32_t>("cpu"));

    assert(crc32c_initialize() == true);

    gt::initialize();
    gt::async::initialize();
    io::initialize(4096);
    io::file::initialize(cmd.get<uint32_t>("storage-queue-depth"));
    io::channel::initialize(cmd.get<uint32_t>("network-queue-depth"));

    tyrdbs::cache::initialize(cmd.get<uint32_t>("block-cache-bits"));

    storage::initialize(io::file::create(cmd.get<std::string_view>("storage-file")),
                        cmd.get<uint32_t>("cache-bits"),
                        cmd.get<uint32_t>("write-cache-bits"),
                        cmd.flag("preallocate-space"));

    module::impl impl(cmd.get<uint32_t>("merge-threads"),
                      cmd.get<uint32_t>("ushards"),
                      cmd.get<uint32_t>("max-slices"));

    db_server_service_t srv(&impl);

    server_t s(io::uri::listen(cmd.get<std::string_view>("uri")), &srv);

    gt::run();

    return 0;
}
