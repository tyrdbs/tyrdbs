#pragma once


#include <common/fmt.h>


namespace tyrtech::logger {


enum class level
{
    critical = 0,
    error,
    warning,
    notice,
    debug
};


extern const char* level_string[];
extern level __level;


void set(level level);


template<typename... Arguments>
void print(level level, Arguments&&... arguments)
{
    if (level > __level)
    {
        return;
    }

    fmt::print("\033[36m[{}]\033[0m {}\n",
               level_string[static_cast<int>(level)],
               fmt::format(std::forward<Arguments>(arguments)...));
}

template<typename... Arguments>
void critical(Arguments&&... arguments)
{
    print(level::critical, std::forward<Arguments>(arguments)...);
}

template<typename... Arguments>
void error(Arguments&&... arguments)
{
    print(level::error, std::forward<Arguments>(arguments)...);
}

template<typename... Arguments>
void warning(Arguments&&... arguments)
{
    print(level::warning, std::forward<Arguments>(arguments)...);
}

template<typename... Arguments>
void notice(Arguments&&... arguments)
{
    print(level::notice, std::forward<Arguments>(arguments)...);
}

template<typename... Arguments>
void debug(Arguments&&... arguments)
{
    print(level::debug, std::forward<Arguments>(arguments)...);
}

}
