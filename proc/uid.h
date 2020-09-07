#ifndef PROC_UID_H
#define PROC_UID_H

#include <stdint.h>

namespace proc {

uint32_t getuid(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                uint32_t reserved4, uint32_t reserved5);

uint32_t geteuid(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                 uint32_t reserved4, uint32_t reserved5);

uint32_t getgid(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                uint32_t reserved4, uint32_t reserved5);

uint32_t getegid(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                 uint32_t reserved4, uint32_t reserved5);

} // namespace proc

#endif
