#include <stddef.h>
#include <stdint.h>

#include "io/i386/io.h"

namespace io {

void out(uint16_t port, uint8_t value) {
	asm volatile (
		"movb %0, %%al\n"
		"mov %1, %%dx\n"
		"outb %%al, %%dx"
		:
		: "r" (value), "r" (port)
		: "al", "dx");
}

void out16(uint16_t port, uint16_t value) {
	asm volatile (
		"mov %0, %%ax\n"
		"mov %1, %%dx\n"
		"outw %%ax, %%dx"
		:
		: "r" (value), "r" (port)
		: "ax", "dx");
}

void out32(uint16_t port, uint32_t value) {
	asm volatile (
		"mov %0, %%eax\n"
		"mov %1, %%dx\n"
		"outl %%eax, %%dx"
		:
		: "r" (value), "r" (port)
		: "eax", "dx");
}

void out_block(uint16_t port, uint8_t* value, size_t size) {
	asm volatile (
		"mov %0, %%esi\n"
		"mov %1, %%dx\n"
		"mov %2, %%ecx\n"
		"rep outsb"
		:
		: "r" (value), "r" (port), "r" (size)
		: "esi", "dx", "cx");
}

void out_block16(uint16_t port, uint16_t* value, size_t size) {
	asm volatile (
		"mov %0, %%esi\n"
		"mov %1, %%dx\n"
		"mov %2, %%ecx\n"
		"rep outsw"
		:
		: "r" (value), "r" (port), "r" (size)
		: "esi", "dx", "cx");
}

uint8_t in(uint16_t port) {
	uint8_t ret;
	asm volatile (
		"mov %1, %%dx\n"
		"inb %%dx, %%al\n"
		"movb %%al, %0"
		: "=r" (ret)
		: "r" (port)
		: "al", "dx");
	return ret;
}

uint16_t in16(uint16_t port) {
	uint16_t ret;
	asm volatile (
		"mov %1, %%dx\n"
		"inw %%dx, %%ax\n"
		"mov %%ax, %0"
		: "=r" (ret)
		: "r" (port)
		: "ax", "dx");
	return ret;
}

uint32_t in32(uint16_t port) {
	uint32_t ret;
	asm volatile (
		"mov %1, %%dx\n"
		"inl %%dx, %%eax\n"
		"mov %%eax, %0"
		: "=r" (ret)
		: "r" (port)
		: "eax", "dx");
	return ret;
}

void in_block(uint16_t port, uint8_t* buffer, size_t size) {
	asm volatile (
		"mov %0, %%edi\n"
		"mov %1, %%dx\n"
		"mov %2, %%ecx\n"
		"rep insb"
		:
		: "r" (buffer), "r" (port), "r" (size)
		: "edi", "dx", "cx");
}

void in_block16(uint16_t port, uint16_t* buffer, size_t size) {
	asm volatile (
		"mov %0, %%edi\n"
		"mov %1, %%dx\n"
		"mov %2, %%ecx\n"
		"rep insw"
		:
		: "r" (buffer), "r" (port), "r" (size)
		: "edi", "dx", "cx");
}

} // namespace io

