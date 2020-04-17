#pragma once


#include <storage/file_reader.h>
#include <storage/file_writer.h>


namespace tyrtech::storage {


void initialize(io::file file,
                uint32_t cache_bits,
                uint32_t write_cache_bits,
                bool preallocate_space);

uint32_t capacity();
uint32_t size();
std::string_view path();

uint64_t new_cache_id();

file_reader create_reader(file_descriptor&& descriptor);
file_writer create_writer();

}
