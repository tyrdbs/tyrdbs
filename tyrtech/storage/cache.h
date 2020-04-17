#pragma once


#include <common/aligned_buffer.h>
#include <common/slabs.h>
#include <common/wtinylfu.h>


namespace tyrtech::storage {


class cache : private disallow_copy
{
public:
    uint32_t allocate();
    void free(uint32_t page);

    void add(uint64_t cache_key, uint32_t page);
    uint32_t get(uint64_t cache_key);

    char* get_memory(uint32_t page);

public:
    cache(uint32_t cache_bits);

private:
    static constexpr uint32_t buffer_alignment{4096};

private:
    using buffers_t =
            std::vector<std::unique_ptr<aligned_buffer>>;

    using pages_t =
            slabs<void*>;

    using cache_t =
            wtinylfu<uint64_t, uint32_t>;

private:
    buffers_t m_buffers;
    pages_t m_pages;

    cache_t m_cache;
};

}
