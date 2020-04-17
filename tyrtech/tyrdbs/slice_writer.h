#pragma once


#include <tyrdbs/slice.h>
#include <tyrdbs/node_writer.h>
#include <tyrdbs/key_buffer.h>


namespace tyrtech::tyrdbs {


class slice_writer : private disallow_copy, disallow_move
{
public:
    DEFINE_EXCEPTION(runtime_error, invalid_data_error);

public:
    static constexpr uint64_t max_idx{0x1000000000000ULL};

public:
    void add(iterator* it, bool compact);
    void add(const std::string_view& key,
             std::string_view value,
             bool eor,
             bool deleted,
             uint64_t idx);

    void flush();
    std::shared_ptr<slice> commit();

public:
    slice_writer();
    ~slice_writer();

private:
    class index_writer : private disallow_copy, disallow_move
    {
    public:
        void add(const std::string_view& min_key,
                 const std::string_view& max_key,
                 uint64_t location);

        uint64_t flush();

    public:
        index_writer(slice_writer* writer);

    private:
        using index_writer_ptr =
                std::unique_ptr<index_writer>;

    private:
        node_writer m_node;

        key_buffer m_first_key;
        key_buffer m_last_key;

        index_writer_ptr m_higher_level;

        slice_writer* m_writer{nullptr};
    };

private:
    uint64_t m_slice_ndx{static_cast<uint64_t>(-1)};

    key_buffer m_first_key;
    key_buffer m_last_key;

    bool m_last_eor{true};

    bool m_commited{false};

    storage::file_writer m_writer;

    node_writer m_node;
    index_writer m_index{this};

    slice::header m_header;

    std::shared_ptr<node> m_last_node;

private:
    bool check(const std::string_view& key,
               const std::string_view& value,
               bool eor,
               bool deleted);

    uint64_t store(node_writer* node, bool is_leaf);
};

}
