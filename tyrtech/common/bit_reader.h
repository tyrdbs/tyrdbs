#pragma once


#include <common/disallow_copy.h>
#include <common/branch_prediction.h>

#include <algorithm>
#include <cassert>
#include <cstdint>


namespace tyrtech {


template<typename ReaderType>
class bit_reader : private disallow_copy
{
public:
    uint64_t read(uint8_t bits)
    {
        assert(unlikely(bits <= 64));
        uint64_t value = 0;

        while (bits != 0)
        {
            if (unlikely(m_offset == 64))
            {
                load();
            }

            uint8_t part_bits = std::min(static_cast<uint8_t>(64 - m_offset), bits);
            uint64_t v = m_bits >> (64 - (m_offset + part_bits));

            if (part_bits != 64)
            {
                v &= (1UL << part_bits) - 1;
            }

            value |= v << (bits - part_bits);

            bits -= part_bits;
            m_offset += part_bits;
        }

        return value;
    }

    uint64_t read()
    {
        if (unlikely(m_offset == 64))
        {
            load();
        }

        return (m_bits >> (63 - m_offset++)) & 0x01;
    }

public:
    bit_reader(ReaderType* reader)
      : m_reader(reader)
    {
    }

private:
    ReaderType* m_reader{nullptr};

    uint64_t m_bits{0};
    uint8_t m_offset{64};

private:
    void load()
    {
        m_reader->read(reinterpret_cast<char*>(&m_bits), sizeof(m_bits));
        m_offset = 0;
    }
};

}
