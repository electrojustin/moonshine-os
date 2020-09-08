#ifndef PROC_OPEN_H
#define PROC_OPEN_H

#include <stdint.h>

namespace proc {

uint32_t open(uint32_t path_addr, uint32_t flags, uint32_t mode,
              uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

uint32_t openat(uint32_t directory_fd, uint32_t path_addr, uint32_t flags,
                uint32_t mode, uint32_t reserved1, uint32_t reserved2);

uint32_t access(uint32_t path_addr, uint32_t mode, uint32_t reserved1,
                uint32_t reserved2, uint32_t reserved3, uint32_t reserved4);

} // namespace proc

#endif
