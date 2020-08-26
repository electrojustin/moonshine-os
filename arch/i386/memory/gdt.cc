#include "arch/i386/memory/gdt.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"

namespace arch {
namespace memory {

namespace {

using lib::std::memset;

struct gdt_descriptor descriptor;

struct gdt_entry gdt_table[GDT_TABLE_SIZE];

// Loads GDT.
// Also sets code segment (using jmp) and the data segments (using mov).
void load_gdt(void) {
	void* descriptor_address = &descriptor;
	asm volatile (
		"lgdt (%0)\n"
		"jmp $0x08,$reload_data\n"
		"reload_data:\n"
		"mov $0x10, %%ax\n"
		"mov %%ax, %%ds\n"
		"mov %%ax, %%es\n"
		"mov %%ax, %%ss\n"
		"mov %%ax, %%gs\n"
		"mov %%ax, %%fs\n"
		:
		: "r" (descriptor_address));
}

// Loads a new TLS into the appropriate registers
void load_tls(uint16_t selector) {
	asm volatile(
		"mov %0, %%ax\n"
		"mov %%ax, %%gs\n"
		"mov %%ax, %%fs\n"
		:
		: "r" (selector));
}

extern "C" uint32_t stack_top;

} // namespace

struct tss main_tss;

constexpr uint8_t NULL_SEGMENT = 0x00;
constexpr uint8_t USER_CODE_SEGMENT = 0xFA;
constexpr uint8_t CODE_SEGMENT = 0x9A;
constexpr uint8_t USER_DATA_SEGMENT = 0xF2;
constexpr uint8_t DATA_SEGMENT = 0x92;
constexpr uint8_t TSS_SEGMENT = 0xE9; // Note that TSS segments access bytes have a different format

constexpr uint8_t FOUR_KB_BLOCKS = 0x80;
constexpr uint8_t PROTECTED_MODE = 0x40;

// The format of a GDT entry is insane and disjointed.
// This handy function helps us not think about it.
void populate_gdt_entry(gdt_entry* entry, uint64_t base, uint32_t limit, uint8_t flags, uint8_t access) {
	entry->base_low = base & 0xFFFF;
	entry->base_mid = (base >> 16) & 0xFF;
	entry->base_high = base >> 24;
	entry->limit = limit & 0xFFFF;
	entry->flags = flags;
	entry->flags |= (limit >> 16) & 0xF;
	entry->access = access;
}

void setup_gdt(void) {
	descriptor.size = GDT_TABLE_SIZE*sizeof(struct gdt_entry)-1;
	descriptor.addr = (uint32_t)gdt_table;

	memset((char*)&main_tss, sizeof(struct tss), 0);
	main_tss.esp0 = (uint32_t)&stack_top;
	main_tss.ss0 = DATA_SELECTOR;	

	populate_gdt_entry(gdt_table, 0, 0, 0, NULL_SEGMENT);
	populate_gdt_entry(gdt_table+1, 0, 0xFFFFF, FOUR_KB_BLOCKS | PROTECTED_MODE, CODE_SEGMENT);
	populate_gdt_entry(gdt_table+2, 0, 0xFFFFF, FOUR_KB_BLOCKS | PROTECTED_MODE, DATA_SEGMENT);
	populate_gdt_entry(gdt_table+3, 0, 0xFFFFF, FOUR_KB_BLOCKS | PROTECTED_MODE, USER_CODE_SEGMENT);
	populate_gdt_entry(gdt_table+4, 0, 0xFFFFF, FOUR_KB_BLOCKS | PROTECTED_MODE, USER_DATA_SEGMENT);
	populate_gdt_entry(gdt_table+5, (uint32_t)&main_tss, sizeof(struct tss), 0, TSS_SEGMENT);

	load_gdt();
	flush_tss();
}

void set_tls(struct tls_segment& tls) {
	populate_gdt_entry(gdt_table+tls.gdt_index, tls.segment_base, tls.limit, FOUR_KB_BLOCKS | PROTECTED_MODE, USER_DATA_SEGMENT);
	load_tls(tls.gdt_index*sizeof(struct gdt_entry) | 0x3);
}

void flush_tss(void) {
	gdt_table[5].access = TSS_SEGMENT; // This needs to be done to clear the busy bit
	asm volatile(
		"movl %0, %%eax\n"
		"ltr %%ax\n"
		:
		: "i" (TSS_SELECTOR));
}

} // namespace memory
} // namespace arch
