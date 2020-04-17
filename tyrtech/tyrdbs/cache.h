#pragma once


#include <tyrdbs/node.h>
#include <storage/file_reader.h>


namespace tyrtech::tyrdbs::cache {


using node_ptr =
        std::shared_ptr<node>;


void initialize(uint32_t cache_bits);

node_ptr get(const storage::file_reader& reader, uint64_t chunk_ndx, uint64_t location);
void set(uint64_t chunk_ndx, uint64_t location, node_ptr node);

}
