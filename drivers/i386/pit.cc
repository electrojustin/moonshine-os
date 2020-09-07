#include <stdint.h>

#include "arch/i386/cpu/save_restore.h"
#include "arch/i386/interrupts/idt.h"
#include "arch/i386/interrupts/pic.h"
#include "arch/interrupts/control.h"
#include "arch/interrupts/interrupts.h"
#include "drivers/i386/pit.h"
#include "io/i386/io.h"
#include "lib/math.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/time.h"
#include "proc/process.h"
#include "proc/sleep.h"

namespace drivers {

namespace {

using arch::interrupts::disable_interrupts;
using arch::interrupts::enable_interrupts;
using arch::interrupts::end_interrupt;
using arch::interrupts::INTERRUPT_GATE;
using arch::interrupts::pic_get_mask;
using arch::interrupts::pic_set_mask;
using arch::interrupts::register_interrupt_handler;
using io::out;
using lib::divide;
using lib::multiply;
using lib::std::kfree;
using lib::std::system_time;
using lib::std::tick;
using lib::std::time;
using proc::advance_process_queue;
using proc::execute_processes;
using proc::get_currently_executing_process;
using proc::process;
using proc::RUNNABLE;
using proc::SLEEP_WAIT;
using proc::sleep_wait;
using proc::WAITING;

// PIT Mode 3 can only handle even numbered periods;
uint16_t actual_period;
struct time tick_size;

uint8_t pit_irq_num;

constexpr uint8_t PIT_DATA_PORT = 0x40;
constexpr uint8_t PIT_COMMAND_PORT = 0x43;

extern "C" void pit_handler(char is_userspace) {
  end_interrupt(pit_irq_num);

  tick(tick_size);

  struct process *proc = get_currently_executing_process();
  struct process *current_proc = proc;
  do {
    if (current_proc->process_state == WAITING &&
        current_proc->wait->type == SLEEP_WAIT) {
      struct sleep_wait *wait = (struct sleep_wait *)current_proc->wait;
      if (wait->end_time.seconds < system_time.seconds ||
          (wait->end_time.seconds == system_time.seconds &&
           wait->end_time.nanoseconds < system_time.nanoseconds)) {
        current_proc->process_state = RUNNABLE;
        kfree(current_proc->wait);
      }
    }
    current_proc = current_proc->next;
  } while (current_proc != proc);

  if (is_userspace) {
    advance_process_queue();
    execute_processes();
  }
}

extern "C" void pit_interrupt(void);

SAVE_PROCESSOR_STATE(pit_interrupt, pit_handler)

} // namespace

void init_pit(uint8_t irq_num, uint16_t period) {
  pit_irq_num = irq_num;

  actual_period = period & 0xFFFE;
  uint64_t real_nanos = multiply((uint64_t)actual_period, 3);
  real_nanos = multiply(real_nanos, ((uint64_t)1000000000));
  real_nanos = divide(real_nanos, 3579545);
  tick_size.seconds = 0;
  tick_size.nanoseconds = real_nanos;

  // Set the mode:
  // Channel 0 (00)
  // Access mode high and low (11)
  // Square wave (011)
  // Binary mode (0)
  out(PIT_COMMAND_PORT, 0b00110110);

  // Set period
  out(PIT_DATA_PORT, actual_period & 0xFF);
  out(PIT_DATA_PORT, (actual_period >> 8) & 0xFF);

  // Register the interrupt
  register_interrupt_handler(irq_num, INTERRUPT_GATE, 0, (void *)pit_interrupt);

  // Unmask PIC interrupt 0
  uint16_t mask = pic_get_mask();
  mask = mask & 0xFFFE;
  pic_set_mask(mask);
}

} // namespace drivers
