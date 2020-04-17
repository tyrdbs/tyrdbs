#pragma once


#include <storage/disk_writer.h>


namespace tyrtech::storage {


class file_writer : private disallow_copy
{
public:
    DEFINE_EXCEPTION(disk_writer::error, error);

public:
    static constexpr uint64_t max_file_size{1UL << (page_bits + file_pages_bits)};

public:
    template<typename T>
    void write(const T& data)
    {
        write(reinterpret_cast<const char*>(&data), sizeof(T));
    }

    void write(const char* data, uint32_t size);

    void add_padding();
    void flush();

    file_descriptor commit();

    uint32_t size() const;

public:
    file_writer() = default;
    file_writer(disk_writer* writer);

    ~file_writer();

public:
    file_writer(file_writer&& other);
    file_writer& operator=(file_writer&& other);

private:
    disk_writer* m_writer{nullptr};

    disk_writer::state* m_state;

    bool m_commited{false};
    bool m_flushed{false};
};

}
