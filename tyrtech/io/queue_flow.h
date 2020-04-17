#pragma once


#include <common/disallow_copy.h>
#include <gt/condition.h>


namespace tyrtech::io {


class queue_flow : private disallow_copy
{
public:
    class resource : private disallow_copy
    {
    public:
        resource(queue_flow& queue_flow);
        ~resource();

    private:
        queue_flow& m_queue_flow;
    };

public:
    void acquire();
    void release(uint32_t count = 1);

    uint32_t enqueued() const;

public:
    queue_flow(uint32_t queue_size);

private:
    uint32_t m_queue_size{0};
    uint32_t m_enqueued{0};

    gt::condition m_cond;
};

}
