#ifndef ARCH_I386_INTERRUPTS_ERROR_INTERRUPTS_H
#define ARCH_I386_INTERRUPTS_ERROR_INTERRUPTS_H

#include "arch/i386/interrupts/idt.h"
#include "arch/i386/memory/gdt.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace arch {
namespace interrupts {

namespace {
	using arch::memory::USER_CODE_SELECTOR;
	using lib::std::panic;
	using lib::std::print_error;
	using lib::std::sprintnk;
	using proc::kill_current_process;

	static inline void handle_fault(struct interrupt_frame* frame, char* message) {
		if (frame->code_segment == USER_CODE_SELECTOR) {
			print_error(message);
			kill_current_process();
		} else {
			panic(message);
		}
	}
	
}

__attribute__((interrupt)) void divide_by_zero(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"Divide by zero!");
}

__attribute__((interrupt)) void overflow(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"Overflow!");
}

__attribute__((interrupt)) void range_exceeded(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"Range exceeded!");
}

__attribute__((interrupt)) void invalid_opcode(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"Invalid opcode!");
}

__attribute__((interrupt)) void device_not_available(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"Device not available!");
}

__attribute__((interrupt)) void double_fault(struct interrupt_frame* frame, uint32_t error_code) {
	panic((char*)"Double fault!");
}

__attribute__((interrupt)) void invalid_tss(struct interrupt_frame* frame, uint32_t error_code) {
	panic((char*)"Invalid TSS!");
}

__attribute__((interrupt)) void floating_point_exception(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"Floating point exception!");
}

__attribute__((interrupt)) void general_protection_fault(struct interrupt_frame* frame, uint32_t error_code) {
	handle_fault(frame, (char*)"General protection fault!");
}

constexpr uint32_t USER_PAGE_FAULT = 0x4;

__attribute__((interrupt)) void page_fault(struct interrupt_frame* frame, uint32_t error_code) {
	asm volatile("cli");
	if (error_code & USER_PAGE_FAULT) {
		print_error((char*)"Segmentation Fault");
		kill_current_process();
	} else {
		char buf[100];
		sprintnk(buf, 100, "Page fault! Error code: %x", error_code);
		panic(buf);
	}
}

__attribute__((interrupt)) void simd_exception(struct interrupt_frame* frame) {
	handle_fault(frame, (char*)"SIMD exception!");
}

} // namespace interrupts
} // namespace arch

#endif
