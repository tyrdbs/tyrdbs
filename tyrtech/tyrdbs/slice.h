#pragma once


#include <storage/engine.h>
#include <tyrdbs/node.h>
#include <tyrdbs/attributes.h>
#include <tyrdbs/iterator.h>


namespace tyrtech::tyrdbs {


class slice : private disallow_copy, disallow_move
{
public:
    struct stats
    {
        uint64_t key_count{0};
        uint64_t compressed_size{0};
        uint64_t uncompressed_size{0};
        uint64_t total_nodes{0};
        uint64_t leaf_nodes{0};
    } __attribute__ ((packed));

public:
    std::unique_ptr<iterator> range(const std::string_view& min_key, const std::string_view& max_key);
    std::unique_ptr<iterator> begin();

    void unlink();

    uint64_t key_count() const;
    const storage::extents_t& extents() const;

public:
    static uint64_t count();

public:
    slice() = default;
    slice(storage::file_reader&& reader);

    ~slice();

private:
    static constexpr uint64_t signature{0x3130306264727974UL};

public:
    struct header
    {
        uint64_t signature{slice::signature};
        uint64_t root{static_cast<uint64_t>(-1)};
        uint16_t first_node_size{static_cast<uint16_t>(-1)};
        stats stats;
    } __attribute__ ((packed));

private:
    uint64_t m_slice_ndx{static_cast<uint64_t>(-1)};

    storage::file_reader m_reader;

    uint64_t m_key_count{0};

    uint64_t m_root{static_cast<uint64_t>(-1)};
    uint64_t m_first_node_size{0};

    bool m_unlink{false};

private:
    uint64_t find_node_for(uint64_t location,
                           const std::string_view& min_key,
                           const std::string_view& max_key) const;

    std::shared_ptr<node> load(uint64_t location) const;

private:
    friend class slice_writer;
    friend class slice_iterator;
};

}
