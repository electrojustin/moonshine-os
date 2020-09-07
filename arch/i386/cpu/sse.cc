#include "arch/i386/cpu/sse.h"
#include "lib/std/stdio.h"

namespace arch {
namespace cpu {

char is_sse_enabled;

void maybe_enable_sse(void) {
  asm volatile("mov $0x1, %%eax\n"
               "cpuid\n"
               "shr $25, %%edx\n"
               "and $0x1, %%edx\n"
               "movb %%dl, is_sse_enabled"
               :
               :
               : "eax", "ebx", "ecx", "edx");

  lib::std::printk("Checking SSE...\n");

  if (is_sse_enabled) {
    asm volatile("mov %%cr0, %%eax\n"
                 "and $0xFFFB, %%ax\n"
                 "or $0x2, %%ax\n"
                 "mov %%eax, %%cr0\n"
                 "mov %%cr4, %%eax\n"
                 "or $0x600, %%ax\n"
                 "mov %%eax, %%cr4"
                 :
                 :
                 : "eax", "edx");
    lib::std::printk("SSE enabled!\n");
  }
}

} // namespace cpu
} // namespace arch
