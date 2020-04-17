#include <storage/disk_reader.h>


namespace tyrtech::storage {


const char* disk_reader::read(uint64_t cache_id, uint32_t file_page, const extents_t& extents)
{
    assert(likely(file_page < (1U << file_pages_bits)));
    uint64_t cache_key = (cache_id << file_pages_bits) | file_page;

    auto mem_page = m_cache->get(cache_key);

    if (mem_page != invalid_handle)
    {
        return m_cache->get_memory(mem_page);
    }

    mem_page = m_latch.acquire(cache_key);

    if (mem_page == invalid_handle)
    {
        mem_page = m_cache->allocate();
        m_latch.set(cache_key, mem_page);

        uint32_t disk_page = get_disk_page(file_page, extents);
        m_disk->read(disk_page, m_cache->get_memory(mem_page));

        m_latch.release(cache_key);
    }
    else
    {
        m_latch.wait_for(cache_key);
    }

    if (m_latch.remove(cache_key) == true)
    {
        m_cache->add(cache_key, mem_page);
    }

    return m_cache->get_memory(mem_page);
}

void disk_reader::remove(const extents_t& extents)
{
    m_disk->remove(extents);
}

disk_reader::disk_reader(disk* disk, cache* cache)
  : m_disk(disk)
  , m_cache(cache)
{
    m_latch.set_empty_value(invalid_handle);
}

uint32_t disk_reader::get_disk_page(uint32_t file_page, const extents_t& extents)
{
    uint32_t disk_page = invalid_handle;

    for (auto&& extent : extents)
    {
        uint32_t extent_page = extent >> 32;
        uint32_t extent_pages = extent & 0xffffffffU;

        if (extent_pages > file_page)
        {
            disk_page = extent_page + file_page;

            break;
        }

        file_page -= extent_pages;
    }

    return disk_page;
}

}
