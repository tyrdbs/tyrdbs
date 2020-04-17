#pragma once


#include <message/builder.h>
#include <message/parser.h>


namespace tests {


struct slice_builder final : public tyrtech::message::struct_builder<1, 0>
{
    struct extents_builder final : public tyrtech::message::list_builder
    {
        extents_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        void add_value(const uint64_t& value)
        {
            add_element();
            list_builder::add_value(value);
        }
    };

    slice_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    decltype(auto) add_extents()
    {
        set_offset<0>();
        return extents_builder(m_builder);
    }

    static constexpr uint16_t extents_bytes_required()
    {
        return extents_builder::bytes_required();
    }
};

struct slice_parser final : public tyrtech::message::struct_parser<1, 0>
{
    struct extents_parser final : public tyrtech::message::list_parser
    {
        extents_parser(const tyrtech::message::parser* parser, uint16_t offset)
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

            m_element_size = tyrtech::message::element<uint64_t>::size;

            return true;
        }

        decltype(auto) value() const
        {
            return tyrtech::message::element<uint64_t>().parse(m_parser, m_offset);
        }
    };

    slice_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    slice_parser() = default;

    bool has_extents() const
    {
        return has_offset<0>();
    }

    decltype(auto) extents() const
    {
        return extents_parser(m_parser, offset<0>());
    }
};

struct snapshot_builder final : public tyrtech::message::struct_builder<2, 0>
{
    struct slices_builder final : public tyrtech::message::list_builder
    {
        slices_builder(tyrtech::message::builder* builder)
          : list_builder(builder)
        {
        }

        decltype(auto) add_value()
        {
            add_element();
            return slice_builder(m_builder);
        }
    };

    snapshot_builder(tyrtech::message::builder* builder)
      : struct_builder(builder)
    {
    }

    void add_path(const std::string_view& value)
    {
        set_offset<0>();
        struct_builder<2, 0>::add_value(value);
    }

    static constexpr uint16_t path_bytes_required()
    {
        return tyrtech::message::element<std::string_view>::size;
    }

    decltype(auto) add_slices()
    {
        set_offset<1>();
        return slices_builder(m_builder);
    }

    static constexpr uint16_t slices_bytes_required()
    {
        return slices_builder::bytes_required();
    }
};

struct snapshot_parser final : public tyrtech::message::struct_parser<2, 0>
{
    struct slices_parser final : public tyrtech::message::list_parser
    {
        slices_parser(const tyrtech::message::parser* parser, uint16_t offset)
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
            return slice_parser(m_parser, m_offset);
        }
    };

    snapshot_parser(const tyrtech::message::parser* parser, uint16_t offset)
      : struct_parser(parser, offset)
    {
    }

    snapshot_parser() = default;

    bool has_path() const
    {
        return has_offset<0>();
    }

    decltype(auto) path() const
    {
        return tyrtech::message::element<std::string_view>().parse(m_parser, offset<0>());
    }

    bool has_slices() const
    {
        return has_offset<1>();
    }

    decltype(auto) slices() const
    {
        return slices_parser(m_parser, offset<1>());
    }
};

}
