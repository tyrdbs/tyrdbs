#pragma once


#include <io/channel.h>


namespace tyrtech::io {


class channel_reader : private disallow_copy
{
public:
    DEFINE_EXCEPTION(channel::error, error);

public:
    uint32_t read(char* data, uint32_t size);

public:
    channel_reader(channel* channel);

private:
    channel* m_channel{nullptr};
};

}
