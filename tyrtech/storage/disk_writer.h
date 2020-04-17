#pragma once


#include <common/slab_list.h>
#include <gt/condition.h>
#include <storage/disk.h>
#include <storage/cache.h>


namespace tyrtech::storage {


class disk_writer : public disallow_copy
{
public:
    DEFINE_EXCEPTION(disk::error, error);

public:
    class state
    {
    public:
        char* buffer{nullptr};
        uint32_t buffer_offset{0};

        file_descriptor descriptor;

    private:
        using cached_pages_t =
                std::vector<uint32_t>;

        cached_pages_t cached_pages;

        uint32_t mem_page{invalid_handle};
        uint32_t state_handle{invalid_handle};

    private:
        friend class disk_writer;
    }; // TODO: __attribute__ ((packed))

public:
    state* allocate();
    void free(state* state);

    void allocate_page(state* state);
    void write_page(state* state);

    void flush_pages(state* state);

    void remove(state* state);

public:
    disk_writer(disk* disk, cache* cache, uint32_t write_cache_bits);

private:
    using latch_t =
            std::unordered_map<uint64_t, gt::condition>;

    using states_t =
            slab_list<state, 1024>;

private:
    disk* m_disk{nullptr};
    cache* m_cache{nullptr};

    uint32_t m_max_dirty_pages{0};
    uint32_t m_dirty_pages{0};

    gt::condition m_dirty_pages_cond;

    bool m_global_flush_active{false};

    states_t::entry_pool_t m_state_entries;
    states_t m_states{&m_state_entries};

    latch_t m_latch;

private:
    uint64_t new_cache_id();

    void flush_pages(state* state, bool wait);

    void start_global_flush();
    void flush_thread();
};

}
