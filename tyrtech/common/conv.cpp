#include <common/conv.h>

#include <charconv>


namespace tyrtech::conv {


template<>
uint16_t parse(const std::string_view& value)
{
    uint16_t parsed_value;

    auto res = std::from_chars(value.begin(), value.end(), parsed_value);

    if (res.ptr != value.end() || res.ec != std::errc())
    {
        throw format_error("can't convert '{}' to u16", value);
    }

    return parsed_value;
}

template<>
uint32_t parse(const std::string_view& value)
{
    uint32_t parsed_value;

    auto res = std::from_chars(value.begin(), value.end(), parsed_value);

    if (res.ptr != value.end() || res.ec != std::errc())
    {
        throw format_error("can't convert '{}' to u32", value);
    }

    return parsed_value;
}

template<>
uint64_t parse(const std::string_view& value)
{
    uint64_t parsed_value;

    auto res = std::from_chars(value.begin(), value.end(), parsed_value);

    if (res.ptr != value.end() || res.ec != std::errc())
    {
        throw format_error("can't convert '{}' to u64", value);
    }

    return parsed_value;
}

template<>
std::string_view parse(const std::string_view& value)
{
    return value;
}

}
