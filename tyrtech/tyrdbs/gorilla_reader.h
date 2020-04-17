#pragma once


#include <common/bit_reader.h>


namespace tyrtech::tyrdbs {


template<typename BufferReader>
class gorilla_reader : private disallow_copy
{
public:
    bool next(void)
    {
        if (m_ndx == m_samples)
        {
            return false;
        }

        load_next_timestamp();
        load_next_value();

        m_ndx++;

        return true;
    }

    decltype(auto) value() const
    {
        struct
        {
            uint64_t timestamp;
            uint64_t value;
        } v;

        v.timestamp = m_timestamp;
        v.value = m_value;

        return v;
    }

public:
    gorilla_reader(uint32_t samples, BufferReader* reader)
      : m_samples(samples)
      , m_reader(reader)
    {
    }

private:
    using reader_t =
            bit_reader<BufferReader>;

private:
    uint32_t m_samples{0};
    uint32_t m_ndx{0};

    uint64_t m_timestamp{0};
    uint64_t m_delta{0};

    uint64_t m_value{0};
    uint32_t m_value_size{static_cast<uint32_t>(-1)};
    uint32_t m_ctz{static_cast<uint32_t>(-1)};

    reader_t m_reader;

private:
    void load_next_timestamp()
    {
        if (m_ndx == 0)
        {
            m_timestamp = m_reader.read(64);

            return;
        }

        int64_t v = 0;

        if (m_reader.read() == 0)
        {
            v = 0;
        }
        else
        {
            if (m_reader.read() == 0)
            {
                v = m_reader.read(7);
                v -= 0x3f;
            }
            else
            {
                if (m_reader.read() == 0)
                {
                    v = m_reader.read(9);
                    v -= 0xff;
                }
                else
                {
                    if (m_reader.read() == 0)
                    {
                        v = m_reader.read(12);
                        v -= 0x7ff;
                    }
                    else
                    {
                        v = m_reader.read(32);
                        v -= 0x7fffffffU;
                    }
                }
            }
        }

        m_delta += v;
        m_timestamp += m_delta;
    }

    void load_next_value()
    {
        if (m_ndx == 0)
        {
            m_value = m_reader.read(64);

            return;
        }

        if (m_reader.read() == 0)
        {
            m_value_size = static_cast<uint32_t>(-1);
            m_ctz = static_cast<uint32_t>(-1);
        }
        else
        {
            if (m_reader.read() == 0)
            {
                assert(likely(m_value_size != static_cast<uint32_t>(-1)));
                assert(likely(m_ctz != static_cast<uint32_t>(-1)));
            }
            else
            {
                uint64_t value_ctrl = m_reader.read(9);

                m_value_size = value_ctrl & 0x1f;
                m_ctz = 16 - ((value_ctrl >> 5) + m_value_size);
            }

            m_value ^= m_reader.read(m_value_size << 2) << (m_ctz << 2);
        }
    }
};

}
