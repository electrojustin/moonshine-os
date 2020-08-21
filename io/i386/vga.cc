#include <stdint.h>

#include "io/vga.h"

namespace io {

namespace {

uint16_t* terminal_buffer;

constexpr uint32_t TERMINAL_BUFFER_ADDRESS = 0xB8000;

} // namespace

void vga_initialize(void) {
	uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*)TERMINAL_BUFFER_ADDRESS;
	for (uint8_t i = 0; i < VGA_HEIGHT; i++) {
		for (uint8_t j = 0; j < VGA_WIDTH; j++) {
			terminal_buffer[i*VGA_WIDTH + j] = vga_entry(' ', color);
		}
	}
}

void put_entry_at(char c, uint8_t color, uint8_t x, uint8_t y) {
	terminal_buffer[y*VGA_WIDTH + x] = vga_entry(c, color);
}

} // namespace io
