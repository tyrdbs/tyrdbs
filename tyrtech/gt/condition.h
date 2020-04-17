#pragma once


#include <gt/engine.h>


namespace tyrtech::gt {


class condition : private disallow_copy
{
public:
    void wait();

    void signal();
    void signal_all();

public:
    condition() = default;

public:
    condition(condition&& other);
    condition& operator=(condition&& other);

private:
    context_queue_t m_wait_queue{new_context_queue()};
};

}
