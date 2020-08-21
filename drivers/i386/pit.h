#ifndef DRIVERS_I386_PIT_H
#define DRIVERS_I386_PIT_H

#include <stdint.h>

namespace drivers {

void init_pit(uint8_t irq_num, uint16_t period);

} // namespace drivers

#endif
