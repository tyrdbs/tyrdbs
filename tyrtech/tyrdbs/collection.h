#pragma once


#include <tyrdbs/ushard.h>


namespace tyrtech::tyrdbs {


class collection : private disallow_copy, disallow_move
{
public:
    std::shared_ptr<ushard> get_ushard(uint32_t ushard_id, bool auto_create);

    void drop_ushard(uint32_t ushard_id);
    void drop();

    std::string_view name() const;

public:
    collection(const std::string_view& name);
    ~collection();

private:
    using ushard_ptr =
            std::shared_ptr<ushard>;

    using ushard_map_t =
            std::unordered_map<uint32_t, ushard_ptr>;

private:
    char m_name[256];
    std::string_view m_name_view;

    ushard_map_t m_ushard_map;

    bool m_dropped{false};
};

}
