#include <tyrdbs/slice_writer.h>
#include <tyrdbs/cache.h>
#include <tyrdbs/location.h>

#include <crc32c.h>


namespace tyrtech::tyrdbs {


extern thread_local uint64_t slice_count;


void slice_writer::index_writer::add(const std::string_view& min_key,
                                     const std::string_view& max_key,
                                     uint64_t location)
{
    if (m_first_key.size() == 0)
    {
        m_first_key.assign(min_key);
    }

    index_attributes attributes;
    attributes.location = location;

    auto res = m_node.add(min_key, max_key, true, false, attributes, true);

    if (res == -1)
    {
        if (m_higher_level == nullptr)
        {
            m_higher_level = std::make_unique<index_writer>(m_writer);
        }

        location = m_writer->store(&m_node, false);

        m_node.add(min_key, max_key, true, false, attributes, true);

        m_higher_level->add(m_first_key.data(), m_last_key.data(), location);
        m_first_key.assign(min_key);
    }
    else
    {
        assert(likely(res == static_cast<int32_t>(max_key.size())));
    }

    m_last_key.assign(max_key);
}

uint64_t slice_writer::index_writer::flush()
{
    uint64_t location = m_writer->store(&m_node, false);

    if (m_higher_level == nullptr)
    {
        return location;
    }
    else
    {
        m_higher_level->add(m_first_key.data(), m_last_key.data(), location);
    }

    return m_higher_level->flush();
}

slice_writer::index_writer::index_writer(slice_writer* writer)
  : m_writer(writer)
{
}

void slice_writer::add(iterator* it, bool compact)
{
    while (it->next() == true)
    {
        bool skip_key = compact == true && it->deleted() == true;

        if (skip_key == true)
        {
            continue;
        }

        add(it->key(), it->value(), it->eor(), it->deleted(), it->idx());
    }
}

void slice_writer::add(const std::string_view& key,
                       std::string_view value,
                       bool eor,
                       bool deleted,
                       uint64_t idx)
{
    assert(likely(m_commited == false));
    assert(likely(idx < max_idx));

    bool new_key = check(key, value, eor, deleted);

    if (new_key == true && m_first_key.size() == 0)
    {
        m_first_key.assign(key);
    }

    while (true)
    {
        data_attributes attributes;
        attributes.idx = idx;

        if (auto res = m_node.add(key, value, eor, deleted, attributes, false); res != -1)
        {
            value = value.substr(res, value.size() - res);

            if (value.size() == 0)
            {
                break;
            }
        }

        uint64_t location = store(&m_node, true);

        if (m_first_key.size() != 0)
        {
            m_index.add(m_first_key.data(), m_last_key.data(), location);

            if (new_key == true)
            {
                m_first_key.assign(key);
            }
            else
            {
                m_first_key.clear();
            }
        }
    }

    m_last_key.assign(key);
    m_last_eor = eor;

    m_header.stats.key_count++;
}

void slice_writer::flush()
{
    assert(likely(m_commited == false));

    if (m_last_eor == false)
    {
        throw invalid_data_error("data not complete");
    }

    uint64_t location = store(&m_node, true);
    m_writer.write(location::invalid_size);

    auto last_data_node = m_last_node;

    if (m_first_key.size() != 0)
    {
        m_index.add(m_first_key.data(), m_last_key.data(), location);
    }

    m_header.root = m_index.flush();

    last_data_node->set_next(location::invalid_size);
    m_last_node->set_next(location::invalid_size);

    m_writer.write(location::invalid_size);
    m_writer.add_padding();

    m_writer.write(m_header);
    m_writer.add_padding();

    m_writer.flush();
}

std::shared_ptr<slice> slice_writer::commit()
{
    assert(likely(m_header.root != static_cast<uint64_t>(-1)));

    assert(likely(m_commited == false));
    m_commited = true;

    auto c = std::make_shared<slice>();

    c->m_slice_ndx = m_slice_ndx;
    c->m_reader = storage::create_reader(m_writer.commit());
    c->m_key_count = m_header.stats.key_count;
    c->m_root = m_header.root;
    c->m_first_node_size = m_header.first_node_size;

    return c;
}

slice_writer::slice_writer()
  : m_slice_ndx(storage::new_cache_id())
  , m_writer(storage::create_writer())
{
    slice_count++;
}

slice_writer::~slice_writer()
{
    if (m_commited == false)
    {
        assert(likely(slice_count > 0));
        slice_count--;
    }
}

bool slice_writer::check(const std::string_view& key,
                         const std::string_view& value,
                         bool eor,
                         bool deleted)
{
    if (key.size() == 0)
    {
        throw invalid_data_error("key of zero length not allowed");
    }

    if (key.size() >= node::max_key_size)
    {
        throw invalid_data_error("maximum key size exceded");
    }

    int32_t cmp = key.compare(m_last_key.data());

    if (m_last_key.size() != 0)
    {
        if (cmp < 0)
        {
            throw invalid_data_error("input keys not sorted");
        }
        else if (cmp == 0)
        {
            if (m_last_eor == true || deleted == true)
            {
                throw invalid_data_error("duplicate keys");
            }
        }
        else
        {
            if (m_last_eor == false)
            {
                throw invalid_data_error("key eor mismatch");
            }
        }
    }

    if (deleted == true)
    {
        if (eor == false || value.size() != 0)
        {
            throw invalid_data_error("invalid combination of key attributes");
        }
    }

    return cmp != 0;
}

uint64_t slice_writer::store(node_writer* node, bool is_leaf)
{
    assert(likely(m_commited == false));

    using buffer_t =
            std::array<char, node::page_size>;

    buffer_t buffer;

    uint32_t size = node->flush(buffer.data(), buffer.size());
    assert(likely(size <= location::max_size));

    if (m_header.first_node_size != location::invalid_size)
    {
        m_writer.write(location::size(size, is_leaf));
    }
    else
    {
        assert(likely(is_leaf == true));
        m_header.first_node_size = location::size(size, is_leaf);
    }

    uint64_t location = location::location(m_writer.size(), size, is_leaf);

    m_writer.write(crc32c_update(0, buffer.data(), size));
    m_writer.write(buffer.data(), size);

    if (m_last_node != nullptr)
    {
        m_last_node->set_next(location);
    }

    m_last_node = node->reset();

    cache::set(m_slice_ndx, location, m_last_node);

    m_header.stats.compressed_size += size;
    m_header.stats.uncompressed_size += node::node_size;

    m_header.stats.total_nodes++;
    m_header.stats.leaf_nodes += is_leaf;

    return location;
}

}
