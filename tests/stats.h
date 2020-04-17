#pragma once


#include <common/disallow_copy.h>
#include <common/clock.h>
#include <common/logger.h>

#include <tdigest.h>

#include <cstdint>
#include <cmath>
#include <vector>


namespace tests {


class stats : private tyrtech::disallow_copy
{
public:
    class timer_sample : private tyrtech::disallow_copy
    {
    public:
        ~timer_sample()
        {
            m_stats->add(tyrtech::clock::now() - m_t1);
        }

    private:
        timer_sample(stats* stats)
          : m_stats(stats)
        {
        }

    private:
        stats* m_stats{nullptr};
        uint64_t m_t1{tyrtech::clock::now()};

    private:
        friend class stats;
    };

public:
    timer_sample stopwatch()
    {
        return timer_sample(this);
    }

    void add(uint64_t sample)
    {
        td_add(m_hist, sample, 1);

        m_min = std::min(m_min, sample);
        m_max = std::max(m_max, sample);
        m_sum += sample;
        m_n++;
    }

    uint64_t n() const
    {
        return m_n;
    }

    uint64_t sum() const
    {
        return m_sum;
    }

    uint64_t min() const
    {
        return m_min;
    }

    uint64_t max() const
    {
        return m_max;
    }

    float avg() const
    {
        return static_cast<float>(m_sum) / m_n;
    }

    float value_at(float p) const
    {
        return td_value_at(m_hist, p);
    }

    void report()
    {
        tyrtech::logger::notice("elements: {}, sum: {}, min: {}, max: \033[31m{}\033[0m",
                                n(),
                                sum(),
                                min(),
                                max());
        tyrtech::logger::notice("  avg: \033[1m{:.2f}\033[0m, 50%: {:.2f}, 90%: {:.2f}, "
                                "95%: {:.2f}, 99%: {:.2f}, 99.99%: \033[1m{:.2f}\033[0m",
                                avg(),
                                value_at(0.5),
                                value_at(0.9),
                                value_at(0.95),
                                value_at(0.99),
                                value_at(0.9999));
    }

public:
    stats()
    {
        m_hist = td_new(1000);

        if (m_hist == nullptr)
        {
            throw std::bad_alloc();
        }
    }

    ~stats()
    {
        td_free(m_hist);
        m_hist = nullptr;
    }

public:
    stats(stats&& other) noexcept
    {
        *this = std::move(other);
    }

    stats& operator=(stats&& other) noexcept
    {
        m_hist = other.m_hist;
        m_min = other.m_min;
        m_max = other.m_max;
        m_sum = other.m_sum;
        m_n = other.m_n;

        other.m_hist = nullptr;

        return *this;
    }

private:
    td_histogram* m_hist{nullptr};

    uint64_t m_min{static_cast<uint64_t>(-1)};
    uint64_t m_max{0};
    uint64_t m_sum{0};
    uint32_t m_n{0};
};

}
