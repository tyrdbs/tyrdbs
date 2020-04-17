#pragma once


#include <message/builder.h>
#include <message/parser.h>


namespace tyrtech {


struct test_struct1_builder final : public tyrtech::message::struct_builder<1, 0>
{
    struct a_builder final : public tyrtech::message::list_builder
    {
        a_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        void add_value(const std::string_view& value)
        {
            add_element();
            list_builder::add_value(value);
        }
    };

    test_struct1_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    decltype(auto) add_a()
    {
        set_offset<0>();
        return a_builder(m_builder);
    }

    static constexpr uint16_t a_bytes_required()
    {
        return a_builder::bytes_required();
    }
};

struct test_struct1_parser final : public tyrtech::message::struct_parser<1, 0>
{
    struct a_parser final : public tyrtech::message::list_parser
    {
        a_parser(const tyrtech::message::parser* parser, uint16_t offset)
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
            return tyrtech::message::element<std::string_view>().parse(m_parser, m_offset);
        }
    };

    test_struct1_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    test_struct1_parser() = default;

    bool has_a() const
    {
        return has_offset<0>();
    }

    decltype(auto) a() const
    {
        return a_parser(m_parser, offset<0>());
    }
};

struct test_struct2_builder final : public tyrtech::message::struct_builder<3, 0>
{
    struct c_builder final : public tyrtech::message::list_builder
    {
        c_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        decltype(auto) add_value()
        {
            add_element();
            return test_struct1_builder(m_builder);
        }
    };

    test_struct2_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_a(const uint16_t& value)
    {
        set_offset<0>();
        struct_builder<3, 0>::add_value(value);
    }

    static constexpr uint16_t a_bytes_required()
    {
        return tyrtech::message::element<uint16_t>::size;
    }

    void add_b(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<3, 0>::add_value(value);
    }

    static constexpr uint16_t b_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }

    decltype(auto) add_c()
    {
        set_offset<2>();
        return c_builder(m_builder);
    }

    static constexpr uint16_t c_bytes_required()
    {
        return c_builder::bytes_required();
    }
};

struct test_struct2_parser final : public tyrtech::message::struct_parser<3, 0>
{
    struct c_parser final : public tyrtech::message::list_parser
    {
        c_parser(const tyrtech::message::parser* parser, uint16_t offset)
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

    test_struct2_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    test_struct2_parser() = default;

    bool has_a() const
    {
        return has_offset<0>();
    }

    decltype(auto) a() const
    {
        return tyrtech::message::element<uint16_t>().parse(m_parser, offset<0>());
    }

    bool has_b() const
    {
        return has_offset<1>();
    }

    decltype(auto) b() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }

    bool has_c() const
    {
        return has_offset<2>();
    }

    decltype(auto) c() const
    {
        return c_parser(m_parser, offset<2>());
    }
};

struct test_struct3_builder final : public tyrtech::message::struct_builder<6, 0>
{
    struct d_builder final : public tyrtech::message::list_builder
    {
        d_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        decltype(auto) add_value()
        {
            add_element();
            return test_struct2_builder(m_builder);
        }
    };

    struct e_builder final : public tyrtech::message::list_builder
    {
        e_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        void add_value(const std::string_view& value)
        {
            add_element();
            list_builder::add_value(value);
        }
    };

    struct f_builder final : public tyrtech::message::list_builder
    {
        f_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        void add_value(const int32_t& value)
        {
            add_element();
            list_builder::add_value(value);
        }
    };

    test_struct3_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_a(const int32_t& value)
    {
        set_offset<0>();
        struct_builder<6, 0>::add_value(value);
    }

    static constexpr uint16_t a_bytes_required()
    {
        return tyrtech::message::element<int32_t>::size;
    }

    void add_b(const std::string_view& value)
    {
        set_offset<1>();
        struct_builder<6, 0>::add_value(value);
    }

    static constexpr uint16_t b_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }

    decltype(auto) add_c()
    {
        set_offset<2>();
        return test_struct3_builder(m_builder);
    }

    decltype(auto) add_d()
    {
        set_offset<3>();
        return d_builder(m_builder);
    }

    static constexpr uint16_t d_bytes_required()
    {
        return d_builder::bytes_required();
    }

    decltype(auto) add_e()
    {
        set_offset<4>();
        return e_builder(m_builder);
    }

    static constexpr uint16_t e_bytes_required()
    {
        return e_builder::bytes_required();
    }

    decltype(auto) add_f()
    {
        set_offset<5>();
        return f_builder(m_builder);
    }

    static constexpr uint16_t f_bytes_required()
    {
        return f_builder::bytes_required();
    }
};

struct test_struct3_parser final : public tyrtech::message::struct_parser<6, 0>
{
    struct d_parser final : public tyrtech::message::list_parser
    {
        d_parser(const tyrtech::message::parser* parser, uint16_t offset)
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
            return test_struct2_parser(m_parser, m_offset);
        }
    };

    struct e_parser final : public tyrtech::message::list_parser
    {
        e_parser(const tyrtech::message::parser* parser, uint16_t offset)
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
            return tyrtech::message::element<std::string_view>().parse(m_parser, m_offset);
        }
    };

    struct f_parser final : public tyrtech::message::list_parser
    {
        f_parser(const tyrtech::message::parser* parser, uint16_t offset)
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

            m_element_size = tyrtech::message::element<int32_t>::size;

            return true;
        }

        decltype(auto) value() const
        {
            return tyrtech::message::element<int32_t>().parse(m_parser, m_offset);
        }
    };

    test_struct3_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    test_struct3_parser() = default;

    bool has_a() const
    {
        return has_offset<0>();
    }

    decltype(auto) a() const
    {
        return tyrtech::message::element<int32_t>().parse(m_parser, offset<0>());
    }

    bool has_b() const
    {
        return has_offset<1>();
    }

    decltype(auto) b() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<1>());
    }

    bool has_c() const
    {
        return has_offset<2>();
    }

    decltype(auto) c() const
    {
        return test_struct3_parser(m_parser, offset<2>());
    }

    bool has_d() const
    {
        return has_offset<3>();
    }

    decltype(auto) d() const
    {
        return d_parser(m_parser, offset<3>());
    }

    bool has_e() const
    {
        return has_offset<4>();
    }

    decltype(auto) e() const
    {
        return e_parser(m_parser, offset<4>());
    }

    bool has_f() const
    {
        return has_offset<5>();
    }

    decltype(auto) f() const
    {
        return f_parser(m_parser, offset<5>());
    }
};

}
