#include <stdint.h>

#include "arch/i386/cpu/model_specific.h"

namespace arch {
namespace cpu {

struct cpu_msr get_msr(uint32_t msr) {
  struct cpu_msr ret;
  asm volatile("mov %2, %%ecx\n"
               "rdmsr\n"
               "mov %%eax, %0\n"
               "mov %%edx, %1"
               : "=r"(ret.low), "=r"(ret.high)
               : "r"(msr));
  return ret;
}

void set_msr(uint32_t msr, struct cpu_msr value) {
  asm volatile("mov %0, %%ecx\n"
               "mov %1, %%eax\n"
               "mov %2, %%edx\n"
               "wrmsr"
               :
               : "m"(msr), "m"(value.low), "m"(value.high));
}

} // namespace cpu
} // namespace arch
