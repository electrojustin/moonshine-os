#ifndef PROC_SYSCALL_H
#define PROC_SYSCALL_H

#include <stdint.h>

namespace proc {

void initialize_syscalls(uint32_t max_syscall_number);

void register_syscall(uint32_t number,
                      uint32_t (*syscall)(uint32_t, uint32_t, uint32_t,
                                          uint32_t, uint32_t, uint32_t));
} // namespace proc

#endif
