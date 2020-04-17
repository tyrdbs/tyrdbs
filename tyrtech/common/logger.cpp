#include <common/logger.h>


namespace tyrtech::logger {


const char* level_string[] = {
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTICE",
    "DEBUG"
};


level __level{level::notice};


void set(level level)
{
    __level = level;
}

}
