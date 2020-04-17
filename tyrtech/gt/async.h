#pragma once


#include <common/slab_list.h>
#include <gt/engine.h>


namespace tyrtech::gt::async {


class job : private disallow_copy
{
public:
    void run(function_t&& function);
    void wait();

private:
    context_queue_t m_wait_queue{new_context_queue()};
    bool m_done{false};

private:
    void signal();
};


using jobs_t =
        slab_list<job, 128>;


class jobs : private disallow_copy
{
public:
    template<typename... Arguments>
    void run(Arguments&&... arguments)
    {
        queue_job(std::bind(std::forward<Arguments>(arguments)...));
    }

    void wait();

private:
    jobs_t m_jobs;

private:
    jobs(jobs_t::entry_pool_t* entry_pool);

private:
    void queue_job(function_t job);

private:
    friend jobs create_jobs();
};


void initialize();
jobs create_jobs();

}
