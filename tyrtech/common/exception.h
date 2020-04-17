#pragma once


#include <common/fmt.h>

#include <exception>


namespace tyrtech {


class exception : public std::exception
{
public:
    const char* what() const noexcept override
    {
        return m_message.c_str();
    }

public:
    template<typename... Arguments>
    exception(Arguments&&... arguments)
      : m_message(fmt::format(std::forward<Arguments>(arguments)...))
    {
    }

private:
    std::string m_message;
};

#define DEFINE_EXCEPTION(base_name, name)                             \
    class name : public base_name                                     \
    {                                                                 \
    public:                                                           \
        template<typename... Arguments>                               \
        name(Arguments&&... arguments)                                \
          : base_name(std::forward<Arguments>(arguments)...)          \
        {                                                             \
        }                                                             \
    }


DEFINE_EXCEPTION(exception, runtime_error);
DEFINE_EXCEPTION(exception, io_error);

}
