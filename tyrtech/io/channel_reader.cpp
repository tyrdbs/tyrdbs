#include <io/channel_reader.h>


namespace tyrtech::io {


uint32_t channel_reader::read(char* data, uint32_t size)
{
    return m_channel->recv(data, size, 0);
}

channel_reader::channel_reader(channel* channel)
  : m_channel(channel)
{
}

}
