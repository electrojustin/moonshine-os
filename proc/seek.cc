#include "filesystem/file.h"
#include "proc/seek.h"

namespace proc {

uint32_t lseek(uint32_t file_descriptor, uint32_t offset, uint32_t whence, uint32_t reserved1, uint32_t reserved2) {
	struct process* current_process = get_currently_executing_process();
	struct file* to_seek = current_process->open_files;

	while (to_seek && to_seek->file_descriptor != file_descriptor) {
		to_seek = to_seek->next;
	}

	if (!to_seek) {
		return -1;
	}

	switch (whence) {
		case SEEK_SET:
			to_seek->offset = offset;
			return 0;
		case SEEK_CUR:
			to_seek->offset += offset;
			return 0;
		case SEEK_END:
			to_seek->offset = to_seek->size + offset;
			return 0;
		default:
			return -1;
	}
}

} // namespace proc
