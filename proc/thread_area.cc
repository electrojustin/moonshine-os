#include <stdint.h>

#include "arch/i386/memory/gdt.h"
#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "proc/process.h"
#include "proc/thread_area.h"
#include "lib/std/stdio.h"

namespace proc {

namespace {

using arch::memory::TLS_ENTRY_OFFSET;
using arch::memory::tls_segment;
using arch::memory::set_page_directory;
using lib::std::krealloc;

constexpr int MAX_TLS_SEGMENTS = 3;

} // namespace

uint32_t set_thread_area(uint32_t tls_segment_addr, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4, uint32_t reserved5) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;

	set_page_directory(page_dir);
	struct tls_segment* tls = (struct tls_segment*)tls_segment_addr;
	struct tls_segment tls_copy = *tls;

	// If index >= 0, we just set the TLS index to that value
	if (tls_copy.gdt_index - TLS_ENTRY_OFFSET >= 0) {
		int new_index = tls->gdt_index;
		set_page_directory(base_page_directory);
		if (new_index - TLS_ENTRY_OFFSET < current_process->num_tls_segments) {
			current_process->tls_segment_index = new_index - TLS_ENTRY_OFFSET;
			return 0;
		} else {
			return -1;
		}
		
	} else if (tls_copy.gdt_index == -1) {
		// If index == -1, we make a new entry in the TLS array
		set_page_directory(base_page_directory);

		current_process->num_tls_segments++;
		if (current_process->num_tls_segments > MAX_TLS_SEGMENTS) {
			return -1;
		}

		uint32_t new_index = current_process->num_tls_segments - 1 + TLS_ENTRY_OFFSET;
		current_process->tls_segment_index = new_index - TLS_ENTRY_OFFSET;

		tls_copy.gdt_index = new_index;

		current_process->tls_segments = (struct tls_segment*)krealloc(current_process->tls_segments, current_process->num_tls_segments * sizeof(struct tls_segment));
		current_process->tls_segments[current_process->num_tls_segments-1] = tls_copy;

		set_page_directory(page_dir);
		tls->gdt_index = new_index;
		return 0;
	} else {
		return -1;
	}
}

uint32_t get_thread_area(uint32_t tls_segment_addr, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4, uint32_t reserved5) {
	struct process* current_proc = get_currently_executing_process();
	uint32_t* page_dir = current_proc->page_dir;
	uint32_t index = current_proc->tls_segment_index;

	set_page_directory(page_dir);
	struct tls_segment* tls = (struct tls_segment*)tls_segment_addr;
	tls->gdt_index = index + TLS_ENTRY_OFFSET;
	return 0;
}

} // namespace proc
