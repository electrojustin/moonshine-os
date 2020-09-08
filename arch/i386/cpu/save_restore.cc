#include "arch/i386/cpu/save_restore.h"
#include "arch/i386/memory/gdt.h"
#include "arch/i386/memory/paging.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace arch {
namespace cpu {

namespace {

using arch::memory::flush_tss;
using arch::memory::main_tss;
using proc::process;
using proc::tls_segment;

extern "C" uint32_t stack_top;

} // namespace

extern "C" __attribute__((cdecl)) void save_processor_state(uint32_t call_addr,
                                                            uint32_t esp) {
  uint32_t *page_dir = nullptr;
  char is_userspace = 1;
  uint32_t kernel_stack_top = 0;
  if (esp > (uint32_t)&stack_top) {
    arch::memory::set_page_directory(base_page_directory);

    struct process *current_process = proc::get_currently_executing_process();
    if (current_process) {
      current_process->esp = esp;
      page_dir = current_process->page_dir;
      kernel_stack_top = current_process->kernel_stack_top;
    }
  } else {
    is_userspace = 0;
    kernel_stack_top = (uint32_t)&stack_top;
  }

  void (*kernel_entry)(char) = (void (*)(char))call_addr;
  kernel_entry(is_userspace);

  // Just in case we pop back into userspace
  if (page_dir) {
    arch::memory::set_page_directory(page_dir);
    restore_processor_state(esp, kernel_stack_top);
  } else {
    restore_processor_state(esp, kernel_stack_top);
  }
}

void restore_processor_state(uint32_t esp, uint32_t kernel_stack_top) {
  main_tss.esp0 = kernel_stack_top;
  flush_tss();
  asm volatile("mov %0, %%esp\n"
               "mov is_sse_enabled, %%eax\n"
               "test %%eax, %%eax\n"
               "jz sse_disabled\n"
               "pop %%ecx\n"
               "fxrstor (%%esp)\n"
               "mov %%ecx, %%esp\n"
               "sse_disabled:\n"
               "popal\n"
               "pop %%ebp\n"
               "sti\n"
               "iret"
               :
               : "r"(esp));
}

} // namespace cpu
} // namespace arch
