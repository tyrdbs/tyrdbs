#pragma once


#include <sys/socket.h>
#include <sys/uio.h>

#include <cstdint>


namespace tyrtech::io {


void initialize(uint32_t queue_size);

int32_t pread(int32_t fd, void* buffer, uint32_t size, int64_t offset);
int32_t pwrite(int32_t fd, const void* buffer, uint32_t size, int64_t offset);

int32_t preadv(int32_t fd, iovec* iov, uint32_t size, int64_t offset);
int32_t pwritev(int32_t fd, iovec* iov, uint32_t size, int64_t offset);

int32_t send(int32_t fd, const char* buffer, uint32_t size, int32_t flags, uint64_t timeout);
int32_t recv(int32_t fd, char* buffer, uint32_t size, int32_t flags, uint64_t timeout);

int32_t accept(int32_t fd, sockaddr* address, uint32_t* address_size, uint64_t timeout);
int32_t connect(int32_t fd, const sockaddr* address, uint32_t address_size, uint64_t timeout);

int32_t allocate(int32_t fd, int32_t mode, uint64_t offset, uint64_t size);
int32_t close(int32_t fd);

int32_t sync(int32_t fd, uint32_t flags);

int32_t nop();

}
