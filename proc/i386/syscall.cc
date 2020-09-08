#include "proc/syscall.h"
#include "arch/i386/cpu/save_restore.h"
#include "arch/i386/cpu/sse.h"
#include "arch/i386/interrupts/idt.h"
#include "arch/i386/memory/paging.h"
#include "arch/interrupts/interrupts.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

extern char stack_top;

namespace proc {

namespace {

uint32_t num_syscalls;

typedef uint32_t (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                              uint32_t);

syscall_t *syscalls;

uint32_t default_syscall(uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi,
                         uint32_t edi, uint32_t ebp) {
  lib::std::panic("Invalid system call!");
}

using arch::cpu::is_sse_enabled;
using arch::interrupts::interrupt_frame;

constexpr uint8_t INTERRUPT_NUMBER = 0x80;

// The extern and the cdecl attribute guarantee that we'll be able to call this
// function in exactly the way we expect. Variables are pushed onto the stack
// and cleaned up by the caller.
extern "C" void syscall_dispatch(char is_userspace) {
  struct process *current_process = proc::get_currently_executing_process();
  uint32_t *esp = (uint32_t *)current_process->esp;
  uint32_t eax, ebx, ecx, edx, edi, esi, ebp;
  uint32_t *page_dir = current_process->page_dir;

  arch::memory::set_page_directory(page_dir);

  if (is_sse_enabled) {
    esp = (uint32_t *)(*esp);
  }

  // Pushal stores these registers in these locations. See processor docs for
  // details.
  edi = esp[0];
  esi = esp[1];
  ebx = esp[4];
  edx = esp[5];
  ecx = esp[6];
  eax = esp[7];
  ebp = esp[8];

  if (eax >= num_syscalls) {
    lib::std::panic("Invalid system call!");
  } else {
    // Execute the syscall
    arch::memory::set_page_directory(base_page_directory);
    eax = syscalls[eax](ebx, ecx, edx, esi, edi, ebp);

    // Set the return value
    // Notice how we use page_dir, not current_process->page_dir.
    // We have no guarantee what state the page tables will be in
    // after a syscall, so we just rely on local variables instead.
    arch::memory::set_page_directory(page_dir);
    esp[7] = eax;
    arch::memory::set_page_directory(base_page_directory);

    proc::execute_processes();
  }
}

extern "C" void syscall_interrupt(void);

SAVE_PROCESSOR_STATE(syscall_interrupt, syscall_dispatch)

} // namespace

void initialize_syscalls(uint32_t max_syscall_number) {
  num_syscalls = max_syscall_number + 1;
  syscalls = (syscall_t *)lib::std::kmalloc(num_syscalls * sizeof(syscall_t));

  for (int i = 0; i < num_syscalls; i++) {
    syscalls[i] = default_syscall;
  }

  arch::interrupts::register_interrupt_handler(INTERRUPT_NUMBER,
                                               arch::interrupts::INTERRUPT_GATE,
                                               3, (void *)syscall_interrupt);
}

void register_syscall(uint32_t number,
                      uint32_t (*syscall)(uint32_t, uint32_t, uint32_t,
                                          uint32_t, uint32_t, uint32_t)) {
  if (number >= num_syscalls) {
    lib::std::panic("Attempted to register an invalid system call!");
  }

  syscalls[number] = syscall;
}

} // namespace proc
