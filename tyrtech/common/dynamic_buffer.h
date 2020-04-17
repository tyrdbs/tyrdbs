#pragma once


#include <common/disallow_copy.h>

#include <cstdint>


namespace tyrtech {


class dynamic_buffer : private disallow_copy
{
public:
    uint32_t size() const;
    char* data();
    const char* data() const;
    void set_size(uint32_t size);

public:
    dynamic_buffer() noexcept = default;
    dynamic_buffer(uint32_t size);
    ~dynamic_buffer();

public:
    dynamic_buffer(dynamic_buffer&& other) noexcept;
    dynamic_buffer& operator=(dynamic_buffer&& other) noexcept;

private:
    char* m_data{nullptr};
    uint32_t m_size{0};
};

}
