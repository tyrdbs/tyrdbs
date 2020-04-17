#pragma once


#include <common/exception.h>


namespace tyrtech::net {


class server_error : public runtime_error
{
public:
    int16_t code() const noexcept
    {
        return m_code;
    }

public:
    template<typename... Arguments>
    server_error(int16_t code, Arguments&&... arguments)
      : runtime_error(std::forward<Arguments>(arguments)...)
      , m_code(code)
    {
    }

private:
    int16_t m_code;
};


#define DEFINE_SERVER_EXCEPTION(code, base_name, name)                \
    class name : public base_name                                     \
    {                                                                 \
    public:                                                           \
        template<typename... Arguments>                               \
        name(Arguments&&... arguments)                                \
          : base_name(code, std::forward<Arguments>(arguments)...)    \
        {                                                             \
        }                                                             \
    }


DEFINE_SERVER_EXCEPTION(-1, server_error, unknown_module_error);
DEFINE_SERVER_EXCEPTION(-2, server_error, unknown_function_error);
DEFINE_SERVER_EXCEPTION(-3, server_error, unknown_exception_error);

}
