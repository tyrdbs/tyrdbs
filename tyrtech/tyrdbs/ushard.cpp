#include <gt/async.h>
#include <tyrdbs/ushard.h>


namespace tyrtech::tyrdbs {


class ushard_iterator : public iterator
{
public:
    bool next() override;

    std::string_view key() const override;
    std::string_view value() const override;
    bool eor() const override;
    bool deleted() const override;
    uint64_t idx() const override;

public:
    ushard_iterator(ushard::slices_t&& slices,
                    const std::string_view& min_key,
                    const std::string_view& max_key);
    ushard_iterator(ushard::slices_t&& slices);

private:
    using element_t =
            std::pair<ushard::slice_ptr, std::unique_ptr<iterator>>;

    using elements_t =
            std::vector<element_t>;

private:
    elements_t m_elements;

    key_buffer m_max_key;
    key_buffer m_last_key;

private:
    struct cmp
    {
        bool operator()(const element_t& e1, const element_t& e2)
        {
            auto& it1 = e1.second;
            auto& it2 = e2.second;

            int32_t cmp = it1->key().compare(it2->key());

            if (cmp == 0)
            {
                return it1->idx() < it2->idx();
            }

            return cmp > 0;
        }
    };

private:
    bool is_out_of_bounds();

    bool advance_last();
    bool advance();
};

bool ushard_iterator::next()
{
    if (m_elements.size() == 0)
    {
        return false;
    }

    if (m_last_key.size() == 0)
    {
        std::sort(m_elements.begin(), m_elements.end(), cmp());
        m_last_key.assign(m_elements.back().second->key());

        if (is_out_of_bounds() == true)
        {
            m_elements.clear();
            return false;
        }

        return true;
    }

    if (eor() == false)
    {
        bool has_next = advance_last();
        assert(likely(has_next == true));

        return true;
    }

    while (advance() == true)
    {
        if (is_out_of_bounds() == true)
        {
            m_elements.clear();
            return false;
        }

        if (key().compare(m_last_key.data()) != 0)
        {
            m_last_key.assign(key());
            return true;
        }
    }

    return false;
}

std::string_view ushard_iterator::key() const
{
    return m_elements.back().second->key();
}

std::string_view ushard_iterator::value() const
{
    return m_elements.back().second->value();
}

bool ushard_iterator::eor() const
{
    return m_elements.back().second->eor();
}

bool ushard_iterator::deleted() const
{
    return m_elements.back().second->deleted();
}

uint64_t ushard_iterator::idx() const
{
    return m_elements.back().second->idx();
}

ushard_iterator::ushard_iterator(ushard::slices_t&& slices,
                                 const std::string_view& min_key,
                                 const std::string_view& max_key)
{
    m_elements.reserve(slices.size());

    auto jobs = gt::async::create_jobs();

    for (auto&& slice : slices)
    {
        auto f = [this, slice = std::move(slice), &min_key, &max_key]
        {
            auto&& it = slice->range(min_key, max_key);

            if (it == nullptr)
            {
                return;
            }

            if (it->next() == false)
            {
                return;
            }

            if (it->key().compare(max_key) > 0)
            {
                return;
            }

            this->m_elements.emplace_back(element_t(std::move(slice),
                                                    std::move(it)));
        };

        jobs.run(std::move(f));
    }

    jobs.wait();

    if (m_elements.size() != 0)
    {
        m_max_key.assign(max_key);
    }
}

ushard_iterator::ushard_iterator(ushard::slices_t&& slices)
{
    m_elements.reserve(slices.size());

    for (auto&& slice : slices)
    {
        auto&& it = slice->begin();

        if (it == nullptr)
        {
            continue;
        }

        if (it->next() == true)
        {
            this->m_elements.emplace_back(element_t(std::move(slice),
                                                    std::move(it)));
        }
    }
}

bool ushard_iterator::is_out_of_bounds()
{
    if (m_max_key.size() == 0)
    {
        return false;
    }

    return key().compare(m_max_key.data()) > 0;
}

bool ushard_iterator::advance_last()
{
    return m_elements.back().second->next();
}

bool ushard_iterator::advance()
{
    bool has_next = advance_last();

    if (has_next == false)
    {
        m_elements.pop_back();
        return m_elements.size() != 0;
    }

    auto&& it = std::lower_bound(m_elements.begin(),
                                 m_elements.end(),
                                 m_elements.back(),
                                 cmp());

    if (it != --m_elements.end())
    {
        element_t e = std::move(m_elements.back());
        m_elements.insert(it, std::move(e));

        m_elements.pop_back();
    }

    return true;
}

std::unique_ptr<iterator> ushard::range(const std::string_view& min_key,
                                        const std::string_view& max_key)
{
    return std::make_unique<ushard_iterator>(get_slices(), min_key, max_key);
}

std::unique_ptr<iterator> ushard::begin()
{
    return std::make_unique<ushard_iterator>(get_slices());
}

void ushard::add(slice_ptr slice, meta_callback* cb)
{
    add(std::move(slice), cb, true);
}

uint64_t ushard::merge(uint32_t tier, meta_callback* cb)
{
    auto&& tier_slices = get_slices_for(tier);
    uint32_t count = tier_slices.size();

    if (count <= max_slices_per_tier)
    {
        return 0;
    }

    cb->remove(tier_slices);

    auto source_key_count = key_count(tier_slices);

    slice_writer target;
    ushard_iterator it(std::move(tier_slices));

    target.add(&it, false);
    target.flush();

    add(target.commit(), cb);
    remove_from(tier, count, cb);

    return source_key_count;
}

uint64_t ushard::compact(meta_callback* cb)
{
    auto&& slices = get_slices();

    if (slices.size() < 2)
    {
        return 0;
    }

    cb->remove(slices);

    auto source_key_count = key_count(slices);

    tier_map_t tier_map_checkpoint = m_tier_map;

    slice_writer target;
    ushard_iterator it(std::move(slices));

    target.add(&it, true);
    target.flush();

    add(target.commit(), cb);

    for (auto&& it : tier_map_checkpoint)
    {
        remove_from(it.first, it.second.size(), cb);
    }

    return source_key_count;
}

void ushard::drop()
{
    m_dropped = true;
}

ushard::slices_t ushard::get_slices() const
{
    slices_t slices;

    for (auto&& it : m_tier_map)
    {
        std::copy(it.second.begin(),
                  it.second.end(),
                  std::back_inserter(slices));
    }

    return slices;
}

ushard::~ushard()
{
    if (m_dropped == false)
    {
        return;
    }

    for (auto&& slice : get_slices())
    {
        slice->unlink();
    }

    m_tier_map.clear();
}

uint32_t ushard::tier_of(const slice_ptr& slice)
{
    return (64 - __builtin_clzll(slice->key_count())) >> 2;
}

ushard::slices_t ushard::get_slices_for(uint32_t tier)
{
    auto&& tier_slices = m_tier_map[tier];
    slices_t slices;

    slices.reserve(tier_slices.size());

    std::copy(tier_slices.begin(),
              tier_slices.end(),
              std::back_inserter(slices));

    return slices;
}

uint64_t ushard::key_count(const slices_t& slices)
{
    uint64_t key_count = 0;

    for (auto&& slice : slices)
    {
        key_count += slice->key_count();
    }

    return key_count;
}

void ushard::add(slice_ptr slice, meta_callback* cb, bool use_add_callback)
{
    if (use_add_callback == true)
    {
        cb->add(slice);
    }

    uint32_t tier = tier_of(slice);
    auto&& it = m_tier_map.find(tier);

    if (it == m_tier_map.end())
    {
        it = m_tier_map.insert(tier_map_t::value_type(tier, slices_t())).first;
    }

    it->second.emplace_back(std::move(slice));

    if (it->second.size() > max_slices_per_tier)
    {
        cb->merge(tier);
    }
}

void ushard::remove_from(uint32_t tier, uint32_t count, meta_callback* cb)
{
    auto& tier_slices = m_tier_map[tier];
    tier_slices.erase(tier_slices.begin(), tier_slices.begin() + count);

    if (tier_slices.size() > max_slices_per_tier)
    {
        cb->merge(tier);
    }
}

}
