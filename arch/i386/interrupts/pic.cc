#include "arch/i386/interrupts/pic.h"
#include "io/i386/io.h"

#include <stdint.h>

namespace arch {
namespace interrupts {

namespace {

	using io::in;
	using io::out;

	// Command address of the master PIC
	constexpr uint8_t MASTER_COMMAND_PORT = 0x20;
	// Data address of the master PIC
	constexpr uint8_t MASTER_DATA_PORT = 0x21;
	// Command address of the slave PIC
	constexpr uint8_t SLAVE_COMMAND_PORT = 0xA0;
	// Data address of the slave PIC
	constexpr uint8_t SLAVE_DATA_PORT = 0xA1;

	// Indicates the following command is four bytes long
	constexpr uint8_t FOUR_BYTE_COMMAND = 0x01;
	// Indicates single PIC mode instead of cascade
	constexpr uint8_t SINGLE_PIC_MODE = 0x02;
	// Indicates level triggered mode instead of edge
	constexpr uint8_t LEVEL_TRIGGERED = 0x08;
	// Initializes the PIC
	constexpr uint8_t INIT_FLAG = 0x10;
	// Indicates that we're operating in 8086 mode
	constexpr uint8_t MODE_8086 = 0x01;

	// End the interrupt
	constexpr uint8_t END_INTERRUPT = 0x20;	

	uint8_t pic_interrupts = 0;

} // namespace

void pic_initialize(uint8_t starting_interrupt) {
	uint16_t mask = pic_get_mask();

	out(MASTER_COMMAND_PORT, INIT_FLAG | FOUR_BYTE_COMMAND);
	out(SLAVE_COMMAND_PORT, INIT_FLAG | FOUR_BYTE_COMMAND);
	out(MASTER_DATA_PORT, starting_interrupt);
	out(SLAVE_DATA_PORT, starting_interrupt+8);
	out(MASTER_DATA_PORT, 4);
	out(SLAVE_DATA_PORT, 2);
	out(MASTER_DATA_PORT, MODE_8086);
	out(SLAVE_DATA_PORT, MODE_8086);

	pic_set_mask(mask);

	pic_interrupts = starting_interrupt;
}

void pic_set_mask(uint16_t mask) {
	out(MASTER_DATA_PORT, mask & 0xFF);
	out(SLAVE_DATA_PORT, mask >> 8);
}

uint16_t pic_get_mask(void) {
	uint16_t ret;
	ret = (uint16_t)in(SLAVE_DATA_PORT) << 8;
	ret |= in(MASTER_DATA_PORT);
	return ret;
}

void end_interrupt(uint8_t irq) {
	if (irq - pic_interrupts > 7) {
		out(SLAVE_COMMAND_PORT, END_INTERRUPT);
	}

	out(MASTER_COMMAND_PORT, END_INTERRUPT);
}

} // namespace interrupts
} // namespace arch
