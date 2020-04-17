#include <common/cmd_line.h>

#include <argtable3/argtable3.h>


namespace tyrtech {


void cmd_line::add_flag(const char* name,
                        const char* short_opts,
                        const char* long_opts,
                        const glossary_t& glossary)
{
    assert(likely(m_arg_map.find(name) == m_arg_map.end()));

    m_arg_map[name] = m_arg_table.size();

    struct arg_lit* arg = arg_litn(short_opts,
                                   long_opts,
                                   0,
                                   1,
                                   first_glossary_line(glossary));

    if (arg == nullptr)
    {
        throw std::bad_alloc();
    }

    m_arg_table.push_back(arg);
    m_arg_types.push_back(true);

    add_other_glossary_lines(glossary);
}

void cmd_line::add_param(const char* name,
                         const char* short_opts,
                         const char* long_opts,
                         const char* data_type,
                         const glossary_t& glossary)
{
    add_param(name, short_opts, long_opts, data_type, true, nullptr, glossary);
}

void cmd_line::add_param(const char* name,
                         const char* short_opts,
                         const char* long_opts,
                         const char* data_type,
                         const char* default_value,
                         const glossary_t& glossary)
{
    add_param(name, short_opts, long_opts, data_type, false, default_value, glossary);
}

void cmd_line::add_param(const char* name,
                         const char* data_type,
                         const glossary_t& glossary)
{
    add_param(name, nullptr, nullptr, data_type, true, nullptr, glossary);
}

void cmd_line::add_separator()
{
    struct arg_rem* arg = arg_rem(nullptr, "\n");

    if (arg == nullptr)
    {
        throw std::bad_alloc();
    }

    m_arg_table.push_back(arg);
}

bool cmd_line::flag(const char* name)
{
    uint32_t ndx = get_ndx_for(name);
    assert(likely(m_arg_types[ndx] == true));

    struct arg_lit* arg = reinterpret_cast<struct arg_lit*>(m_arg_table[ndx]);
    return arg->count == 1;
}

bool cmd_line::has(const char* name)
{
    uint32_t ndx = get_ndx_for(name);
    assert(likely(m_arg_types[ndx] == false));

    struct arg_str* arg = reinterpret_cast<struct arg_str*>(m_arg_table[ndx]);
    return arg->sval[0] != nullptr;
}

void cmd_line::parse(int32_t argc, const char* argv[])
{
    add_end_if_needed();

    int32_t res = arg_parse(argc, const_cast<char**>(argv), m_arg_table.data());

    if (flag("help") == true)
    {
        print_help();
        exit(1);
    }

    if (res > 0)
    {
        print_error();
        exit(1);
    }
}

const char* cmd_line::get(const char* name) const
{
    uint32_t ndx = get_ndx_for(name);
    assert(likely(m_arg_types[ndx] == false));

    struct arg_str* arg = reinterpret_cast<struct arg_str*>(m_arg_table[ndx]);

    const char* value = arg->sval[0];
    assert(likely(value != nullptr));

    return value;
}

cmd_line::cmd_line(const char* prog_name, const char* description, const char* epilogue)
  : m_prog_name(prog_name)
  , m_description(description)
  , m_epilogue(epilogue)
{
    add_flag("help", nullptr, "help", {"display this help and exit"});
}

cmd_line::~cmd_line()
{
    arg_freetable(m_arg_table.data(), m_arg_table.size());
}

void cmd_line::print_help()
{
    arg_print_help_tyrtech(stdout,
                           m_arg_table.data(),
                           m_prog_name,
                           m_description,
                           m_epilogue);
}

void cmd_line::print_error()
{
    arg_print_error_tyrtech(stdout, reinterpret_cast<struct arg_end*>(m_arg_table.back()), m_prog_name);
}

void cmd_line::add_param(const char* name,
                         const char* short_opts,
                         const char* long_opts,
                         const char* data_type,
                         bool required,
                         const char* default_value,
                         const glossary_t& glossary)
{
    assert(likely(m_arg_map.find(name) == m_arg_map.end()));
    assert(likely((required == true) || ((required == false) && (short_opts != nullptr || long_opts != nullptr))));

    m_arg_map[name] = m_arg_table.size();

    int32_t min_count = (required == true) ? 1 : 0;

    struct arg_str* arg = arg_strn(short_opts,
                                   long_opts,
                                   data_type,
                                   min_count,
                                   1,
                                   first_glossary_line(glossary));

    if (arg == nullptr)
    {
        throw std::bad_alloc();
    }

    arg->sval[0] = default_value;

    m_arg_table.push_back(arg);
    m_arg_types.push_back(false);

    add_other_glossary_lines(glossary);
}

void cmd_line::add_end_if_needed()
{
    if (m_end_added == true)
    {
        return;
    }

    struct arg_end* end = arg_end(2);

    if (end == nullptr)
    {
        throw std::bad_alloc();
    }

    m_arg_table.push_back(end);
    m_end_added = true;
}

uint32_t cmd_line::get_ndx_for(const char* name) const
{
    auto it = m_arg_map.find(name);
    assert(likely(it != m_arg_map.end()));

    return it->second;
}

const char* cmd_line::first_glossary_line(const glossary_t& glossary)
{
    if (glossary.size() == 0)
    {
        return nullptr;
    }

    return glossary[0].data();
}

void cmd_line::add_other_glossary_lines(const glossary_t& glossary)
{
    for (uint32_t i = 1; i < glossary.size(); i++)
    {
        struct arg_rem* arg = arg_rem(nullptr, glossary[i].data());

        if (arg == nullptr)
        {
            throw std::bad_alloc();
        }

        m_arg_table.push_back(arg);
    }
}

}
