#pragma once


#include <io/file.h>


namespace tyrtech::io {


class file_writer : private disallow_copy
{
public:
    DEFINE_EXCEPTION(file::error, error);

public:
    uint32_t write(const char* data, uint32_t size);

    uint64_t offset() const;
    void set_offset_to(uint64_t offset);

public:
    file_writer(file* file, uint64_t offset);

private:
    file* m_file{nullptr};
    uint64_t m_offset{0};
};

}
