#include <common/uuid.h>

#include <uuid/uuid.h>


namespace tyrtech {


std::string_view uuid()
{
    uuid_t uuid;

    static thread_local char str[37];

    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, str);

    return std::string_view(str, 36);
}

}
