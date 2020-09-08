#include "proc/arch.h"

namespace proc {

uint32_t arch_prctl(uint32_t code, uint32_t addr, uint32_t reserved1,
                    uint32_t reserved2, uint32_t reserved3,
                    uint32_t reserved4) {
  return 0;
}

} // namespace proc
