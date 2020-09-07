#include <stdint.h>
#include <stddef.h>

#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace arch {
namespace memory {

namespace {

using filesystem::read_fat32;
using filesystem::write_fat32;
using filesystem::file;
using proc::process;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::kmalloc_aligned;
using lib::std::krealloc;
using lib::std::memset;
using lib::std::panic;
using lib::std::strlen;

constexpr uint16_t PRESENT = 0x1;
constexpr uint16_t DIRTY = 0x40;

uint32_t* get_page_table_entry(uint32_t* page_dir, void* virtual_addr) {
	uint32_t page_dir_index = (uint32_t)virtual_addr >> 22;
	uint32_t page_table_index = ((uint32_t)virtual_addr >> 12) & 0x3FF;
	uint32_t* page_table = (uint32_t*)(page_dir[page_dir_index] & (~(0xFFF)));

	if (page_table == nullptr) {
		return nullptr;
	}

	return page_table + page_table_index;
}

} // namespace

void map_memory_range(uint32_t* page_directory, uint32_t* page_table, void* physical_address, void* virtual_address, uint32_t size, enum permission page_permissions) {
	uint32_t* page_table_entry = page_table + ((uint32_t)virtual_address / PAGE_SIZE);
	map_memory_range_offset(page_directory, page_table_entry, physical_address, virtual_address, size, page_permissions);
}

void unmap_memory_range(uint32_t* page_directory, void* virtual_addr, size_t len) {
	void* current_addr = virtual_addr;
	while (current_addr < virtual_addr + len) {
		*get_page_table_entry(page_directory, current_addr) = 0;
	}
}

void map_memory_range_offset(uint32_t* page_directory, uint32_t* page_table_entry, void* physical_address, void* virtual_address, uint32_t size, enum permission page_permissions) {
	if ((uint32_t)physical_address & 0xFFF || size & 0xFFF) {
		lib::std::panic((char*)"Page mappings must be 4KB aligned!");
	}

	void* current_address = physical_address;
	uint32_t page_index = (uint32_t)virtual_address / PAGE_SIZE;
	uint32_t page_table_offset = 0;
	page_directory[page_index/1024] = (uint32_t)(page_table_entry + page_table_offset) | page_permissions | 0x1;
	while ((uint32_t)current_address < (uint32_t)physical_address + size) {
		// Every 1024 pages we need a new page directory.
		if (!(page_index % 1024)) {
			page_directory[page_index/1024] = (uint32_t)(page_table_entry + page_table_offset) | page_permissions | 0x1;
		}

		// Add an entry to the page table.
		page_table_entry[page_table_offset] = ((uint32_t)current_address & 0xFFFFF000) | page_permissions | 0x1;

		// Go to the next page.
		current_address = (void*)((uint32_t)current_address + PAGE_SIZE);
		page_index++;
		page_table_offset++;
	}
}

void map_memory_segment(struct process* proc, uint32_t physical_address, uint32_t virtual_address, size_t len, enum permission page_permissions) {
	uint32_t current_address = virtual_address;
	uint32_t* page_table;
	while (current_address < virtual_address + len) {
		if (proc->page_dir[current_address >> 22]) {
			page_table = (uint32_t*)(proc->page_dir[current_address >> 22] & (~(PAGE_SIZE-1)));
		} else {
			page_table = (uint32_t*)kmalloc_aligned(1024*sizeof(uint32_t), PAGE_SIZE);
			memset((char*)page_table, 1024*sizeof(uint32_t), 0);
			proc->num_page_tables++;
			if (!proc->page_tables) {
				proc->page_tables = (uint32_t**)kmalloc(sizeof(uint32_t*));
			} else {
				proc->page_tables = (uint32_t**)krealloc(proc->page_tables, proc->num_page_tables*sizeof(uint32_t*));
			}

			proc->page_tables[proc->num_page_tables-1] = page_table;

			proc->page_dir[current_address >> 22] = (uint32_t)page_table | page_permissions | 0x1;
		}

		if (physical_address) {
			page_table[(current_address >> 12) & 0x3FF] = physical_address | page_permissions | 0x1;
		} else {
			page_table[(current_address >> 12) & 0x3FF] = page_permissions;
		}

		current_address += PAGE_SIZE;
		if (physical_address) {
			physical_address += PAGE_SIZE;
		}
	}
}

void* virtual_to_physical(uint32_t* page_dir, void* virtual_addr, uint16_t* flags) {
	uint32_t offset = (uint32_t)virtual_addr & 0xFFF;
	uint32_t* page_table_entry = get_page_table_entry(page_dir, virtual_addr);

	if (flags) {
		*flags = *page_table_entry & 0xFFF;
	}
	
	char* return_addr = (char*)(*page_table_entry & (~(0xFFF)));

	if (return_addr == nullptr) {
		return nullptr;
	}

	return (void*)(return_addr + offset);
}

void physical_to_virtual_memcpy(uint32_t* page_dir, char* src, char* dest, size_t size) {
	char* physical_dest = (char*)virtual_to_physical(page_dir, (void*)dest);
	size_t current = 0;
	while (current < size) {
		if (!((uint32_t)(physical_dest) & (PAGE_SIZE-1))) {
			physical_dest = (char*)virtual_to_physical(page_dir, (void*)dest);
			if (!physical_dest) {
				panic("Null pointer exception!");
			}
		}

		*physical_dest = *src;
		physical_dest++;
		src++;
		current++;
		dest++;
	}
}

void virtual_to_physical_memcpy(uint32_t* page_dir, char* src, char* dest, size_t size) {
	char* physical_src = (char*)virtual_to_physical(page_dir, (void*)src);
	size_t current = 0;
	while (current < size) {
		if (!((uint32_t)(physical_src) & (PAGE_SIZE-1))) {
			physical_src = (char*)virtual_to_physical(page_dir, (void*)src);
			if (!physical_src) {
				panic("Null pointer exception!");
			}
		}

		*dest = *physical_src;
		physical_src++;
		src++;
		current++;
		dest++;
	}
}

char* make_virtual_string_copy(uint32_t* page_dir, char* virtual_string) {
	set_page_directory(page_dir);
	uint32_t len = strlen(virtual_string) + 1;
	set_page_directory(base_page_directory);
	char* ret = (char*)kmalloc(len);
	virtual_to_physical_memcpy(page_dir, virtual_string, ret, len);
	return ret;
}

char swap_in_page(struct process* proc, void* virtual_addr) {
	struct file* current_file = proc->open_files;

	while(current_file && !(current_file->mapping <= virtual_addr && current_file->mapping + current_file->mapping_len > virtual_addr)) {
		current_file = current_file->next;
	}

	if (!current_file || !current_file->inode) {
		return 0; // Uh oh, either segfault or panic
	}

	void* actual_addr = kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);

