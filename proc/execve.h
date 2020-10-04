#ifndef PROC_EXECVE_H
#define PROC_EXECVE_H

#include <stdint.h>

namespace proc {

uint32_t execve(uint32_t path_addr, uint32_t argv_addr, uint32_t envp_addr,
                uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

} // namespace proc

#endif
