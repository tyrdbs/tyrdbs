#pragma once


#include <gt/engine.h>


namespace tyrtech::gt {


class mutex : private disallow_copy
{
public:
    void lock();
    void unlock();
    context_t owner() const;

private:
    context_t m_owner{nullptr};
    context_queue_t m_wait_queue{new_context_queue()};
};

}
