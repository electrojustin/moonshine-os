#include <stdint.h>

#include "arch/i386/memory/paging.h"
#include "proc/elf_loader.h"
#include "proc/execve.h"
#include "proc/process.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using lib::std::kfree;
using lib::std::make_string_copy;
using lib::std::strcat;

} // namespace

uint32_t execve(uint32_t path_addr, uint32_t argv_addr, uint32_t env_addr, uint32_t reserved1, uint32_t reserved2) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;
	char* relative_path = make_virtual_string_copy(page_dir, (char*)path_addr);

	if (relative_path[0] != '/') {
		char* tmp = relative_path;
		relative_path = strcat(current_process->working_dir, relative_path);
		kfree(tmp);
	}

	if (!load_elf(relative_path, make_string_copy(current_process->working_dir))) {
		return -1;
	}

	current_process->process_state = STOPPED;

	kfree(relative_path);

	return 0;
}

} // namespace proc
