#pragma once

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc32c_initialize();
uint32_t crc32c_update(uint32_t crc, const void* buf, size_t len);

#ifdef __cplusplus
}
#endif
