#include <stddef.h>

#include "arch/i386/memory/paging.h"
#include "proc/brk.h"
#include "proc/process.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"

namespace proc {

namespace {

using arch::memory::map_memory_segment;
using arch::memory::PAGE_SIZE;
using arch::memory::user_read_write;
using lib::std::krealloc;
using lib::std::kmalloc_aligned;
using lib::std::memset;

} // namespace

uint32_t brk(uint32_t new_brk, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
	struct process* current_process = get_currently_executing_process();
	if (!new_brk) {
		return current_process->brk;
	}

	current_process->brk = new_brk;
	if (current_process->brk > current_process->actual_brk) {
		current_process->num_segments++;
		current_process->segments = (struct process_memory_segment*)krealloc(current_process->segments, current_process->num_segments*sizeof(struct process_memory_segment));
		struct process_memory_segment* new_segment = current_process->segments + current_process->num_segments - 1;
		size_t segment_size = new_brk - current_process->actual_brk;
		segment_size = (segment_size & (PAGE_SIZE-1)) ? ((segment_size/PAGE_SIZE)+1)*PAGE_SIZE : segment_size;
		new_segment->virtual_address = (void*)current_process->actual_brk;
		new_segment->segment_size = segment_size;
		new_segment->alloc_size = segment_size;
		new_segment->actual_address = kmalloc_aligned(segment_size, PAGE_SIZE);
		new_segment->flags = WRITEABLE_MEMORY | READABLE_MEMORY;
		new_segment->source = nullptr;
		new_segment->disk_size = 0;
		current_process->actual_brk += segment_size;
		map_memory_segment(current_process,
				   (uint32_t)new_segment->actual_address,
				   (uint32_t)new_segment->virtual_address,
				   segment_size,
				   user_read_write);
		memset((char*)new_segment->actual_address, segment_size, 0);
	}

	return new_brk;
}

} // namespace proc
