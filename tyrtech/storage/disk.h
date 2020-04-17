#pragma once


#include <common/allocator.h>
#include <gt/condition.h>
#include <io/file.h>
#include <storage/common.h>


namespace tyrtech::storage {


class disk : private disallow_copy
{
public:
    DEFINE_EXCEPTION(io::file::error, error);

public:
    template<typename... Arguments>
    static decltype(auto) create(Arguments&&... arguments)
    {
        return std::make_shared<disk>(io::file::create(std::forward<Arguments>(arguments)...));
    }

    template<typename... Arguments>
    static decltype(auto) open(Arguments&&... arguments)
    {
        return std::make_shared<disk>(io::file::open(std::forward<Arguments>(arguments)...));
    }

public:
    void read(uint32_t page, char* buff);
    uint32_t write(uint32_t page, iovec* iovec, uint32_t size);

    uint32_t allocate(uint32_t pages);
    void free(uint32_t page, uint32_t pages);

    void remove(const extents_t& extents);

    uint32_t size() const;
    uint32_t capacity() const;

    std::string_view path() const;

    uint64_t new_cache_id();

public:
    disk(io::file file, bool preallocate_space);

private:
    io::file m_file;

    gt::condition m_allocation_cond;
    bool m_allocation_in_progress{false};

    bool m_preallocate_space{false};

    allocator m_blocks;

    uint64_t m_next_cache_id{1};

private:
    void allocate_space();
};

}
