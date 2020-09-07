#ifndef ARCH_INTERRUPTS_INTERRUPTS_H
#define ARCH_INTERRUPTS_INTERRUPTS_H

#include <stdint.h>

namespace arch {
namespace interrupts {

// Register an interrupt handler.
void register_interrupt_handler(uint8_t interrupt_number, uint8_t type,
                                uint8_t privilege, void *interrupt_handler);

// Initialize IDT and some exception handling interrupts.
void initialize_interrupts(void);

} // namespace interrupts
} // namespace arch

#endif
