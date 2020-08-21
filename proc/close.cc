#include "proc/close.h"
#include "proc/process.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"

namespace proc {

namespace {

using filesystem::file;
using filesystem::flush_file;
using lib::std::kfree;

} // namespace

void close_file(struct process* current_process, struct file* to_close) {
	flush_file(to_close);

	kfree(to_close->path);
	if (to_close->buffer) {
		kfree(to_close->buffer);
	}

	if (!to_close->next && !to_close->prev) {
		current_process->open_files = nullptr;
	} else if (!to_close->prev) {
		current_process->open_files = to_close->next;
		to_close->next->prev = nullptr;
	} else if (!to_close->next) {
		to_close->prev->next = nullptr;
	} else {
		to_close->prev->next = to_close->next;
		to_close->next->prev = to_close->prev;
	}

	kfree(to_close);
}

uint32_t close(uint32_t file_descriptor, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
	struct process* current_process = get_currently_executing_process();

	struct file* current_file = current_process->open_files;
	while (current_file) {
		if (current_file->file_descriptor == file_descriptor) {
			close_file(current_process, current_file);
			return 0;
		}

		current_file = current_file->next;
	}

	return -1;
}

} // namespace proc
