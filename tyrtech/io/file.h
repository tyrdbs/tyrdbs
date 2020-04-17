#pragma once


#include <common/disallow_copy.h>
#include <common/exception.h>

#include <sys/file.h>
#include <sys/stat.h>


namespace tyrtech::io {


class file : private disallow_copy
{
public:
    DEFINE_EXCEPTION(io_error, error);

public:
    enum class access
    {
        read,
        write,
        read_write
    };

public:
    template<typename... Arguments>
    static file create(Arguments&&... arguments)
    {
        int32_t flags = O_LARGEFILE | O_CREAT | O_EXCL | O_CLOEXEC | O_RDWR;
        int32_t mode =  S_IRUSR | S_IWUSR | S_IRGRP;

        file f(std::forward<Arguments>(arguments)...);
        f.open(flags, mode);

        return f;
    }

    template<typename... Arguments>
    static file open(access access, Arguments&&... arguments)
    {
        int32_t flags = O_LARGEFILE | O_CLOEXEC;
        int32_t mode = 0;

        switch (access)
        {
            case access::read:
            {
                flags |= O_RDONLY;

                break;
            }
            case access::write:
            {
                flags |= O_WRONLY;

                break;
            }
            case access::read_write:
            {
                flags |= O_RDWR;

                break;
            }
        }

        file f(std::forward<Arguments>(arguments)...);
        f.open(flags, mode);

        return f;
    }

    template<typename... Arguments>
    static file mktemp(Arguments&&... arguments)
    {
        int32_t flags = O_LARGEFILE | O_CLOEXEC;

        file f(std::forward<Arguments>(arguments)...);
        f.mkostemp(flags);

        return f;
    }

public:
    void pread(uint64_t offset, char* data, uint32_t size);
    void pwrite(uint64_t offset, const char* data, uint32_t size);

    uint32_t preadv(uint64_t offset, iovec* iov, uint32_t size);
    uint32_t pwritev(uint64_t offset, iovec* iov, uint32_t size);

    void allocate(int32_t mode, uint64_t offset, uint64_t size);
    struct stat64 stat();

    bool try_lock();

    void unlink();

    std::string_view path() const;
    int32_t fd() const;

public:
    static void initialize(uint32_t queue_size);

public:
    file() = default;
    ~file();

public:
    file(file&& other);
    file& operator=(file&& other);

private:
    int32_t m_fd{-1};

    char m_path[128];
    std::string_view m_path_view;

private:
    template<typename... Arguments>
    file(Arguments&&... arguments)
      : m_path_view(format_to(m_path, sizeof(m_path),
                              std::forward<Arguments>(arguments)...))
    {
    }

private:
    void open(int32_t flags, int32_t mode);
    void mkostemp(int32_t flags);
};


}
