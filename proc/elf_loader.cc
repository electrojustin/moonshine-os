#include <stddef.h>
#include <stdint.h>

#include "filesystem/fat32.h"
#include "lib/std/memory.h"
#include "proc/elf_loader.h"
#include "proc/process.h"
#include "lib/std/stdio.h"

namespace proc {

namespace {

using filesystem::directory_entry;
using filesystem::stat_fat32;
using filesystem::read_fat32;
using lib::std::kmalloc;
using lib::std::kfree;
using lib::std::memcpy;
using proc::process_memory_segment;

struct __attribute__((packed)) elf_header {
	unsigned char ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t segments_offset;
	uint32_t sections_offset;
	uint32_t flags;
	uint16_t header_size;
	uint16_t segment_entry_size;
	uint16_t num_segments;
	uint16_t section_entry_size;
	uint16_t num_sections;
	uint16_t string_table_index;
};

struct __attribute__((packed)) segment_header {
	uint32_t type;
	uint32_t offset;
	uint32_t virtual_address;
	uint32_t physical_address; // Ignored
	uint32_t disk_size;
	uint32_t memory_size;
	uint32_t flags;
	uint32_t alignment;
};

constexpr uint16_t EXECUTABLE_TYPE = 2;

constexpr uint16_t MACHINE_TYPE_X86 = 3;

constexpr uint32_t LOADABLE_SEGMENT = 1;
constexpr uint32_t TLS_SEGMENT = 7;

} // namespace

char load_elf(char* path, char* working_dir) {
	struct directory_entry file_info = stat_fat32(path);
	if (!file_info.name || !file_info.size) {
		kfree(file_info.name);
		return 0;
	}
	kfree(file_info.name);

	// Load executable into memory
	uint8_t* file_buf = (uint8_t*)kmalloc(file_info.size);	
	if (!read_fat32(path, file_buf, file_info.size)) {
		kfree(file_buf);
		return 0;
	}

	// Check the header to make sure this is actually an ELF
	struct elf_header* header = (struct elf_header*)file_buf;
	if (header->ident[0] != 0x7F ||
	    header->ident[1] != 'E' ||
	    header->ident[2] != 'L' ||
	    header->ident[3] != 'F' ||
	    header->type != EXECUTABLE_TYPE ||
	    header->machine != MACHINE_TYPE_X86) {
		kfree(file_buf);
		return 0;
	}

	// Find loadable segments and fill out the Process Memory Segment table appropriately
	uint32_t num_segments = header->num_segments;
	struct segment_header* segment_table = (struct segment_header*)(file_buf + header->segments_offset);
	uint32_t num_loadable_segments = 0;
	for (int i = 0; i < num_segments; i++) {
		if (segment_table[i].type == LOADABLE_SEGMENT) {
			num_loadable_segments++;
		}
	}

	struct process_memory_segment* process_segments = (struct process_memory_segment*)kmalloc(num_loadable_segments*sizeof(struct process_memory_segment));
	int process_segment_index = 0;
	for (int i = 0; i < num_segments; i++) {
		if (segment_table[i].type == LOADABLE_SEGMENT) {
			process_segments[process_segment_index].virtual_address = (void*)(segment_table[i].virtual_address);
			process_segments[process_segment_index].segment_size = segment_table[i].memory_size;
			process_segments[process_segment_index].flags = segment_table[i].flags;
			if (segment_table[i].disk_size) {
				process_segments[process_segment_index].source = file_buf + segment_table[i].offset;
				process_segments[process_segment_index].disk_size = segment_table[i].disk_size;
			}

			process_segment_index++;
		}
	}

	// Spawn the process
	char ret = spawn_new_process(path, process_segments, num_loadable_segments, (void(*)())header->entry, working_dir);
	kfree(process_segments);
	kfree(file_buf);
	return ret;
}

} // namespace proc
