#pragma once


#include <fmt/format.h>


namespace tyrtech {


template<typename... Arguments>
std::string_view format_to(char* sink, uint32_t size, Arguments&&... arguments)
{
    auto r = fmt::format_to_n(sink, size - 1, std::forward<Arguments>(arguments)...);
    sink[r.size] = 0;

    return std::string_view(sink, r.size);
}

}
