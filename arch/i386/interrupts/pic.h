#ifndef ARCH_I386_INTERRUPTS_PIC_H
#define ARCH_I386_INTERRUPTS_PIC_H

#include <stdint.h>

namespace arch {
namespace interrupts {

void pic_initialize(uint8_t starting_interrupt);

void pic_set_mask(uint16_t mask);

uint16_t pic_get_mask(void);

void end_interrupt(uint8_t irq);

} // namespace interrupts
} // namespace arch

#endif
