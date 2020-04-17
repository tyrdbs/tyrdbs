#include <gt/async.h>


namespace tyrtech::gt::async {


void job::run(function_t&& function)
{
    m_done = false;

    auto f = [this]
    {
        this->signal();
    };

    auto ctx = create_thread(function);
    set_terminate_callback(ctx, std::move(f));
}

void job::wait()
{
    if (m_done == true)
    {
        return;
    }

    m_wait_queue.push_back(current_context());

    yield(false);

    assert(likely(m_done == true));
}

void job::signal()
{
    m_done = true;

    while (m_wait_queue.empty() == false)
    {
        context_t ctx = *m_wait_queue.front_item();
        m_wait_queue.pop_front();

        enqueue(ctx);
    }
}

void jobs::wait()
{
    while (m_jobs.empty() == false)
    {
        m_jobs.front_item()->wait();
        m_jobs.pop_front();
    }
}

void jobs::queue_job(function_t job)
{
    m_jobs.push_back();
    m_jobs.back_item()->run(std::move(job));
}

jobs::jobs(jobs_t::entry_pool_t* entry_pool)
  : m_jobs(entry_pool)
{
}


thread_local std::unique_ptr<jobs_t::entry_pool_t> __entry_pool;


void initialize()
{
    __entry_pool = std::make_unique<jobs_t::entry_pool_t>();
}

jobs create_jobs()
{
    return jobs(__entry_pool.get());
}

}
