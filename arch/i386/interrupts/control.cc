#include "arch/interrupts/control.h"
#include "arch/i386/interrupts/pic.h"

namespace arch {
namespace interrupts {

void enable_interrupts(void) { asm volatile("sti"); }

void disable_interrupts(void) { asm volatile("cli"); }

} // namespace interrupts
} // namespace arch
