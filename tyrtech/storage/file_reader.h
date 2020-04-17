#pragma once


#include <storage/disk_reader.h>


namespace tyrtech::storage {


class file_reader : private disallow_copy
{
public:
    uint32_t pread(uint64_t offset, char* data, uint32_t size) const;

    const file_descriptor& descriptor() const;
    const extents_t& extents() const;

    uint32_t size() const;

    void unlink();

public:
    file_reader() = default;
    file_reader(disk_reader* reader, file_descriptor&& descriptor);

public:
    file_reader(file_reader&& other);
    file_reader& operator=(file_reader&& other);

private:
    disk_reader* m_reader{nullptr};
    file_descriptor m_descriptor;
};

}
