#include <stdint.h>

#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"

namespace filesystem {

namespace {

using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::strlen;
using lib::std::memcpy;

} // namespace

void load_file(struct file* file) {
	file->offset = 0;

	struct directory_entry file_stats = stat_fat32(file->path);
	file->size = file_stats.size;

	if (file_stats.name) {
		kfree(file_stats.name);
		file->inode = file_stats.inode; 
	} else {
		file->inode = 0;
	}

	if (file_stats.is_directory) {
		struct directory dir = ls_fat32(file->path);
		
		file->size = 0;
		for (int i = 0; i < dir.num_children; i++) {
			file->size += sizeof(struct dirent_header) + strlen(dir.children[i].name) + 1;
		}

		file->buffer = (char*)kmalloc(file->size);

		uint32_t current_size = 0;
		char* out_buf = file->buffer;
		struct dirent_header* header;
		for (int i = 0; i < dir.num_children; i++) {
			struct directory_entry* child = &dir.children[i];

			int name_len = strlen(child->name) + 1;

			header = (struct dirent_header*)out_buf;
			header->inode_number = 1;
			header->size = sizeof(struct dirent_header) + name_len;

			if (child->is_directory) {
				header->type = DIRECTORY;
			} else {
				header->type = REGULAR_FILE;
			}
			current_size += header->size;

			out_buf += sizeof(struct dirent_header);
			char* name = (char*)out_buf;
			memcpy(child->name, name, name_len);
			out_buf += name_len;

			header->next_offset = current_size;

			kfree(child->name);
		}

		kfree(dir.children);
	} else {
		file->buffer = nullptr;
	}
}

} // namespace filesystem
