#include <common/wtinylfu.h>
#include <storage/latch.h>
#include <tyrdbs/cache.h>
#include <tyrdbs/location.h>

#include <crc32c.h>


namespace tyrtech::tyrdbs::cache {


struct key
{
    uint64_t chunk_ndx{static_cast<uint64_t>(-1)};
    uint64_t location{static_cast<uint64_t>(-1)};

    bool operator==(const key& other) const
    {
        return chunk_ndx == other.chunk_ndx &&
                location == other.location;
    }

    bool operator!=(const key& other) const
    {
        return !(*this == other);
    }
} __attribute__ ((packed));

struct key_hasher
{
    size_t operator()(const key& key) const
    {
        size_t seed = 0;

        seed ^= std::hash<uint64_t>()(key.chunk_ndx) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<uint64_t>()(key.location) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};


using cache_t =
        wtinylfu<cache::key, node_ptr, cache::key_hasher>;

using latch_t =
        storage::latch<cache::key, node_ptr, cache::key_hasher>;


thread_local std::unique_ptr<cache_t> __cache;
thread_local std::unique_ptr<latch_t> __latch;

thread_local uint64_t __cache_requests{0};
thread_local uint64_t __cache_misses{0};


void initialize(uint32_t cache_bits)
{
    if (cache_bits != 0)
    {
        __cache = std::make_unique<cache_t>(1U << cache_bits);
        __latch = std::make_unique<latch_t>();
    }
}

void set(uint64_t chunk_ndx, uint64_t location, node_ptr node)
{
    if (__cache.get() == nullptr)
    {
        return;
    }

    cache::key key;

    key.chunk_ndx = chunk_ndx;
    key.location = location;

    __cache->set(key, std::move(node));
}

node_ptr load(const storage::file_reader& reader, uint64_t location)
{
    uint64_t node_offset = location::offset_from(location);
    uint16_t node_size = location::size_from(location);

    assert(likely(node_size <= node::node_size));

    using buffer_t =
            std::array<char, node::page_size>;

    buffer_t buffer;
    char* data = buffer.data();

    reader.pread(node_offset, data, node_size + 6);

    uint32_t crc32c = *reinterpret_cast<const uint32_t*>(data);
    data += sizeof(crc32c);

    if (crc32c_update(0, data, node_size) != crc32c)
    {
        throw runtime_error("invalid node crc32c");
    }

    auto node = std::make_shared<tyrdbs::node>();

    node->load(data, node_size);
    data += node_size;

    uint64_t offset = location::offset_from(location);

    offset += node_size;
    offset += 6;

    uint64_t next_node =
            location::location(offset, *reinterpret_cast<const uint16_t*>(data));

    node->set_next(next_node);

    return node;
}

node_ptr get(const storage::file_reader& reader, uint64_t chunk_ndx, uint64_t location)
{
    if (__cache.get() == nullptr)
    {
        return load(reader, location);
    }

    cache::key cache_key;

    cache_key.chunk_ndx = chunk_ndx;
    cache_key.location = location;

    __cache_requests++;

    auto node = __cache->get(cache_key);

    if (node != nullptr)
    {
        return node;
    }

    node = __latch->acquire(cache_key);

    if (node == nullptr)
    {
        __cache_misses++;

        node = load(reader, location);

        __latch->release(cache_key);
    }
    else
    {
        __latch->wait_for(cache_key);
    }

    if (__latch->remove(cache_key) == true)
    {
        __cache->set(cache_key, node);
    }

    return node;
}

}
