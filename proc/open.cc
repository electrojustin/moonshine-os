#include "proc/open.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using filesystem::directory_entry;
using filesystem::load_file;
using filesystem::stat_fat32;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::make_string_copy;
using lib::std::strcat;
using lib::std::strlen;
using lib::std::substring;

constexpr uint32_t AT_FDCWD =
    (~100) +
    1; // This is supposed to be -100 signed but we assume unsigned ints :\

uint32_t open_internal(struct process *current_process, char *path,
                       uint32_t flags, uint32_t mode) {
  int parent_path_index = strlen(path);
  while (parent_path_index && path[parent_path_index] != '/') {
    parent_path_index--;
  }
  if (parent_path_index > 1) {
    char *parent_path = substring(path, 0, parent_path_index);
    struct directory_entry entry = stat_fat32(parent_path);
    kfree(parent_path);
    if (!entry.name) {
      return -1;
    }
    kfree(entry.name);
  }

  struct file *new_file = (struct file *)kmalloc(sizeof(struct file));
  new_file->file_descriptor = current_process->next_file_descriptor;
  current_process->next_file_descriptor++;
  new_file->path = make_string_copy(path);
  if (!current_process->open_files) {
    current_process->open_files = new_file;
    new_file->prev = nullptr;
    new_file->next = nullptr;
  } else {
    struct file *current_file = current_process->open_files;
    while (current_file->next) {
      current_file = current_file->next;
    }
    current_file->next = new_file;
    new_file->prev = current_file;
    new_file->next = nullptr;
  }

  load_file(new_file);

  return new_file->file_descriptor;
}

} // namespace

uint32_t open(uint32_t path_addr, uint32_t flags, uint32_t mode,
              uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *relative_path = make_virtual_string_copy(page_dir, (char *)path_addr);

  if (relative_path[0] == '/') {
    uint32_t ret = open_internal(current_process, relative_path, flags, mode);
    kfree(relative_path);
    return ret;
  } else {
    char *path = strcat(current_process->working_dir, relative_path);
    uint32_t ret = open_internal(current_process, path, flags, mode);
    kfree(path);
    kfree(relative_path);
    return ret;
  }
}

uint32_t openat(uint32_t directory_fd, uint32_t path_addr, uint32_t flags,
                uint32_t mode, uint32_t reserved1, uint32_t reserved2) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *relative_path = make_virtual_string_copy(page_dir, (char *)path_addr);

  if (relative_path[0] == '/') {
    uint32_t ret = open_internal(current_process, relative_path, flags, mode);
    kfree(relative_path);
    return ret;
  } else if (directory_fd == AT_FDCWD) {
    char *path = strcat(current_process->working_dir, relative_path);
    uint32_t ret = open_internal(current_process, path, flags, mode);
    kfree(path);
    kfree(relative_path);
    return ret;
  }

  struct file *current_file = current_process->open_files;
  while (current_file && current_file->file_descriptor != directory_fd) {
    current_file = current_file->next;
  }

  if (!current_file) {
    kfree(relative_path);
    return -1;
  }

  char *tmp = strcat(current_file->path, "/");
  char *tmp2 = strcat(tmp, relative_path);
  uint32_t return_val = open_internal(current_process, tmp2, flags, mode);
  kfree(tmp);
  kfree(relative_path);

  return return_val;
}

uint32_t access(uint32_t path_addr, uint32_t mode, uint32_t reserved1,
                uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  return 0;
}

} // namespace proc
