#ifndef ARCH_I386_INTERRUPTS_ERROR_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_ERROR_INTERRUPTS_H

#include "lib/std/stdio.h"

namespace arch {
namespace interrupts {

namespace {
	using lib::std::panic;
}

__attribute__((interrupt)) void divide_by_zero(struct interrupt_frame* frame) {
	panic((char*)"Divide by zero!");
}

__attribute__((interrupt)) void overflow(struct interrupt_frame* frame) {
	panic((char*)"Overflow!");
}

__attribute__((interrupt)) void range_exceeded(struct interrupt_frame* frame) {
	panic((char*)"Range exceeded!");
}

__attribute__((interrupt)) void invalid_opcode(struct interrupt_frame* frame) {
	panic((char*)"Invalid opcode!");
}

__attribute__((interrupt)) void device_not_available(struct interrupt_frame* frame) {
	panic((char*)"Device not available!");
}

__attribute__((interrupt)) void double_fault(struct interrupt_frame* frame, uint32_t error_code) {
	panic((char*)"Double fault!");
}

__attribute__((interrupt)) void invalid_tss(struct interrupt_frame* frame, uint32_t error_code) {
	panic((char*)"Invalid TSS!");
}

__attribute__((interrupt)) void floating_point_exception(struct interrupt_frame* frame) {
	panic((char*)"Floating point exception!");
}

__attribute__((interrupt)) void general_protection_fault(struct interrupt_frame* frame, uint32_t error_code) {
	panic((char*)"General protection fault!");
}

__attribute__((interrupt)) void page_fault(struct interrupt_frame* frame) {
	panic((char*)"Page fault!");
}

__attribute__((interrupt)) void simd_exception(struct interrupt_frame* frame) {
	panic((char*)"SIMD exception!");
}

} // namespace interrupts
} // namespace arch

#endif
