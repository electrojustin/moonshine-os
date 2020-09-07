#include "arch/i386/interrupts/apic.h"
#include "arch/i386/cpu/model_specific.h"

namespace arch {
namespace interrupts {

namespace {

using arch::cpu::cpu_msr;

} // namespace

void disable_apic(void) {
  struct cpu_msr value = arch::cpu::get_msr(arch::cpu::APIC_BASE_MSR);
  value.low = value.low & 0xFFFFF7FF;
  arch::cpu::set_msr(arch::cpu::APIC_BASE_MSR, value);
}

} // namespace interrupts
} // namespace arch
