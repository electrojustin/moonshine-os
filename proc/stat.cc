#include "proc/stat.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "lib/math.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace proc {

using arch::memory::physical_to_virtual_memcpy;
using filesystem::ALL_RWX;
using filesystem::CHAR_DEVICE;
using filesystem::DIRECTORY;
using filesystem::directory_entry;
using filesystem::file;
using filesystem::REGULAR_FILE;
using filesystem::stat_fat32;
using lib::divide;
using lib::std::kfree;
using lib::std::kmalloc;

uint32_t fstat64(uint32_t fd, uint32_t stat_addr, uint32_t reserved1,
                 uint32_t reserved2, uint32_t reserved3) {
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
    struct file *current_file = current_process->open_files;
    while (current_file && current_file->file_descriptor != fd) {
      current_file = current_file->next;
    }

    if (!current_file) {
      kfree(dest_stat);
      return -1;
    }

    struct directory_entry dir_entry = stat_fat32(current_file->path);

    if (!dir_entry.name) {
      kfree(dest_stat);
      return -1;
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
  }

  physical_to_virtual_memcpy(page_dir, (char *)dest_stat, (char *)stat_addr,
                             sizeof(struct stat64));

  return 0;
}

} // namespace proc
