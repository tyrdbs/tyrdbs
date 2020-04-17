#pragma once

#include <common/disallow_copy.h>
#include <common/disallow_move.h>

#include <cstdint>
#include <string>

namespace tyrtech::tyrdbs {


struct iterator : private disallow_copy, disallow_move
{
    virtual bool next() = 0;

    virtual std::string_view key() const = 0;
    virtual std::string_view value() const = 0;
    virtual bool eor() const = 0;
    virtual bool deleted() const = 0;
    virtual uint64_t idx() const = 0;

    virtual ~iterator() = default;
};

}
