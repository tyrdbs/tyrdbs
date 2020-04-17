#pragma once


#include <message/builder.h>
#include <message/parser.h>


namespace tyrtech {


struct test_struct1_builder final : public tyrtech::message::struct_builder<2, 0>
{
    test_struct1_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }
};

struct test_struct1_parser final : public tyrtech::message::struct_parser<2, 0>
{
    test_struct1_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    test_struct1_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }
};

struct test_struct_builder final : public tyrtech::message::struct_builder<4, 0>
{
    struct param4_builder final : public tyrtech::message::list_builder
    {
        param4_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        decltype(auto) add_value()
        {
            add_element();
            return test_struct1_builder(m_builder);
        }
    };

    test_struct_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_param1(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<4, 0>::add_value(value);
    }

    static constexpr uint16_t param1_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_param2(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<4, 0>::add_value(value);
    }

    static constexpr uint16_t param2_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }

    decltype(auto) add_param3()
    {
        set_offset<2>();
        return test_struct1_builder(m_builder);
    }

    decltype(auto) add_param4()
    {
        set_offset<3>();
        return param4_builder(m_builder);
    }

    static constexpr uint16_t param4_bytes_required()
    {
        return param4_builder::bytes_required();
    }
};

struct test_struct_parser final : public tyrtech::message::struct_parser<4, 0>
{
    struct param4_parser final : public tyrtech::message::list_parser
    {
        param4_parser(const tyrtech::message::parser* parser, uint16_t offset)
          : list_parser(parser, offset)
        {
        }

        bool next()
        {
            if (m_elements == 0)
            {
                return false;
            }

            m_elements--;

            m_offset += m_element_size;

            m_element_size = tyrtech::message::element<uint16_t>().parse(m_parser, m_offset);
            m_element_size += tyrtech::message::element<uint16_t>::size;

            return true;
        }

        decltype(auto) value() const
        {
            return test_struct1_parser(m_parser, m_offset);
        }
    };

    test_struct_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    test_struct_parser() = default;

    bool has_param1() const
    {
        return has_offset<0>();
    }

    decltype(auto) param1() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_param2() const
    {
        return has_offset<1>();
    }

    decltype(auto) param2() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }

    bool has_param3() const
    {
        return has_offset<2>();
    }

    decltype(auto) param3() const
    {
        return test_struct1_parser(m_parser, offset<2>());
    }

    bool has_param4() const
    {
        return has_offset<3>();
    }

    decltype(auto) param4() const
    {
        return param4_parser(m_parser, offset<3>());
    }
};

}
