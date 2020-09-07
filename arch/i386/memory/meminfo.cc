#include "arch/i386/memory/meminfo.h"
#include "lib/std/stdio.h"

namespace arch {
namespace memory {

namespace {

uint16_t call_bios_memory(uint16_t di) {
  uint16_t num_memory_entries;
  asm volatile("mov $0, %%edi\n"
               "mov %1, %%di\n"
               "mov $0, %%ebx\n"
               "loop:\n"
               "mov $0x534D4150, %%edx\n"
               "mov $0x0000E820, %%eax\n"
               "mov $24, %%ecx\n"
               "int $0x15\n"
               "add $20, %%di\n"
               "test %%ebx, %%ebx\n"
               "jnz loop\n"
               "mov %%cl, %0"
               : "=m"(num_memory_entries)
               : "r"(di));
  return num_memory_entries;
}

} // namespace

struct memory_info_entry memory_map[50];

struct memory_table detect_memory(void) {
  struct memory_info_entry *initial_memory_map =
      (struct memory_info_entry *)memory_map_initial_address;
  struct memory_table ret;
  ret.num_memory_entries =
      call_bios_memory((uint32_t)initial_memory_map & 0xFFFF);
  ret.entries = initial_memory_map;
  lib::std::printk("Detection: %x   %x\n", initial_memory_map[0].addr,
                   initial_memory_map[0].length);
  return ret;
}

} // namespace memory
} // namespace arch
