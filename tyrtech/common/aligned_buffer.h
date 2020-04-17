#pragma once


#include <common/disallow_copy.h>

#include <cstdint>


namespace tyrtech {


class aligned_buffer : private disallow_copy
{
public:
    uint32_t size() const;
    char* data();
    const char* data() const;

public:
    aligned_buffer(uint32_t alignment, uint32_t size);
    ~aligned_buffer();

public:
    aligned_buffer(aligned_buffer&& other) noexcept;
    aligned_buffer& operator=(aligned_buffer&& other) noexcept;

private:
    uint32_t m_alignment{0};
    char* m_data{nullptr};
    uint32_t m_size{0};
};

}
