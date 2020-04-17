#include <gt/mutex.h>


namespace tyrtech::gt {


void mutex::lock()
{
    if (m_owner == nullptr)
    {
        m_owner = current_context();
    }
    else
    {
        m_wait_queue.push_back(current_context());
        yield(false);

        assert(likely(m_owner == nullptr));
        m_owner = current_context();
    }
}

void mutex::unlock()
{
    assert(likely(m_owner == current_context()));

    m_owner = nullptr;

    if (m_wait_queue.empty() == false)
    {
        enqueue(*m_wait_queue.front_item());
        m_wait_queue.pop_front();
    }
}

context_t mutex::owner() const
{
    return m_owner;
}

}
