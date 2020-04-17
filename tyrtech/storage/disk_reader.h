#pragma once


#include <storage/disk.h>
#include <storage/cache.h>
#include <storage/latch.h>


namespace tyrtech::storage {


class disk_reader : private disallow_copy
{
public:
    const char* read(uint64_t cache_id, uint32_t file_page, const extents_t& extents);

    void remove(const extents_t& extents);

public:
    disk_reader(disk* disk, cache* cache);

private:
    using latch_t =
            latch<uint64_t, uint32_t>;

private:
    disk* m_disk{nullptr};
    cache* m_cache{nullptr};

    latch_t m_latch;

private:
    uint32_t get_disk_page(uint32_t file_page, const extents_t& extents);
};

}
