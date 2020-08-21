#ifndef ARCH_PAGING_H
#define ARCH_PAGING_H

#include <stddef.h>
#include <stdint.h>

#include "proc/process.h"

// These are defined in boot.s, near the initial stack.
extern uint32_t base_page_directory[1024];
extern uint32_t base_page_table[1024*1024];

namespace arch {
namespace memory {

namespace {

using proc::process;

} // namespace

constexpr size_t PAGE_SIZE = 4096;

enum permission {
	kernel_read_only = 0,
	kernel_read_write = 2,
	user_read_only = 4,
	user_read_write = 6
};

// Map a range of physical addresses to a range of virtual addresses
void map_memory_range(uint32_t* page_directory, uint32_t* page_table, void* physical_address, void* virtual_address, uint32_t size, enum permission page_permissions);

// A variant of the above function that assumes the caller has passed in pointers to the correct page table entries.
void map_memory_range_offset(uint32_t* page_directory, uint32_t* page_table_entry, void* physical_address, void* virtual_address, uint32_t size, enum permission page_permissions);

void map_memory_segment(struct process* proc, uint32_t physical_address, uint32_t virtual_address, size_t len, enum permission page_permissions);

// Sets the CR3 register to a pointer to the current page directory.
static void inline set_page_directory(uint32_t* page_directory) {
	asm volatile(
		"mov %0, %%cr3"
		:
		: "r" (page_directory));
}

// Sets the page flag in the CR0 register and then "refreshes" the MMU by copying CR3 and copying it back.
static void inline enable_paging(void) {
	asm volatile(
		"mov %%cr0, %%ecx\n"
		"or $0x80000001, %%ecx\n"
		"mov %%ecx, %%cr0\n"
		"mov %%cr3, %%ecx\n"
		"mov %%ecx, %%cr3"
		:
		:
		: "ecx");
}

void* virtual_to_physical(uint32_t* page_dir, void* virtual_addr);

void physical_to_virtual_memcpy(uint32_t* page_dir, char* src, char* dest, size_t size);

void virtual_to_physical_memcpy(uint32_t* page_dir, char* src, char* dest, size_t size);

char* make_virtual_string_copy(uint32_t* page_dir, char* virtual_string);

} // namespace memory
} // namespace arch

#endif
