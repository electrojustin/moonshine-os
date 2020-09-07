#ifndef PROC_BRK_H
#define PROC_BRK_H

#include <stdint.h>

namespace proc {

uint32_t brk(uint32_t new_brk, uint32_t reserved1, uint32_t reserved2,
             uint32_t reserved3, uint32_t reserved4);

} // namespace proc

#endif
