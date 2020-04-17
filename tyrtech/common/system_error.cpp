#include <common/system_error.h>

#include <errno.h>
#include <string.h>


namespace tyrtech {


error system_error(int32_t code)
{
    error _error;

    if (code != 0)
    {
        _error.code = code;
    }
    else
    {
        _error.code = errno;
    }

    static thread_local char buffer[1024];

    _error.message = strerror_r(_error.code, buffer, sizeof(buffer));

    return _error;
}

}
