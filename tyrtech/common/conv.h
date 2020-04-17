#pragma once


#include <common/exception.h>


namespace tyrtech::conv {


DEFINE_EXCEPTION(runtime_error, format_error);


template<typename T>
T parse(const std::string_view& value);

}
