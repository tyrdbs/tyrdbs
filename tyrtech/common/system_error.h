#pragma once


#include <string>
#include <cstdint>


namespace tyrtech {


struct error {
    int32_t code{errno};
    std::string_view message;
};

error system_error(int32_t code = 0);

}
