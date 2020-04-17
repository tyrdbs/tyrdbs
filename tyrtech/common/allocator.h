#pragma once


#include <common/disallow_copy.h>

#include <set>
#include <cstdint>


namespace tyrtech {


class allocator : private disallow_copy
{
public:
    uint32_t allocate(uint32_t size);
    void free(uint32_t idx, uint32_t size);

    void extend(uint32_t size);

    uint32_t capacity() const;
    uint32_t size() const;
    bool full() const;

private:
    using set_t =
            std::set<uint64_t>;

private:
    set_t m_free_set;
    set_t m_idx_set;

    uint32_t m_max_idx{0};
    uint32_t m_size{0};
};

}
