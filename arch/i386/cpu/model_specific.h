#ifndef ARCH_I386_CPU_MODEL_SPECIFIC_H
#define ARCH_I386_CPU_MODEL_SPECIFIC_H

#include <stdint.h>

namespace arch {
namespace cpu {

constexpr uint32_t APIC_BASE_MSR = 0x1B;

struct cpu_msr {
	uint32_t low;
	uint32_t high;
};

struct cpu_msr get_msr(uint32_t msr);

void set_msr(uint32_t msr, struct cpu_msr value);

} // namespace cpu
} // namespace arch

#endif
