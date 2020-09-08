#ifndef ARCH_I386_CPU_SAVE_RESTORE_H
#define ARCH_I386_CPU_SAVE_RESTORE_H

#include "proc/process.h"

namespace arch {
namespace cpu {

namespace {

using proc::process;

// Internal use only - do not touch.
extern "C" __attribute__((cdecl)) void save_process_state(uint32_t call_addr,
                                                          uint32_t esp);

} // namespace

// Restores the state of a suspended process given the stack pointer where it
// left off.
void restore_processor_state(uint32_t esp, uint32_t kernel_stack_top);

// Creates an ISR named "entry_name" that saves the processor state and calls
// "exit_name". Note that this macro screens context switches from the kernel
// and won't destroy their stacks.
#define SAVE_PROCESSOR_STATE(entry_name, exit_name)                            \
  asm("" #entry_name ":\n"                                                     \
      "cli\n"                                                                  \
      "pushal\n"                                                               \
      "mov is_sse_enabled, %eax\n"                                             \
      "test %eax, %eax\n"                                                      \
      "jz sse_disabled\n"                                                      \
      "mov %esp, %ecx\n"                                                       \
      "and $0xFFFFFFF0, %esp\n"                                                \
      "sub $0x200, %esp\n"                                                     \
      "fxsave (%esp)\n"                                                        \
      "push %ecx\n"                                                            \
      "sse_disabled:\n"                                                        \
      "mov %esp, %ecx\n"                                                       \
      "push %ecx\n"                                                            \
      "push $" #exit_name "\n"                                                 \
      "call save_processor_state\n");

} // namespace cpu
} // namespace arch

#endif
