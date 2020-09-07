#ifndef FILESYSTEM_FILE_H
#define FILESYSTEM_FILE_H

#include <stddef.h>
#include <stdint.h>

namespace filesystem {

struct file {
	uint32_t file_descriptor;
	char* path;
	char* buffer;
	uint32_t inode; // Actually just cluster num
	uint32_t size;
	uint32_t offset;
	void* mapping;
	size_t mapping_len;
	struct file* next;
	struct file* prev;
};

struct __attribute__((packed)) dirent_header {
	uint64_t inode_number = 1; // Unused
	uint64_t next_offset;
	uint16_t size;
	uint8_t type;
};

constexpr uint8_t SYMLINK = 0xA;
constexpr uint8_t REGULAR_FILE = 0x8;
constexpr uint8_t BLOCK_DEVICE = 0x6;
constexpr uint8_t DIRECTORY = 0x4;
constexpr uint8_t CHAR_DEVICE = 0x2;
constexpr uint8_t FIFO = 0x1;

constexpr uint32_t ALL_RWX = 0x1FF;

// Loads a file into memory
void load_file(struct file* file);

} // namespace filesystem

#endif
