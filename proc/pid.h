#ifndef PROC_PID_H
#define PROC_PID_H

#include <stdint.h>

namespace proc {

uint32_t getpid(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                uint32_t reserved4, uint32_t reserved5, uint32_t reserved6);

} // namespace proc

#endif
