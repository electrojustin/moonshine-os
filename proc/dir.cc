#include "proc/dir.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"
#include "proc/read_write.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using filesystem::directory;
using filesystem::directory_entry;
using filesystem::ls_fat32;
using filesystem::mkdir_fat32;
using filesystem::stat_fat32;
using lib::std::kfree;
using lib::std::make_string_copy;
using lib::std::memcpy;
using lib::std::strcat;
using lib::std::strlen;

void cleanup_dir(int start, struct directory dir) {
  for (start; start < dir.num_children; start++) {
    kfree(dir.children[start].name);
  }
  kfree(dir.children);
}

} // namespace

uint32_t chdir(uint32_t path_addr, uint32_t reserved1, uint32_t reserved2,
               uint32_t reserved3, uint32_t reserved4) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *path = make_virtual_string_copy(page_dir, (char *)path_addr);

  if (path[0] != '/') {
    char *tmp = path;
    path = strcat(current_process->working_dir, path);
    kfree(tmp);
  }

  struct directory_entry dir_entry = stat_fat32(path);

  if (!dir_entry.name) {
    kfree(path);
    return -1;
  }

  kfree(dir_entry.name);

  kfree(current_process->working_dir);

  if (path[strlen(path) - 1] != '/') {
    char *tmp = path;
    path = strcat(path, "/");
    kfree(tmp);
  }

  current_process->working_dir = make_string_copy(path);

  kfree(path);

  return 0;
}

uint32_t getdents(uint32_t file_descriptor, uint32_t output, uint32_t count,
                  uint32_t reserved1, uint32_t reserved2) {
  return read(file_descriptor, output, count, 0, 0);
}

uint32_t mkdir(uint32_t path_addr, uint32_t mode, uint32_t reserved1,
               uint32_t reserved2, uint32_t reserved3) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *path = make_virtual_string_copy(page_dir, (char *)path_addr);

  if (path[0] != '/') {
    char *tmp = path;
    path = strcat(current_process->working_dir, path);
    kfree(tmp);
  }

  uint32_t ret = !mkdir_fat32(path);

  kfree(path);

  return ret;
}

} // namespace proc