	map_memory_segment(proc, 
			   (uint32_t)actual_addr,
			   (uint32_t)virtual_addr,
			   PAGE_SIZE,
			   user_read_write);

	uint32_t offset = (uint32_t)virtual_addr - (uint32_t)current_file->mapping;
	size_t read_len = current_file->mapping_len - offset < PAGE_SIZE ? current_file->mapping_len - offset : PAGE_SIZE;
	read_fat32(current_file->inode, offset, (uint8_t*)actual_addr, read_len);
	return 1;
}

void flush_pages(uint32_t* page_dir, struct file* backing_file) {
	void* current_addr = backing_file->mapping;
	while(current_addr < backing_file->mapping + backing_file->mapping_len) {
		uint16_t flags = 0;
		void* actual_addr = virtual_to_physical(page_dir, current_addr, &flags);	
		if (!actual_addr || !(flags & PRESENT)) {
			current_addr += PAGE_SIZE;
			continue;
		}

		uint32_t* page_table_entry = get_page_table_entry(page_dir, current_addr);

		lib::std::printk("here2\n");
		if (flags & DIRTY) {
			// Flush dirty pages to disk
			uint32_t offset = (uint32_t)current_addr - (uint32_t)backing_file->mapping;
			size_t write_len = backing_file->mapping_len - offset < PAGE_SIZE ? backing_file->mapping_len - offset : PAGE_SIZE;
			lib::std::printk("here3 %d %d %c\n", write_len, offset, *((char*)actual_addr+3));
			write_fat32(backing_file->path, offset, (uint8_t*)actual_addr, write_len);
			*page_table_entry ^= DIRTY;
		}

		// Free the memory allocation
		kfree(actual_addr);
	
		*page_table_entry ^= PRESENT;

		current_addr += PAGE_SIZE;
	}
}

} // namespace memory
} // namespace arch
