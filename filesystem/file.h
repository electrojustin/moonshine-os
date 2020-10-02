#ifndef FILESYSTEM_FILE_H
#define FILESYSTEM_FILE_H

#include <stddef.h>
#include <stdint.h>

namespace filesystem {

struct file_mapping {
  void *mapping;
  size_t mapping_len;
  uint32_t offset;
  struct file *file;
  char is_private;
  struct file_mapping *next;
  struct file_mapping *prev;
};

struct pipe;

struct file_descriptor {
  uint32_t num;
  struct file *file;
  struct file_descriptor *next;
  struct file_descriptor *prev;
};

struct file {
  char *path;
  char *buffer; // Exclusively used for directories
  struct pipe *read_write_pipe;
  uint32_t inode; // Actually just cluster num
  uint32_t size;
  uint32_t offset;
  int num_references;
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
void load_file(struct file *file);

struct file_descriptor *
find_file_descriptor(uint32_t descriptor_num,
                     struct file_descriptor *descriptors);

} // namespace filesystem

#endif
