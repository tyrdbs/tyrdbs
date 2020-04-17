#pragma once


#include <common/disallow_copy.h>
#include <common/exception.h>


namespace tyrtech::io {


class channel : private disallow_copy
{
public:
    DEFINE_EXCEPTION(io_error, error);
    DEFINE_EXCEPTION(error, disconnected_error);
    DEFINE_EXCEPTION(error, timeout_error);
    DEFINE_EXCEPTION(error, unable_to_connect_error);
    DEFINE_EXCEPTION(error, address_in_use_error);
    DEFINE_EXCEPTION(error, address_not_found_error);

public:
    uint32_t recv(char* data, uint32_t size, uint64_t timeout);
    uint32_t send(const char* data, uint32_t size, uint64_t timeout);
    void send_all(const char* data, uint32_t size, uint64_t timeout);

    void disconnect();
    std::string_view uri() const;

public:
    static void initialize(uint32_t queue_size);

public:
    virtual std::shared_ptr<channel> accept() = 0;

public:
    virtual ~channel();

protected:
    int32_t m_fd{-1};

    char m_uri[128];
    std::string_view m_uri_view;

protected:
    channel(int32_t fd);

protected:
    void connect(const void* address, uint32_t address_size, uint64_t timeout);
    void listen(const void* address, uint32_t address_size);
    void accept(int32_t* fd, void* address, uint32_t address_size);
};

}
