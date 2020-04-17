#include <common/branch_prediction.h>
#include <tyrdbs/collection.h>

#include <cassert>


namespace tyrtech::tyrdbs {


std::shared_ptr<ushard> collection::get_ushard(uint32_t ushard_id, bool auto_create)
{
    auto it = m_ushard_map.find(ushard_id);

    if (unlikely(it == m_ushard_map.end()))
    {
        if (auto_create == true)
        {
            auto s = std::make_shared<ushard>();
            m_ushard_map[ushard_id] = s;

            return s;
        }
        else
        {
            return std::shared_ptr<ushard>();
        }
    }

    return it->second;
}

void collection::drop_ushard(uint32_t ushard_id)
{
    auto it = m_ushard_map.find(ushard_id);

    if (unlikely(it == m_ushard_map.end()))
    {
        return;
    }

    it->second->drop();
    m_ushard_map.erase(it);
}

void collection::drop()
{
    m_dropped = true;
}

std::string_view collection::name() const
{
    return m_name_view;
}

collection::collection(const std::string_view& name)
{
    m_name_view = format_to(m_name, sizeof(m_name), "{}", name);
}

collection::~collection()
{
    if (m_dropped == false)
    {
        return;
    }

    for (auto&& it : m_ushard_map)
    {
        it.second->drop();
    }

    m_ushard_map.clear();
}

}
