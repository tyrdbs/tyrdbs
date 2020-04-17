#include <common/branch_prediction.h>
#include <tyrdbs/location.h>

#include <cassert>


namespace tyrtech::tyrdbs::location {


uint64_t offset_from(uint64_t location)
{
    return location >> 16;
}

uint16_t size_from(uint64_t location)
{
    return location & 0x7fffU;
}

bool is_leaf_from(uint64_t location)
{
    return (location & 0x8000U) != 0;
}

bool is_valid(uint64_t location)
{
    return (location & 0xffffU) != invalid_size;
}

uint16_t size(uint16_t size, uint16_t is_leaf)
{
    return (is_leaf << 15) | size;
}

uint64_t location(uint64_t offset, uint16_t size, bool is_leaf)
{
    assert(likely(offset < max_offset));
    assert(likely(size < max_size));

    return (offset << 16) | location::size(size, is_leaf);
}

uint64_t location(uint64_t offset, uint16_t size)
{
    assert(likely(offset < max_offset));

    return (offset << 16) | size;
}

}
