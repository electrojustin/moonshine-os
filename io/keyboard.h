#ifndef IO_KEYBOARD_H
#define IO_KEYBOARD_H

#include <stddef.h>
#include <stdint.h>

#include "proc/process.h"

namespace io {

using proc::wait_reason;

constexpr uint32_t KEYBOARD_WAIT = 0x1;

struct keyboard_wait : wait_reason {
  char *buf; // NOTE: This is in virtual address space
  uint32_t index;
  uint32_t len;
};

struct key_presses {
  size_t num_key_presses;
  uint8_t keycodes[128];
};

void key_press(uint8_t keycode);

void key_release(uint8_t keycode);

struct key_presses get_pressed_keys(void);

char get_ascii_mapping(uint8_t keycode);

char is_shift(uint8_t keycode);

} // namespace io

#endif
