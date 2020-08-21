#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "io/keyboard.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"
#include "proc/read_write.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using arch::memory::physical_to_virtual_memcpy;
using arch::memory::virtual_to_physical;
using arch::memory::virtual_to_physical_memcpy;
using filesystem::del_fat32;
using filesystem::file;
using filesystem::read_fat32;
using filesystem::write_fat32;
using io::KEYBOARD_WAIT;
using io::keyboard_wait;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::krealloc;
using lib::std::memcpy;
using lib::std::streq;
using lib::std::strlen;

uint32_t write_internal(struct process* current_process, uint32_t file_descriptor, char* buf, uint32_t size) {
	if (file_descriptor < 2) {
		for (int i = 0; i < size; i++) {
			lib::std::putc(buf[i]);
		}

		return size;
	} else {
		struct file* to_write = current_process->open_files;
		while(to_write && to_write->file_descriptor != file_descriptor) {
			to_write = to_write->next;
		}

		if (!to_write) {
			return -1;
		}

		// If this is the first time we're writing to the file, erase its previous contents
		if (!to_write->needs_flushed) {
			if (to_write->buffer) {
				kfree(to_write->buffer);
				to_write->buffer = nullptr;
			}
			to_write->size = 0;
		}

		if (!to_write->size) {
			to_write->buffer = (char*)kmalloc(size);
			to_write->size = size;
		} else if (to_write->offset + size > to_write->size) {
			to_write->buffer = (char*)krealloc(to_write->buffer, to_write->offset + size);
			to_write->size = to_write->offset + size;
		}

		memcpy(buf, to_write->buffer + to_write->offset, size);

		to_write->offset += size;

		to_write->needs_flushed = 1;

		return size;
	}
}

uint32_t read_from_keyboard(struct process* current_process, char* buf, uint32_t size) {
	struct keyboard_wait* wait = (struct keyboard_wait*)kmalloc(sizeof(struct keyboard_wait));
	wait->type = KEYBOARD_WAIT;
	wait->buf = buf;
	wait->index = 0;
	wait->len = size;

	current_process->process_state = WAITING;
	current_process->wait = wait;

	return size;
}

} // namespace

uint32_t read(uint32_t file_descriptor, uint32_t dest, uint32_t size, uint32_t reserved1, uint32_t reserved2) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;

	if (file_descriptor < 2) {
		return read_from_keyboard(current_process, (char*)dest, size);
	} else {
		struct file* to_read = current_process->open_files;
		while(to_read && to_read->file_descriptor != file_descriptor) {
			to_read = to_read->next;
		}

		if (!to_read) {
			return -1;
		}

		uint32_t read_size = size < (to_read->size - to_read->offset) ? size : (to_read->size - to_read->offset);
		if (read_size) {
			physical_to_virtual_memcpy(page_dir, to_read->buffer, (char*)dest, read_size);
		}

		to_read->offset += read_size;

		return read_size;
	}
}

uint32_t readlink(uint32_t path_addr, uint32_t buf_addr, uint32_t len, uint32_t reserved1, uint32_t reserved2) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;
	char* path = make_virtual_string_copy(page_dir, (char*)path_addr);

	if (streq("/proc/self/exe", path, strlen("/proc/self/exe"))) {
		size_t executable_path_len = strlen(current_process->path) + 1;
		size_t copy_len = executable_path_len < len ? executable_path_len : len;
		physical_to_virtual_memcpy(page_dir, current_process->path, (char*)buf_addr, copy_len);
		kfree(path);
		return copy_len;
	}

	kfree(path);
	return -1;
}

uint32_t write(uint32_t file_descriptor, uint32_t src, uint32_t size, uint32_t reserved1, uint32_t reserved2) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;
	char* buf = (char*)kmalloc(size);
	virtual_to_physical_memcpy(page_dir, (char*)src, buf, size);

	uint32_t ret = write_internal(current_process, file_descriptor, buf, size);
	kfree(buf);
	return ret;
}

uint32_t writev(uint32_t file_descriptor, uint32_t io_vector_addr, uint32_t vector_len, uint32_t reserved1, uint32_t reserved2) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;
	struct io_vector* vector = (struct io_vector*)kmalloc(sizeof(struct io_vector) * vector_len);
	virtual_to_physical_memcpy(page_dir, (char*)io_vector_addr, (char*)vector, sizeof(struct io_vector) * vector_len);

	uint32_t total_len = 0;
	for (int i = 0; i < vector_len; i++) {	
		total_len += vector[i].len;
	}

	char* buf = (char*)kmalloc(total_len);
	char* current_buf = buf;
	for (int i = 0; i < vector_len; i++) {
		virtual_to_physical_memcpy(page_dir, vector[i].buf, current_buf, vector[i].len);
		current_buf += vector[i].len;
	}

	uint32_t ret = write_internal(current_process, file_descriptor, buf, total_len);
	kfree(buf);
	kfree(vector);
	return ret;
}

uint32_t unlink(uint32_t path_addr, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;
	char* path = make_virtual_string_copy(page_dir, (char*)path_addr);
	del_fat32(path);
	kfree(path);
	return 0;
}

} // namespace proc
