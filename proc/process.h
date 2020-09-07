#ifndef PROC_PROCESS_H
#define PROC_PROCESS_H

#include <stddef.h>
#include <stdint.h>

#include "arch/i386/memory/gdt.h"
#include "filesystem/file.h"

namespace proc {

namespace {

using arch::memory::tls_segment;
using filesystem::file;

} // namespace

enum state { STOPPED = 0, WAITING = 1, RUNNABLE = 2, NEW = 3 };

constexpr uint32_t EXECUTABLE_MEMORY = 0x1;
constexpr uint32_t WRITEABLE_MEMORY = 0x2;
constexpr uint32_t READABLE_MEMORY = 0x4;

struct process_memory_segment {
  void *actual_address; // For internal use only, must be page aligned
  void *virtual_address;
  size_t segment_size;
  size_t alloc_size;
  void *source = nullptr; // May be null
  size_t disk_size = 0;   // May be 0
  uint32_t flags;
};

struct wait_reason {
  uint32_t type;
};

struct process {
  uint32_t pid;

  char *path;
  char *working_dir;

  uint32_t *page_dir;               // Must be page aligned!
  uint32_t **page_tables = nullptr; // Must be page aligned!
  size_t num_page_tables = 0;

  void (*entry)(void);

  struct process_memory_segment *segments;
  uint32_t num_segments;

  uint32_t esp; // Saved esp from the process

  uint32_t kernel_stack_top;

  struct tls_segment *tls_segments;
  int num_tls_segments;
  int tls_segment_index;

  uint32_t actual_brk;
  uint32_t brk;

  uint32_t lower_brk;

  int argc;
  char **argv;

  struct file *open_files = nullptr;
  uint32_t next_file_descriptor = 2;

  enum state process_state;

  struct wait_reason *wait;

  struct process *next;
  struct process *prev;
};

constexpr uint32_t DEFAULT_CODE_START = 0x80000000;
constexpr uint32_t DEFAULT_STACK_BOTTOM = 0xC0000000;
constexpr uint32_t DEFAULT_STACK_SIZE = 0x10000;

char spawn_new_process(char *path, int argc, char **argv,
                       struct process_memory_segment *segments,
                       uint32_t num_segments, void (*entry_address)(void),
                       char *working_dir = "/");

void execute_processes(void);

struct process *get_currently_executing_process(void);

void advance_process_queue(void);

void set_userspace_page_table(void);

uint32_t assign_pid(void);

void kill_current_process(void);

} // namespace proc

#endif
