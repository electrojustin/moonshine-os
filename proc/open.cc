#include "proc/open.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "filesystem/pipe.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using arch::memory::physical_to_virtual_memcpy;
using filesystem::directory_entry;
using filesystem::file;
using filesystem::file_descriptor;
using filesystem::load_file;
using filesystem::pipe;
using filesystem::PIPE_MAX_SIZE;
using filesystem::stat_fat32;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::make_string_copy;
using lib::std::memset;
using lib::std::strcat;
using lib::std::strlen;
using lib::std::substring;

constexpr uint32_t AT_FDCWD =
    (~100) +
    1; // This is supposed to be -100 signed but we assume unsigned ints :\

constexpr uint32_t O_CREAT = 0x200;

uint32_t open_internal(struct process *current_process, char *path,
                       uint32_t flags, uint32_t mode) {
  if (!(flags & O_CREAT)) {
    struct directory_entry entry = stat_fat32(path);
    if (!entry.name) {
      return -2;
    }
    kfree(entry.name);
    if (path[1] == 'e') {
      while (1)
        ;
    }
  } else {
    int parent_path_index = strlen(path);
    while (parent_path_index && path[parent_path_index] != '/') {
      parent_path_index--;
    }
    if (parent_path_index > 1) {
      char *parent_path = substring(path, 0, parent_path_index);
      struct directory_entry entry = stat_fat32(parent_path);
      kfree(parent_path);
      if (!entry.name) {
        return -2;
      }
      kfree(entry.name);
    }
  }

  struct file *new_file = (struct file *)kmalloc(sizeof(struct file));
  new_file->path = make_string_copy(path);

  struct file_descriptor *new_fd =
      (struct file_descriptor *)kmalloc(sizeof(struct file_descriptor));
  new_fd->file = new_file;
  new_fd->num = current_process->next_file_descriptor;
  current_process->next_file_descriptor++;
  new_fd->prev = nullptr;
  if (!current_process->open_files) {
    current_process->open_files = new_fd;
    new_fd->next = nullptr;
  } else {
    new_fd->prev = nullptr;
    new_fd->next = current_process->open_files;
    new_fd->next->prev = new_fd;
    current_process->open_files = new_fd;
  }

  load_file(new_file);

  return new_fd->num;
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

  struct file_descriptor *current_fd =
      find_file_descriptor(directory_fd, current_process->open_files);

  if (!current_fd) {
    kfree(relative_path);
    return -1;
  }

  char *tmp = strcat(current_fd->file->path, "/");
  char *tmp2 = strcat(tmp, relative_path);
  uint32_t return_val = open_internal(current_process, tmp2, flags, mode);
  kfree(tmp);
  kfree(relative_path);

  return return_val;
}

uint32_t access(uint32_t path_addr, uint32_t mode, uint32_t reserved1,
                uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *path = make_virtual_string_copy(page_dir, (char *)path_addr);

  struct directory_entry dir_entry = stat_fat32(path);

  if (!dir_entry.name) {
    kfree(path);
    return -2;
  }

  kfree(path);
  kfree(dir_entry.name);
  return 0;
}

uint32_t pipe(uint32_t fd_addr, uint32_t flags, uint32_t reserved1,
              uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;

  struct pipe *new_pipe = (struct pipe *)kmalloc(sizeof(struct pipe));
  memset((char *)new_pipe->buf, PIPE_MAX_SIZE, 0);
  new_pipe->read_index = 0;
  new_pipe->write_index = 0;
  new_pipe->read_wait = nullptr;
  new_pipe->write_wait = nullptr;
  new_pipe->num_references = 2;

  struct file *new_file1 = (struct file *)kmalloc(sizeof(struct file));
  new_file1->read_write_pipe = new_pipe;
  new_file1->path = nullptr;
  new_file1->buffer = nullptr;
  new_file1->num_references = 1;

  struct file_descriptor *new_fd1 =
      (struct file_descriptor *)kmalloc(sizeof(struct file_descriptor));
  new_fd1->num = current_process->next_file_descriptor;
  current_process->next_file_descriptor++;
  new_fd1->file = new_file1;

  struct file *new_file2 = (struct file *)kmalloc(sizeof(struct file));
  new_file2->read_write_pipe = new_pipe;
  new_file2->path = nullptr;
  new_file2->buffer = nullptr;
  new_file2->num_references = 1;

  struct file_descriptor *new_fd2 =
      (struct file_descriptor *)kmalloc(sizeof(struct file_descriptor));
  new_fd2->num = current_process->next_file_descriptor;
  current_process->next_file_descriptor++;
  new_fd2->file = new_file2;

  new_fd2->prev = new_fd1;
  new_fd1->prev = nullptr;
  new_fd1->next = new_fd2;

  if (!current_process->open_files) {
    current_process->open_files = new_fd1;
    new_fd2->next = nullptr;
  } else {
    new_fd2->next = current_process->open_files;
    new_fd2->next->prev = new_fd2;
    current_process->open_files = new_fd1;
  }

  physical_to_virtual_memcpy(page_dir, (char *)&new_fd1->num, (char *)fd_addr,
                             sizeof(uint32_t));
  physical_to_virtual_memcpy(page_dir, (char *)&new_fd2->num,
                             (char *)fd_addr + sizeof(uint32_t),
                             sizeof(uint32_t));

  return 0;
}

} // namespace proc
