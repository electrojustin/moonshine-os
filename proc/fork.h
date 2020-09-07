#ifndef PROC_FORK_H
#define PROC_FORK_H

#include <stdint.h>

namespace proc {

uint32_t fork(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
              uint32_t reserved4, uint32_t reserved5);

uint32_t clone(uint32_t flags, uint32_t stack_addr, uint32_t parent_tid_addr,
               uint32_t tls, uint32_t child_tid_addr);

} // namespace proc

#endif
