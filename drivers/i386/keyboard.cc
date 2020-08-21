#include <stdint.h>

#include "drivers/i386/keyboard.h"
#include "arch/interrupts/interrupts.h"
#include "arch/i386/interrupts/idt.h"
#include "arch/i386/interrupts/pic.h"
#include "io/i386/io.h"
#include "io/keyboard.h"

namespace drivers {

namespace {

using arch::interrupts::interrupt_frame;
using arch::interrupts::pic_get_mask;
using arch::interrupts::pic_set_mask;
using io::in;
using io::out;

uint8_t keyboard_irq_num;

constexpr uint8_t KEYBOARD_COMMAND_PORT = 0x64;
constexpr uint8_t KEYBOARD_DATA_PORT = 0x60;
constexpr uint8_t KEYBOARD_DISABLE_ONE = 0xAD;
constexpr uint8_t KEYBOARD_DISABLE_TWO = 0xA7;
constexpr uint8_t KEYBOARD_ENABLE_ONE = 0xAE;
constexpr uint8_t KEYBOARD_ENABLE_TWO = 0xA8;
constexpr uint8_t KEYBOARD_ENABLE_INTERRUPT_ONE = 0x1;
constexpr uint8_t KEYBOARD_ENABLE_INTERRUPT_TWO = 0x2;
constexpr uint16_t KEYBOARD_INTERRUPT_MASK = ~0x2;

__attribute__((interrupt)) void keyboard_interrupt(struct interrupt_frame* frame) {
	uint8_t keycode = in(KEYBOARD_DATA_PORT);
	if (keycode > 0x58) {
		io::key_release(keycode - 0x80);
	} else {
		io::key_press(keycode);
	}
	arch::interrupts::end_interrupt(keyboard_irq_num);
}

} // namespace

void init_keyboard(uint8_t irq_num) {
	keyboard_irq_num = irq_num;

	arch::interrupts::register_interrupt_handler(keyboard_irq_num, arch::interrupts::INTERRUPT_GATE, 0, (void*)keyboard_interrupt);

	// Disable keyboard
	out(KEYBOARD_COMMAND_PORT, KEYBOARD_DISABLE_ONE);
	out(KEYBOARD_COMMAND_PORT, KEYBOARD_DISABLE_TWO);

	// Flush keyboard buffer
	in(KEYBOARD_DATA_PORT);

	// Enable keyboard
	out(KEYBOARD_COMMAND_PORT, KEYBOARD_ENABLE_ONE);
	out(KEYBOARD_COMMAND_PORT, KEYBOARD_ENABLE_TWO);

	// Enable keyboard controller interrupts
	out(KEYBOARD_DATA_PORT, KEYBOARD_ENABLE_INTERRUPT_ONE | KEYBOARD_ENABLE_INTERRUPT_TWO);

	// Unmask keyboard IRQ
	pic_set_mask(pic_get_mask() & KEYBOARD_INTERRUPT_MASK);
}

} // namespace drivers
