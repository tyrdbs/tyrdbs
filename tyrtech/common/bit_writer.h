#pragma once


#include <common/disallow_copy.h>
#include <common/branch_prediction.h>

#include <algorithm>
#include <cassert>
#include <cstdint>


namespace tyrtech {


template<typename WriterType>
class bit_writer : private disallow_copy
{
public:
    void write(uint64_t value, uint8_t bits)
    {
        assert(unlikely(bits <= 64));

        while (bits != 0)
        {
            if (m_offset == 64)
            {
                flush();
            }

            uint8_t part_bits = std::min(static_cast<uint8_t>(64 - m_offset), bits);
            uint64_t v = value >> (bits - part_bits);

            if (part_bits != 64)
            {
                v &= (1UL << part_bits) - 1;
            }

            m_bits |= v << (64 - (m_offset + part_bits));

            bits -= part_bits;
            m_offset += part_bits;
        }
    }

    void flush()
    {
        if (m_offset == 0)
        {
            return;
        }

        m_writer->write(m_bits);

        m_bits = 0;
        m_offset = 0;
    }

public:
    bit_writer(WriterType* writer)
      : m_writer(writer)
    {
    }

private:
    WriterType* m_writer{nullptr};

    uint64_t m_bits{0};
    uint8_t m_offset{0};
};

}
