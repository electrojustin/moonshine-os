#ifndef PROC_ARCH_H
#define PROC_ARCH_H

#include <stdint.h>

namespace proc {

uint32_t arch_prctl(uint32_t code, uint32_t addr, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

} // namespace proc

#endif
