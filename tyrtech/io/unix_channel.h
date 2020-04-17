#pragma once


#include <io/channel.h>


namespace tyrtech::io::unix {


std::shared_ptr<channel> connect(const std::string_view& path, uint64_t timeout);
std::shared_ptr<channel> listen(const std::string_view& path);

}
