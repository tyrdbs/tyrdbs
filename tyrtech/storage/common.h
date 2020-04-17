#pragma once


#include <vector>
#include <cstdint>


namespace tyrtech::storage {


static constexpr uint32_t page_bits{12};
static constexpr uint32_t page_size{1 << page_bits};
static constexpr uint32_t page_mask{page_size - 1};

static constexpr uint32_t file_pages_bits{25};

static constexpr uint32_t invalid_handle{static_cast<uint32_t>(-1)};


using extents_t =
        std::vector<uint64_t>;


struct file_descriptor
{
    uint64_t cache_id{0};
    uint64_t size{0};

    extents_t extents;
};


}
