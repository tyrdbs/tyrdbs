#pragma once


#include <cstdint>


namespace tyrtech::tyrdbs {


struct data_attributes
{
    uint64_t idx;
} __attribute__ ((packed));

struct index_attributes
{
    uint64_t location;
} __attribute__ ((packed));

}
