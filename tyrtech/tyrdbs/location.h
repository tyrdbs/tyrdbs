#pragma once


#include <cstdint>


namespace tyrtech::tyrdbs::location {


static constexpr uint64_t max_offset{0x1000000000000UL};
static constexpr uint16_t max_size{0x7fffU};
static constexpr uint16_t invalid_size{0xffffU};


uint64_t offset_from(uint64_t location);
uint16_t size_from(uint64_t location);

bool is_leaf_from(uint64_t location);
bool is_valid(uint64_t location);

uint16_t size(uint16_t size, uint16_t is_leaf);

uint64_t location(uint64_t offset, uint16_t size, bool is_leaf);
uint64_t location(uint64_t offset, uint16_t size);

}
