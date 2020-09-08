#include "proc/pid.h"
#include "proc/process.h"

namespace proc {

uint32_t getpid(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                uint32_t reserved4, uint32_t reserved5, uint32_t reserved6) {
  return get_currently_executing_process()->pid;
}

} // namespace proc
