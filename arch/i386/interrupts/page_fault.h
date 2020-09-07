#ifndef ARCH_INTERRUPTS_PAGE_FAULT_H
#define ARCH_INTERRUPTS_PAGE_FAULT_H

#include "arch/i386/memory/paging.h"
#include "proc/process.h"

namespace arch {
namespace interrupts {

namespace {

using arch::memory::PAGE_SIZE;
using arch::memory::set_page_directory;
using arch::memory::swap_in_page;
using proc::get_currently_executing_process;
using proc::process;

uint32_t esi;
uint32_t edi;

} // namespace

constexpr uint32_t USER_PAGE_FAULT = 0x4;
uint32_t page_fault_addr;

__attribute__((interrupt)) void page_fault(struct interrupt_frame *frame,
                                           uint32_t error_code) {
  asm volatile("cli\n"
               "mov %%cr2, %0\n"
               "mov %%esi, %1\n"
               "mov %%edi, %2"
               : "=r"(page_fault_addr), "=m"(esi), "=m"(edi));

  if (error_code & USER_PAGE_FAULT) {
    set_page_directory(base_page_directory);

    struct process *current_process = get_currently_executing_process();
    uint32_t *page_dir = current_process->page_dir;

    if (!swap_in_page(current_process,
                      (void *)(page_fault_addr & (~(PAGE_SIZE - 1))))) {
      print_error((char *)"Segmentation Fault");
      kill_current_process();
    } else {
      set_page_directory(page_dir);
      asm volatile("mov %0, %%esi\n"
                   "mov %1, %%edi\n"
                   "sti"
                   :
                   : "m"(esi), "m"(edi));
    }
  } else {
    char buf[200];
    sprintnk(buf, 100, "Page fault! Error code: %x Address: %x", error_code,
             page_fault_addr);
    panic(buf);
  }
}

} // namespace interrupts
} // namespace arch

#endif
