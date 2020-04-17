#include <storage/disk_writer.h>

#include <limits.h>



namespace tyrtech::storage {


disk_writer::state* disk_writer::allocate()
{
    m_states.push_back();
    state* state = m_states.back_item();

    state->buffer = nullptr;
    state->buffer_offset = 0;
    state->cached_pages.clear();
    state->descriptor.size = 0;
    state->descriptor.cache_id = new_cache_id();
    state->descriptor.extents.clear();
    state->mem_page = invalid_handle;
    state->state_handle = m_states.back();

    return state;
}

void disk_writer::free(state* state)
{
    m_states.erase(state->state_handle);
}

void disk_writer::allocate_page(state* state)
{
    assert(likely(state->mem_page == invalid_handle));

    while (m_dirty_pages >= m_max_dirty_pages)
    {
        start_global_flush();
        m_dirty_pages_cond.wait();
    }

    m_dirty_pages++;

    state->mem_page = m_cache->allocate();
    state->buffer = m_cache->get_memory(state->mem_page);
}

void disk_writer::write_page(state* state)
{
    if (state->buffer_offset == 0)
    {
        assert(likely(state->mem_page == invalid_handle));
        return;
    }

    state->cached_pages.push_back(state->mem_page);

    state->mem_page = invalid_handle;
    state->buffer = nullptr;
    state->buffer_offset = 0;

    gt::yield();
}

void disk_writer::flush_pages(state* state)
{
    flush_pages(state, true);
}

void disk_writer::remove(state* state)
{
    if (state->mem_page != invalid_handle)
    {
        m_cache->free(state->mem_page);
        m_dirty_pages--;
    }

    m_disk->remove(state->descriptor.extents);

    for (auto&& mem_page : state->cached_pages)
    {
        m_cache->free(mem_page);
    }

    assert(likely(m_dirty_pages >= state->cached_pages.size()));

    m_dirty_pages -= state->cached_pages.size();
    m_dirty_pages_cond.signal_all();
}

disk_writer::disk_writer(disk* disk, cache* cache, uint32_t write_cache_bits)
  : m_disk(disk)
  , m_cache(cache)
  , m_max_dirty_pages(1U << write_cache_bits)
{
}

uint64_t disk_writer::new_cache_id()
{
    return m_disk->new_cache_id();
}

void disk_writer::flush_pages(state* state, bool wait)
{
    {
        auto it = m_latch.find(state->descriptor.cache_id);

        if (it != m_latch.end())
        {
            if (wait == true)
            {
                it->second.wait();
            }
            else
            {
                return;
            }
        }
    }

    if (state->cached_pages.size() == 0)
    {
        return;
    }

    auto& cond = m_latch[state->descriptor.cache_id];

    state::cached_pages_t cached_pages;
    std::swap(state->cached_pages, cached_pages);

    uint32_t file_page = 0;

    for (auto&& extent : state->descriptor.extents)
    {
        file_page += extent & 0xffffffffU;
    }

    uint32_t pages = cached_pages.size();
    uint32_t disk_page = m_disk->allocate(pages);

    state->descriptor.extents.push_back((static_cast<uint64_t>(disk_page) << 32) | pages);

    auto it = cached_pages.begin();

    while (pages != 0)
    {
        uint32_t size = std::min(static_cast<uint32_t>(IOV_MAX), pages);
        iovec iov[IOV_MAX];

        for (uint32_t i = 0; i < size; i++)
        {
            assert(likely(it != cached_pages.end()));

            iov[i].iov_base = m_cache->get_memory(*it);
            iov[i].iov_len = page_size;

            ++it;
        }

        auto res = m_disk->write(disk_page, iov, size);

        if (res != (size << page_bits))
        {
            throw disk::error("{}: unable to write", m_disk->path());
        }

        disk_page += size;
        pages -= size;
    }

    for (auto&& mem_page : cached_pages)
    {
        assert(likely(file_page < (1U << file_pages_bits)));
        uint64_t cache_key = (state->descriptor.cache_id << file_pages_bits) | file_page++;

        m_cache->add(cache_key, mem_page);
    }

    assert(likely(m_dirty_pages >= cached_pages.size()));

    m_dirty_pages -= cached_pages.size();
    m_dirty_pages_cond.signal_all();

    cond.signal_all();
    m_latch.erase(state->descriptor.cache_id);
}

void disk_writer::start_global_flush()
{
    if (m_global_flush_active == true)
    {
        return;
    }

    m_global_flush_active = true;

    gt::create_thread(&disk_writer::flush_thread, this);
}

void disk_writer::flush_thread()
{
    uint32_t e = m_states.begin();

    while (e != invalid_handle)
    {
        flush_pages(m_states.item(e), false);
        e = m_states.next(e);
    }

    m_global_flush_active = false;
}

}
