#pragma once


#include <io/channel.h>


namespace tyrtech::io::tcp {


std::shared_ptr<channel> connect(const std::string_view& host, const std::string_view& service, uint64_t timeout);
std::shared_ptr<channel> listen(const std::string_view& host, const std::string_view& service);

}
