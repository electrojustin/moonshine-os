#include "proc/stat.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "lib/math.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using arch::memory::physical_to_virtual_memcpy;
using filesystem::ALL_RWX;
using filesystem::CHAR_DEVICE;
using filesystem::DIRECTORY;
using filesystem::directory_entry;
using filesystem::FIFO;
using filesystem::file;
using filesystem::REGULAR_FILE;
using filesystem::stat_fat32;
using lib::divide;
using lib::std::kfree;
using lib::std::kmalloc;

int stat_internal(uint32_t *page_dir, char *path, struct stat64 *dest_stat) {
  struct directory_entry dir_entry = stat_fat32(path);

  if (!dir_entry.name) {
    return 0;
  }

  kfree(dir_entry.name);

  dest_stat->size = dir_entry.size;
  dest_stat->block_size = 512;
  dest_stat->num_blocks = divide(dest_stat->size, dest_stat->block_size) + 1;

  if (dir_entry.is_directory) {
    dest_stat->mode = ((uint32_t)DIRECTORY << 12) | ALL_RWX;
  } else {
    dest_stat->mode = ((uint32_t)REGULAR_FILE << 12) | ALL_RWX;
  }

  return 1;
}

} // namespace

uint32_t fstat64(uint32_t fd, uint32_t stat_addr, uint32_t reserved1,
                 uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  struct stat64 *dest_stat = (struct stat64 *)kmalloc(sizeof(struct stat64));

  dest_stat->dev_id = 0;
  dest_stat->inode_num = 0;
  dest_stat->num_hardlinks = 0;
  dest_stat->uid = 0;
  dest_stat->gid = 0;

  if (fd < 2) {
    dest_stat->mode = ((uint32_t)CHAR_DEVICE << 12) | ALL_RWX;
  } else {
    struct file_descriptor *current_fd =
        find_file_descriptor(fd, current_process->open_files);

    if (!current_fd) {
      kfree(dest_stat);
      return -1;
    }

    if (current_fd->file->read_write_pipe) {
      dest_stat->mode = ((uint32_t)FIFO << 12) | ALL_RWX;
    } else if (!stat_internal(page_dir, current_fd->file->path, dest_stat)) {
      kfree(dest_stat);
      return -1;
    }
  }

  physical_to_virtual_memcpy(page_dir, (char *)dest_stat, (char *)stat_addr,
                             sizeof(struct stat64));

  kfree(dest_stat);

  return 0;
}

uint32_t stat64(uint32_t path_addr, uint32_t stat_addr, uint32_t reserved1,
                uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  struct stat64 *dest_stat = (struct stat64 *)kmalloc(sizeof(struct stat64));
  char *path = make_virtual_string_copy(page_dir, (char *)path_addr);

  dest_stat->dev_id = 0;
  dest_stat->inode_num = 0;
  dest_stat->num_hardlinks = 0;
  dest_stat->uid = 0;
  dest_stat->gid = 0;

  if (!stat_internal(page_dir, path, dest_stat)) {
    kfree(dest_stat);
    kfree(path);
    return -1;
  }

  physical_to_virtual_memcpy(page_dir, (char *)dest_stat, (char *)stat_addr,
                             sizeof(struct stat64));

  kfree(path);
  kfree(dest_stat);

  return 0;
}

} // namespace proc
