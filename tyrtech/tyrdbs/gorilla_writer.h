#pragma once


#include <common/bit_writer.h>


namespace tyrtech::tyrdbs {


template<typename BufferWriter>
class gorilla_writer : private disallow_copy
{
public:
    void write(uint64_t timestamp, uint64_t value)
    {
        write_timestamp(timestamp);
        write_value(value);

        m_samples++;
    }

    void flush()
    {
        m_writer.flush();
    }

    uint32_t samples() const
    {
        return m_samples;
    }

public:
    gorilla_writer(BufferWriter* writer)
      : m_writer(writer)
    {
    }

private:
    using writer_t =
            bit_writer<BufferWriter>;

private:
    uint32_t m_samples{0};

    uint64_t m_first_timestamp{0};
    uint64_t m_last_timestamp{0};

    int64_t m_last_delta{0};
    int64_t m_delta{0};

    uint64_t m_last_value{0};
    uint32_t m_last_clz{static_cast<uint32_t>(-1)};
    uint32_t m_last_ctz{static_cast<uint32_t>(-1)};

    writer_t m_writer;

private:
    void write_timestamp(uint64_t timestamp)
    {
        assert(likely(timestamp >= m_last_timestamp));

        if (m_samples == 0)
        {
            m_writer.write(timestamp, 64);
            m_first_timestamp = timestamp;
        }
        else
        {
            m_delta = timestamp - m_last_timestamp;

            if (m_samples == 1)
            {
                write_encoded(m_delta);
            }
            else
            {
                write_encoded(m_delta - m_last_delta);
            }

            m_last_delta = m_delta;
        }

        m_last_timestamp = timestamp;
    }

    void write_value(uint64_t value)
    {
        if (m_samples == 0)
        {
            m_writer.write(value, 64);
        }
        else
        {
            uint64_t xor_value = value ^ m_last_value;

            if (xor_value == 0)
            {
                m_writer.write(0, 1);

                m_last_clz = static_cast<uint32_t>(-1);
                m_last_ctz = static_cast<uint32_t>(-1);
            }
            else
            {
                uint32_t clz = __builtin_clzl(xor_value) >> 2;
                uint32_t ctz = __builtin_ctzl(xor_value) >> 2;

                if (m_last_clz <= clz && m_last_ctz <= ctz)
                {
                    uint32_t value_size = 16 - (m_last_clz + m_last_ctz);

                    xor_value >>= (m_last_ctz << 2);

                    m_writer.write(0x02, 2);
                    m_writer.write(xor_value, value_size << 2);
                }
                else
                {
                    uint32_t value_size = 16 - (clz + ctz);
                    uint32_t value_ctrl = 0;

                    xor_value >>= (ctz << 2);

                    value_ctrl |= 0x03 << 9;
                    value_ctrl |= clz << 5;
                    value_ctrl |= value_size;

                    m_writer.write(value_ctrl, 11);
                    m_writer.write(xor_value, value_size << 2);

                    m_last_clz = clz;
                    m_last_ctz = ctz;
                }
            }
        }

        m_last_value = value;
    }

    void write_encoded(int64_t value)
    {
        if (value == 0)
        {
            m_writer.write(0, 1);
        }
        else if (value >= -63 && value <= 64)
        {
            value += 0x3f;
            value &= 0x7f;
            value |= 0x02 << 7;

            m_writer.write(value, 9);
        }
        else if (value >= -255 && value <= 256)
        {
            value += 0x0ff;
            value &= 0x1ff;
            value |= 0x06 << 9;

            m_writer.write(value, 12);
        }
        else if (value >= -2047 && value <= 2048)
        {
            value += 0x7ff;
            value &= 0xfff;
            value |= 0x0e << 12;

            m_writer.write(value, 16);
        }
        else
        {
            assert((value & ~(0xffffffffU)) == 0);

            value += 0x7fffffffU;
            value &= 0xffffffffU;
            value |= 0x0fUL << 32;

            m_writer.write(value, 36);
        }
    }
};

}
