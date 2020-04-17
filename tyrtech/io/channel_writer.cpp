#include <io/channel_writer.h>


namespace tyrtech::io {


uint32_t channel_writer::write(const char* data, uint32_t size)
{
    return m_channel->send(data, size, 0);
}

channel_writer::channel_writer(channel* channel)
  : m_channel(channel)
{
}

}
