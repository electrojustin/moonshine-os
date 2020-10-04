#include <stdint.h>

#include "arch/i386/memory/paging.h"
#include "filesystem/file.h"
#include "filesystem/pipe.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/elf_loader.h"
#include "proc/execve.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using arch::memory::virtual_to_physical_memcpy;
using filesystem::file;
using filesystem::pipe;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::make_string_copy;
using lib::std::strcat;

} // namespace

uint32_t execve(uint32_t path_addr, uint32_t argv_addr, uint32_t envp_addr,
                uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
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
  char *current_argv = nullptr;
  virtual_to_physical_memcpy(page_dir, (char *)current_argv_addr,
                             (char *)&current_argv, sizeof(char *));
  while (current_argv) {
    current_argv_addr += sizeof(char *);
    argc++;
    virtual_to_physical_memcpy(page_dir, (char *)current_argv_addr,
                               (char *)&current_argv, sizeof(char *));
  }
  char **argv = (char **)kmalloc((argc + 1) * sizeof(char *));
  current_argv_addr = argv_addr;
  for (int i = 1; i < argc; i++) {
    current_argv = nullptr;
    virtual_to_physical_memcpy(page_dir, (char *)current_argv_addr,
                               (char *)&current_argv, sizeof(char *));
    argv[i] = make_virtual_string_copy(page_dir, current_argv);
    current_argv_addr += sizeof(char *);
  }
  argv[0] = make_string_copy(relative_path);
  argv[argc] = nullptr;

  char **envp = nullptr;
  if (envp_addr) {
    int envc = 0;
    uint32_t current_envp_addr = envp_addr;
    char *current_envp = nullptr;
    virtual_to_physical_memcpy(page_dir, (char *)current_envp_addr,
                               (char *)&current_envp, sizeof(char *));
    while (current_envp) {
      current_envp_addr += sizeof(char *);
      envc++;
      virtual_to_physical_memcpy(page_dir, (char *)current_envp_addr,
                                 (char *)&current_envp, sizeof(char *));
    }
    envp = (char **)kmalloc((envc + 1) * sizeof(char *));
    current_envp_addr = envp_addr;
    for (int i = 0; i < envc; i++) {
      current_envp = nullptr;
      virtual_to_physical_memcpy(page_dir, (char *)current_envp_addr,
                                 (char *)&current_envp, sizeof(char *));
      envp[i] = make_virtual_string_copy(page_dir, current_envp);
      current_envp_addr += sizeof(char *);
    }
    envp[envc] = nullptr;
  }

  struct file_descriptor *standard_in = current_process->standard_in;
  struct file_descriptor *standard_out = current_process->standard_out;
  struct file_descriptor *standard_error = current_process->standard_error;
  struct file_descriptor *open_files = current_process->open_files;
  uint32_t next_file_descriptor = current_process->next_file_descriptor;

  current_process->standard_in = nullptr;
  current_process->standard_out = nullptr;
  current_process->standard_error = nullptr;
  current_process->open_files = nullptr;

  if (!load_elf(relative_path, argc, argv, envp, current_process->working_dir,
                standard_in, standard_out, standard_error, open_files,
                next_file_descriptor)) {
    return -1;
  }

  current_process->process_state = STOPPED;

  kfree(relative_path);

  return 0;
}

} // namespace proc
