#include <stddef.h>
#include <stdint.h>

#include "arch/i386/interrupts/error_interrupts.h"
#include "arch/i386/interrupts/idt.h"
#include "arch/i386/interrupts/page_fault.h"
#include "arch/i386/interrupts/pic.h"
#include "arch/i386/memory/gdt.h"
#include "arch/interrupts/interrupts.h"
#include "lib/std/stdio.h"

namespace arch {
namespace interrupts {

namespace {
constexpr size_t NUM_INTERRUPTS = 256;
struct idt_descriptor descriptor;
struct idt_entry idt[NUM_INTERRUPTS];

__attribute__((interrupt)) void
unregistered_interrupt(struct interrupt_frame *frame) {
  lib::std::printk((char *)"Unregistered interrupt\n");

  // We don't actually know the interrupt number, but this is just to figure out
  // which PIC we should send the end of interrupt command to. We'll just assume
  // we need to send it to both of them.
  arch::interrupts::end_interrupt(0xFF);
}

void register_interrupt_handler_internal(uint8_t interrupt_number, uint8_t type,
                                         uint8_t privilege,
                                         void *interrupt_handler) {
  idt[interrupt_number].offset_low = (uint32_t)interrupt_handler & 0xFFFF;
  idt[interrupt_number].offset_high = (uint32_t)interrupt_handler >> 16;
  idt[interrupt_number].flags =
      0b10000000 | type |
      (privilege
       << 5); // The highest bit means "enabled" so it should always be set.
  idt[interrupt_number].selector = arch::memory::CODE_SELECTOR;
}

void register_error_interrupts(void) {
  register_interrupt_handler_internal(0, TRAP_GATE, 0, (void *)divide_by_zero);
  register_interrupt_handler_internal(4, TRAP_GATE, 0, (void *)overflow);
  register_interrupt_handler_internal(5, TRAP_GATE, 0, (void *)range_exceeded);
  register_interrupt_handler_internal(6, TRAP_GATE, 0, (void *)invalid_opcode);
  register_interrupt_handler_internal(7, TRAP_GATE, 0,
                                      (void *)device_not_available);
  register_interrupt_handler_internal(8, TRAP_GATE, 0, (void *)double_fault);
  register_interrupt_handler_internal(10, TRAP_GATE, 0, (void *)invalid_tss);
  register_interrupt_handler_internal(13, TRAP_GATE, 0,
                                      (void *)general_protection_fault);
  register_interrupt_handler_internal(14, TRAP_GATE, 0, (void *)page_fault);
  register_interrupt_handler_internal(16, TRAP_GATE, 0,
                                      (void *)floating_point_exception);
  register_interrupt_handler_internal(19, TRAP_GATE, 0, (void *)simd_exception);
}

void flush_idt(void) {
  void *descriptor_address = &descriptor;

  // Loads the interrupt table.
  asm volatile("lidt (%0)" : : "r"(descriptor_address));
}
} // namespace

void register_interrupt_handler(uint8_t interrupt_number, uint8_t type,
                                uint8_t privilege, void *interrupt_handler) {
  register_interrupt_handler_internal(interrupt_number, type, privilege,
                                      interrupt_handler);
  flush_idt();
}

void initialize_interrupts(void) {
  descriptor.size = NUM_INTERRUPTS * sizeof(idt_entry) - 1;
  descriptor.addr = (uint32_t)idt;

  for (int i = 0; i < NUM_INTERRUPTS; i++) {
    register_interrupt_handler_internal(i, INTERRUPT_GATE, 0,
                                        (void *)unregistered_interrupt);
  }

  register_error_interrupts();

  flush_idt();
}

} // namespace interrupts
} // namespace arch
