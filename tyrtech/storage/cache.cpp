#include <storage/common.h>
#include <storage/cache.h>


namespace tyrtech::storage {


uint32_t cache::allocate()
{
    if (m_pages.full() == true)
    {
        auto old_page = m_cache.evict();

        if (old_page != invalid_handle)
        {
            free(old_page);
        }
    }

    return m_pages.allocate();
}

void cache::free(uint32_t page)
{
    m_pages.free(page);
}

void cache::add(uint64_t cache_key, uint32_t page)
{
    auto old_page = m_cache.set(cache_key, page);

    if (old_page != invalid_handle)
    {
        free(old_page);
    }
}

uint32_t cache::get(uint64_t cache_key)
{
    return m_cache.get(cache_key);
}

char* cache::get_memory(uint32_t page)
{
    uint16_t buffer_ndx = m_pages.slab_index(page);
    uint16_t page_ndx = m_pages.entry_index(page);

    return m_buffers[buffer_ndx]->data() + (static_cast<uint32_t>(page_ndx) << page_bits);
}

cache::cache(uint32_t cache_bits)
  : m_cache(1U << cache_bits)
{
    m_cache.set_empty_item(invalid_handle);

    uint64_t total_size = 1UL << (cache_bits + page_bits);

    while (total_size != 0)
    {
        uint64_t part_size = std::min(total_size, 1UL << 27);
        total_size -= part_size;

        m_buffers.push_back(std::make_unique<aligned_buffer>(buffer_alignment, part_size));
        m_pages.extend(std::make_unique<pages_t::slab>(part_size >> page_bits));
    }
}

}
