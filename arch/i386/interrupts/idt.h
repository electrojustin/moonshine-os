#ifndef ARCH_I386_INTERRUPTS_IDT_H
#define ARCH_I386_INTERRUPTS_IDT_H

#include <stdint.h>

namespace arch {
namespace interrupts {

struct __attribute__((__packed__)) idt_descriptor {
	uint16_t size;
	uint32_t addr;
};

struct __attribute__((__packed__)) idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero = 0;
	uint8_t flags;
	uint16_t offset_high;
};

struct __attribute__((__packed__)) interrupt_frame {
	uint16_t data_segment;
	uint32_t esp;
	uint32_t eflags;
	uint16_t code_segment;
	uint32_t return_address;
};

constexpr uint8_t INTERRUPT_GATE = 0xE;
constexpr uint8_t TRAP_GATE = 0xF;

} // namespace interrupts
} // namespace arch

#endif
