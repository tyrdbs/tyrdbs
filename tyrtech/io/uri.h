#pragma once


#include <io/channel.h>


namespace tyrtech::io::uri {


std::shared_ptr<channel> connect(const std::string_view& uri, uint64_t timeout);
std::shared_ptr<channel> listen(const std::string_view& uri);

}
