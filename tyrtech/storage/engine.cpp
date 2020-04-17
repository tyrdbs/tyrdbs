#include <common/disallow_copy.h>
#include <common/disallow_move.h>
#include <storage/engine.h>


namespace tyrtech::storage {


struct engine : private disallow_copy, disallow_move
{
    disk disk;
    cache cache;

    disk_reader disk_reader;
    disk_writer disk_writer;

    file_reader create_reader(file_descriptor&& descriptor);
    file_writer create_writer();
    uint32_t capacity() const;
    uint32_t size() const;
    std::string_view path() const;

    engine(io::file file,
           uint32_t cache_bits,
           uint32_t write_cache_bits,
           bool preallocate_space);
};

engine::engine(io::file file,
               uint32_t cache_bits,
               uint32_t write_cache_bits,
               bool preallocate_space)
  : disk(std::move(file), preallocate_space)
  , cache(cache_bits)
  , disk_reader(&disk, &cache)
  , disk_writer(&disk, &cache, write_cache_bits)
{
    assert(likely(cache_bits > 6));
    assert(likely(cache_bits > write_cache_bits));
}


thread_local std::unique_ptr<engine> __engine;


void initialize(io::file file,
                uint32_t cache_bits,
                uint32_t write_cache_bits,
                bool preallocate_space)
{
    __engine = std::make_unique<engine>(std::move(file),
                                        cache_bits,
                                        write_cache_bits,
                                        preallocate_space);
}

uint64_t new_cache_id()
{
    return __engine->disk.new_cache_id();
}

file_reader create_reader(file_descriptor&& descriptor)
{
    return file_reader(&__engine->disk_reader, std::move(descriptor));
}

file_writer create_writer()
{
    return file_writer(&__engine->disk_writer);
}

uint32_t capacity()
{
    return __engine->disk.capacity();
}

uint32_t size()
{
    return __engine->disk.size();
}

std::string_view path()
{
    return __engine->disk.path();
}

}
