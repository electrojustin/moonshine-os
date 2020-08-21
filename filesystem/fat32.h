#ifndef FILESYSTEM_FAT32_H
#define FILESYSTEM_FAT32_H

#include <stddef.h>
#include <stdint.h>

#include "drivers/i386/pata.h"
#include "filesystem/mbr.h"

namespace filesystem {

namespace {

using drivers::ide_device;

} // namespace

// Human readable representation of a directory entry
struct directory_entry {
	char* name;
	char read_only;
	char is_directory;
	uint32_t size;
};

// Human readable representation of a directory
struct directory {
	size_t num_children;
	struct directory_entry* children;
};

// Initialize the filesystem
void init_fat32(struct ide_device& device, struct partition& partition);

// Read the first len bytes of a file
// Returns 1 on success, 0 otherwise
char read_fat32(char* path, uint8_t* buf, size_t len);

// Returns basic information about a file or directory
struct directory_entry stat_fat32(char* path);

// Returns a list of files in a directory
// num_children will be 0 if path points to a file, not a directory
struct directory ls_fat32(char* path);

// Write a file of size len
// Returns 1 on success, 0 otherwise
char write_fat32(char* path, uint8_t attributes, uint8_t* buf, size_t len);

// Creates a new directory at the specified path
// Returns 1 on success, 0 otherwise
char mkdir_fat32(char* path);

// Deletes a file
// Note that this will not 0 the file, so it could be recoverable
char del_fat32(char* path);

} // namespace filesystem

#endif
