#include <io/queue_flow.h>


namespace tyrtech::io {


queue_flow::resource::resource(queue_flow& queue_flow)
  : m_queue_flow(queue_flow)
{
    m_queue_flow.acquire();
}

queue_flow::resource::~resource()
{
    m_queue_flow.release();
}

void queue_flow::acquire()
{
    while (m_enqueued == m_queue_size)
    {
        m_cond.wait();
    }

    m_enqueued++;
}

void queue_flow::release(uint32_t count)
{
    assert(likely(m_enqueued >= count));
    m_enqueued -= count;

    m_cond.signal_all();
}

uint32_t queue_flow::enqueued() const
{
    return m_enqueued;
}

queue_flow::queue_flow(uint32_t queue_size)
  : m_queue_size(queue_size)
{
}

}
