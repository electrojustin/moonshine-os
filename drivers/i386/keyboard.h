#ifndef DRIVERS_I386_KEYBOARD_H
#define DRIVERS_I386_KEYBOARD_H

#include <stdint.h>

namespace drivers {

void init_keyboard(uint8_t irq_num);

} // namespace drivers

#endif
