#ifndef PROC_MMAP_H
#define PROC_MMAP_H

#include <stdint.h>

namespace proc {

uint32_t mmap(uint32_t req_addr, uint32_t len, uint32_t prot, uint32_t flags, uint32_t file_descriptor);

uint32_t munmap(uint32_t req_addr, uint32_t len, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

uint32_t mprotect(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4, uint32_t reserved5);

uint32_t msync(uint32_t req_addr, uint32_t len, uint32_t flags, uint32_t reserved1, uint32_t reserved2);

} // namespace proc

#endif
