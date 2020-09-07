#include "io/keyboard.h"
#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"

namespace io {

namespace {

using arch::memory::virtual_to_physical;
using lib::std::getc;
using lib::std::kfree;
using lib::std::putc;
using proc::get_currently_executing_process;
using proc::process;
using proc::RUNNABLE;
using proc::WAITING;

volatile uint8_t pressed_keys[128] = {0};

const char ascii_keycodes_translation[128] = {
    0,   0,    '1',  '2', '3',  '4', '5', '6', '7', '8', '9', '0', '-',
    '=', '\b', '\t', 'q', 'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    '[', ']',  '\n', 0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',',
    '.', '/',  0,    '*', 0,    ' ', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   '7', '8', '9', '-', '4', '5', '6',
    '+', '1',  '2',  '3', '0',  '.', 0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,    0,    0,   0,    0,   0,   0,   0,   0,   0};

} // namespace

void key_press(uint8_t keycode) {
  pressed_keys[keycode] = 1;

  // Find out if someone's waiting on this keypress
  struct process *proc = get_currently_executing_process();
  if (proc) {
    struct process *current_proc = proc;
    do {
      if (current_proc->process_state == WAITING &&
          current_proc->wait->type == KEYBOARD_WAIT) {
        struct keyboard_wait *wait = (struct keyboard_wait *)current_proc->wait;
        uint32_t *page_dir = current_proc->page_dir;
        while (wait->index < wait->len) {
          char c = getc();
          if (c) {
            // Echo keystroke
            putc(c);

            *(char *)virtual_to_physical(page_dir, wait->buf + wait->index) = c;
            wait->index++;

            if (c == '\n') {
              break;
            }
          } else {
            return;
          }
        }
        current_proc->process_state = RUNNABLE;
        kfree(current_proc->wait);
        current_proc->wait = nullptr;
        return;
      }

      current_proc = current_proc->next;
    } while (current_proc != proc);
  }
}

void key_release(uint8_t keycode) { pressed_keys[keycode] = 0; }

struct key_presses get_pressed_keys(void) {
  struct key_presses ret;

  int keycodes_index = 0;
  for (int i = 0; i < 128; i++) {
    if (pressed_keys[i]) {
      ret.keycodes[keycodes_index] = i;
      keycodes_index++;
      if (!is_shift(i)) {
        pressed_keys[i] = 0;
      }
    }
  }
  ret.num_key_presses = keycodes_index;

  return ret;
}

char get_ascii_mapping(uint8_t keycode) {
  if (keycode > 128) {
    return 0;
  }

  return ascii_keycodes_translation[keycode];
}

char is_shift(uint8_t keycode) {
  if (keycode == 0x2A || keycode == 0x36) {
    return 1;
  } else {
    return 0;
  }
}

} // namespace io
