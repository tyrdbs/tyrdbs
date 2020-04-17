#include <gt/condition.h>


namespace tyrtech::gt {


void condition::wait()
{
    m_wait_queue.push_back(current_context());
    yield(false);
}

void condition::signal()
{
    if (m_wait_queue.empty() == false)
    {
        context_t ctx = *m_wait_queue.front_item();
        m_wait_queue.pop_front();

        enqueue(ctx);
    }
}

void condition::signal_all()
{
    while (m_wait_queue.empty() == false)
    {
        context_t ctx = *m_wait_queue.front_item();
        m_wait_queue.pop_front();

        enqueue(ctx);
    }
}

condition::condition(condition&& other)
{
    *this = std::move(other);
}

condition& condition::operator=(condition&& other)
{
    m_wait_queue = std::move(other.m_wait_queue);

    return *this;
}

}
