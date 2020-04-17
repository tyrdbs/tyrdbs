#pragma once


#include <tyrdbs/slice_writer.h>


namespace tyrtech::tyrdbs {


class ushard : private disallow_copy, disallow_move
{
public:
    static constexpr uint32_t max_slices_per_tier{4};

public:
    using slice_ptr =
            std::shared_ptr<slice>;

    using slices_t =
            std::vector<slice_ptr>;

public:
    struct meta_callback
    {
        virtual void add(const slice_ptr& slice) = 0;
        virtual void remove(const slices_t& slices) = 0;

        virtual void merge(uint16_t tier) = 0;

        virtual ~meta_callback() = default;
    };

public:
    std::unique_ptr<iterator> range(const std::string_view& min_key,
                                    const std::string_view& max_key);
    std::unique_ptr<iterator> begin();

    void add(slice_ptr slice, meta_callback* cb);

    uint64_t merge(uint32_t tier, meta_callback* cb);
    uint64_t compact(meta_callback* cb);

    void drop();

    slices_t get_slices() const;

public:
    ~ushard();

private:
    using tier_map_t =
            std::unordered_map<uint32_t, slices_t>;

private:
    tier_map_t m_tier_map;
    bool m_dropped{false};

private:
    uint32_t tier_of(const slice_ptr& slice);

    slices_t get_slices_for(uint32_t tier);
    uint64_t key_count(const slices_t& slices);

    void add(slice_ptr slice, meta_callback* cb, bool use_add_callback);
    void remove_from(uint32_t tier, uint32_t count, meta_callback* cb);
};

}
