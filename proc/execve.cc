#include <stdint.h>

#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/elf_loader.h"
#include "proc/execve.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using arch::memory::virtual_to_physical;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::make_string_copy;
using lib::std::strcat;

} // namespace

uint32_t execve(uint32_t path_addr, uint32_t argv_addr, uint32_t env_addr,
                uint32_t reserved1, uint32_t reserved2) {
  if (!path_addr || !argv_addr) {
    return -1;
  }

  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *relative_path = make_virtual_string_copy(page_dir, (char *)path_addr);

  if (relative_path[0] != '/') {
    char *tmp = relative_path;
    relative_path = strcat(current_process->working_dir, relative_path);
    kfree(tmp);
  }

  int argc = 0;
  uint32_t current_argv_addr = argv_addr;
  while (*(char **)virtual_to_physical(page_dir, (void *)current_argv_addr)) {
    current_argv_addr += sizeof(char *);
    argc++;
  }
  char **argv = (char **)kmalloc((argc + 1) * sizeof(char *));
  current_argv_addr = argv_addr;
  for (int i = 0; i < argc; i++) {
    char *current_argv =
        *(char **)virtual_to_physical(page_dir, (void *)current_argv_addr);
    argv[i] = make_virtual_string_copy(page_dir, current_argv);
    current_argv_addr += sizeof(char *);
  }
  argv[argc] = nullptr;

  if (!load_elf(relative_path, argc, argv,
                make_string_copy(current_process->working_dir))) {
    return -1;
  }

  current_process->process_state = STOPPED;

  kfree(relative_path);

  return 0;
}

} // namespace proc
