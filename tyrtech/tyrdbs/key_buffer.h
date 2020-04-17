#pragma once


#include <tyrdbs/node.h>


namespace tyrtech::tyrdbs {


class key_buffer : public disallow_copy
{
public:
    std::string_view data() const;
    uint32_t size() const;

    void clear();
    void assign(const std::string_view& other);

public:
    key_buffer() noexcept = default;

private:
    using data_t =
            std::array<char, node::max_key_size>;

private:
    data_t m_data;
    uint32_t m_size{0};
};

}
