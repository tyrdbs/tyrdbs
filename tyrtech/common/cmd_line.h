#pragma once


#include <common/disallow_copy.h>
#include <common/branch_prediction.h>
#include <common/conv.h>

#include <unordered_map>
#include <vector>
#include <cstdio>
#include <cassert>


namespace tyrtech {


class cmd_line : private disallow_copy
{
public:
    DEFINE_EXCEPTION(runtime_error, error);

public:
    using glossary_t =
            std::vector<std::string_view>;

public:
    void add_flag(const char* name,
                  const char* short_opts,
                  const char* long_opts,
                  const glossary_t& glossary);

    void add_param(const char* name,
                   const char* short_opts,
                   const char* long_opts,
                   const char* data_type,
                   const glossary_t& glossary);

    void add_param(const char* name,
                   const char* short_opts,
                   const char* long_opts,
                   const char* data_type,
                   const char* default_value,
                   const glossary_t& glossary);

    void add_param(const char* name,
                   const char* data_type,
                   const glossary_t& glossary);

    void add_separator();

    bool flag(const char* name);
    bool has(const char* name);

    void parse(int32_t argc, const char* argv[]);
    const char* get(const char* name) const;

    template<typename T>
    T get(const char* name) const
    {
        const char* value = get(name);

        try
        {
            return conv::parse<T>(std::string_view(value));
        }
        catch (conv::format_error& e)
        {
            throw error("invalid argument: {}", e.what());
        }
    }

public:
    cmd_line(const char* prog_name, const char* description, const char* epilogue);
    ~cmd_line();

private:
    using arg_table_t =
            std::vector<void*>;

    using arg_map_t =
            std::unordered_map<const char*, uint32_t>;

    using arg_types_t =
            std::vector<bool>;

private:
    const char* m_prog_name{nullptr};
    const char* m_description{nullptr};
    const char* m_epilogue{nullptr};

    arg_map_t m_arg_map;

    arg_table_t m_arg_table;
    arg_types_t m_arg_types;

    bool m_end_added{false};

private:
    void print_help();
    void print_error();

    void add_param(const char* name,
                   const char* short_opts,
                   const char* long_opts,
                   const char* data_type,
                   bool required,
                   const char* default_value,
                   const glossary_t& glossary);
    void add_end_if_needed();

    uint32_t get_ndx_for(const char* name) const;

    const char* first_glossary_line(const glossary_t& glossary);
    void add_other_glossary_lines(const glossary_t& glossary);
};

}
