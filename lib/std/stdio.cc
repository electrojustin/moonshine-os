#include <stdarg.h>

#include "arch/i386/memory/paging.h"
#include "io/keyboard.h"
#include "io/vga.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace lib {
namespace std {

namespace {

using arch::memory::set_page_directory;
using arch::memory::virtual_to_physical;
using io::VGA_COLOR_LIGHT_GREY;
using io::VGA_COLOR_BLACK;
using io::VGA_HEIGHT;
using io::VGA_WIDTH;
using io::vga_color;
using io::vga_entry_color;
using io::vga_entry;
using io::put_entry_at;
using io::key_presses;
using io::get_pressed_keys;
using io::get_ascii_mapping;
using io::is_shift;
using proc::get_currently_executing_process;
using proc::process;

uint8_t terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

// The output_buffer is a ring buffer of rows. Each entry is 2 bytes: 1 for color, 1 for character.
// top_index points to the top of the page, bottom_index points to the bottom.
// All arithmetic done on them is modulo the height of the page.
// terminal_column is simply the column.
uint8_t terminal_column = 0;
uint16_t output_buffer[VGA_HEIGHT][VGA_WIDTH] = {0};
uint8_t top_index = 1;
uint8_t bottom_index = 0;

void refresh_terminal(void) {
	int i, j;
	for (i = 0; i < VGA_HEIGHT; i++) {
		for (j = 0; j < VGA_WIDTH; j++) {
			uint16_t output = output_buffer[(i + top_index) % VGA_HEIGHT][j];

			// Clear the end of the line, once we hit a null terminator.
			if (!(output & 0xFF)) {
				for (j; j < VGA_WIDTH; j++) {
					put_entry_at(' ', VGA_COLOR_BLACK, j, i);
				}
				break;
			}

			// Otherwise write the entry.
			put_entry_at(output & 0xFF, output >> 8, j, i);
		}
	}
}

void putc_internal(char c) {
	if (c == '\b') {
		if (terminal_column != 0) {
			terminal_column--;
		}
	}

	// The check for terminal_column == VGA_WIDTH enables wrapping.
	if (c == '\n' || terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		top_index = (top_index + 1) % VGA_HEIGHT;
		bottom_index = (bottom_index + 1) % VGA_HEIGHT;
		
	}
	if (c != '\n' && c != '\b') {
		output_buffer[bottom_index][terminal_column] = terminal_color << 8 | c;
		terminal_column++;
	}
	if (terminal_column != VGA_WIDTH) {
		output_buffer[bottom_index][terminal_column] = 0;
	}
}

int puts_internal(const char* s) {
	int index = 0;
	while(s[index]) {
		putc_internal(s[index]);
		index++;
	}
	return index;
}

} // namespace

void putc(char c) {
	putc_internal(c);
	refresh_terminal();
}

void puts(const char* s) {
	puts_internal(s);
	refresh_terminal();
}

void setcolor(uint8_t color) {
	terminal_color = color;
}

// Print formatted string.
// Formats supported:
// %c = character
// %s = string
// %d = decimal formatted integer
// %x = hex formatted integer (unsigned)
// %% = %
int printk(const char* format, ...) {
	char tmp_buf[512];
	va_list parameters;
	va_start(parameters, format);

	int written = vsprintnk(tmp_buf, 512, format, parameters);
	puts(tmp_buf);

	return written;
}

// Unroll stack and print to console.
void print_stack_trace(void* current_ebp) {
	set_page_directory(base_page_directory);

	struct process* current_process = proc::get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;

	original_ebp = nullptr;

	// Save ebp
	asm volatile(
		"mov %%ebp, %0"
		: "=r" (original_ebp));

	// x86 ABI says the last ebp is stored at the memory address the current one points to.
	// The stored eip is stored directly above ebp.
	// This prints the eip and uses ebp to unroll the stack.
	for (int i = 0; i < 10 && current_ebp != &stack_top && current_ebp; i++) {
		printk("From %x\n", *(((uint32_t*)current_ebp)+1));
		void* tentative_ebp_addr = virtual_to_physical(page_dir, current_ebp);
		if (tentative_ebp_addr) {
			current_ebp = *(void**)tentative_ebp_addr;
		} else {
			current_ebp = *(void**)current_ebp;
		}
	}

	// Restore the original ebp.
	asm volatile(
		"mov %0, %%ebp"
		:
		: "r" (original_ebp));
}

char getc(void) {
	struct key_presses keys = get_pressed_keys();
	char shift = 0;
	char c = 0;

	if (keys.num_key_presses) {
		for (int i = 0; i < keys.num_key_presses; i++) {
			if (is_shift(keys.keycodes[i])) {
				shift = 1;
			}
			if(!c) {
				c = get_ascii_mapping(keys.keycodes[i]);
			}
		}

		if (shift) {
			if ('a' <= c && c <= 'z') {
				return c - 0x20;
			} else if ('[' <= c && c <= ']') {
				return c + 0x20;
			} else if (c == ',' || c == '.') {
				return c + 0x10;
			}
		} else {
			return c;
		}
	}
	return 0;
}

} // namespace std
} // namespace lib
