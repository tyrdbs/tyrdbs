#pragma once


#include <io/channel.h>


namespace tyrtech::io {


class channel_writer : private disallow_copy
{
public:
    DEFINE_EXCEPTION(channel::error, error);

public:
    uint32_t write(const char* data, uint32_t size);

public:
    channel_writer(channel* channel);

private:
    channel* m_channel{nullptr};
};

}
