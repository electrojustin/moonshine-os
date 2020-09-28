#include <stdint.h>

#include "arch/i386/cpu/sse.h"
#include "arch/i386/memory/gdt.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/file.h"
#include "filesystem/pipe.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/fork.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::cpu::is_sse_enabled;
using arch::memory::copy_mapping;
using arch::memory::flush_pages;
using arch::memory::map_memory_segment;
using arch::memory::PAGE_SIZE;
using arch::memory::permission;
using arch::memory::tls_segment;
using arch::memory::user_read_only;
using arch::memory::user_read_write;
using arch::memory::virtual_to_physical;
using filesystem::file;
using filesystem::file_mapping;
using filesystem::pipe;
using lib::std::kmalloc;
using lib::std::kmalloc_aligned;
using lib::std::make_string_copy;
using lib::std::memcpy;
using lib::std::memset;

void insert_file(struct process *proc, struct file *to_insert) {
  to_insert->prev = nullptr;
  to_insert->next = proc->open_files;

  proc->open_files = to_insert;
}

} // namespace

uint32_t fork(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
              uint32_t reserved4, uint32_t reserved5, uint32_t reserved6) {
  struct process *parent_proc = get_currently_executing_process();
  struct process *new_proc = (struct process *)kmalloc(sizeof(struct process));

  new_proc->pid = assign_pid();

  new_proc->path = make_string_copy(parent_proc->path);
  new_proc->working_dir = make_string_copy(parent_proc->working_dir);

  new_proc->page_dir =
      (uint32_t *)kmalloc_aligned(1024 * sizeof(uint32_t), PAGE_SIZE);
  memcpy((char *)base_page_directory, (char *)new_proc->page_dir,
         2 * sizeof(uint32_t));
  memset((char *)(new_proc->page_dir + 2), 1022 * sizeof(uint32_t), 0);
  new_proc->page_tables = nullptr;
  new_proc->num_page_tables = 0;

  new_proc->entry = parent_proc->entry;

  struct file *current_file = parent_proc->open_files;
  while (current_file) {
    struct file_mapping *mapping = current_file->mappings;
    while (mapping) {
      map_memory_segment(new_proc, 0, (uint32_t)mapping->mapping,
                         mapping->mapping_len, user_read_write);
      mapping = mapping->next;
    }
    current_file = current_file->next;
  }

  new_proc->num_segments = parent_proc->num_segments;
  new_proc->segments = (struct process_memory_segment *)kmalloc(
      new_proc->num_segments * sizeof(struct process_memory_segment));
  for (int i = 0; i < new_proc->num_segments; i++) {
    new_proc->segments[i].flags = parent_proc->segments[i].flags;
    new_proc->segments[i].virtual_address =
        parent_proc->segments[i].virtual_address;
    new_proc->segments[i].segment_size = parent_proc->segments[i].segment_size;
    new_proc->segments[i].alloc_size = parent_proc->segments[i].alloc_size;

    new_proc->segments[i].actual_address =
        kmalloc_aligned(parent_proc->segments[i].alloc_size, PAGE_SIZE);
    memcpy((char *)parent_proc->segments[i].actual_address,
           (char *)new_proc->segments[i].actual_address,
           new_proc->segments[i].alloc_size);

    uint32_t virtual_address = (uint32_t)new_proc->segments[i].virtual_address;
    if (!virtual_address) {
      virtual_address = (uint32_t)new_proc->segments[i].actual_address;
      new_proc->kernel_stack_top =
          (virtual_address + new_proc->segments[i].segment_size) & 0xFFFFFFFC;
    }
    size_t page_offset = virtual_address & (PAGE_SIZE - 1);
    map_memory_segment(new_proc, (uint32_t)new_proc->segments[i].actual_address,
                       virtual_address - page_offset,
                       new_proc->segments[i].alloc_size, user_read_write);
  }

  new_proc->esp = new_proc->kernel_stack_top -
                  (parent_proc->kernel_stack_top - parent_proc->esp);

  new_proc->num_tls_segments = parent_proc->num_tls_segments;
  new_proc->tls_segments = (struct tls_segment *)kmalloc(
      sizeof(struct tls_segment) * new_proc->num_tls_segments);
  memcpy((char *)parent_proc->tls_segments, (char *)new_proc->tls_segments,
         sizeof(struct tls_segment) * new_proc->num_tls_segments);
  new_proc->tls_segment_index = parent_proc->tls_segment_index;

  new_proc->actual_brk = parent_proc->actual_brk;
  new_proc->brk = parent_proc->brk;
  new_proc->lower_brk = parent_proc->lower_brk;

  new_proc->argc = parent_proc->argc;
  new_proc->argv = (char **)kmalloc(new_proc->argc * sizeof(char *));
  for (int i = 0; i < new_proc->argc; i++) {
    new_proc->argv[i] = make_string_copy(parent_proc->argv[i]);
  }

  new_proc->next_file_descriptor = parent_proc->next_file_descriptor;
  current_file = parent_proc->open_files;
  new_proc->open_files = nullptr;
  while (current_file) {
    if (current_file->read_write_pipe) {
      current_file->read_write_pipe->num_references += 2;
    }

    struct file *new_file = (struct file *)kmalloc(sizeof(struct file));
    memcpy((char *)current_file, (char *)new_file, sizeof(struct file));
    new_file->path = make_string_copy(new_file->path);
    insert_file(new_proc, new_file);

    new_file->mappings = nullptr;
    struct file_mapping *current_mapping = current_file->mappings;
    struct file_mapping *current_new_mapping = nullptr;
    while (current_mapping) {
      struct file_mapping *new_mapping =
          (struct file_mapping *)kmalloc(sizeof(struct file_mapping));
      new_mapping->mapping = current_mapping->mapping;
      new_mapping->mapping_len = current_mapping->mapping_len;
      new_mapping->offset = current_mapping->offset;
      new_mapping->is_private = current_mapping->is_private;
      new_mapping->next = nullptr;
      new_mapping->prev = current_new_mapping;

      copy_mapping(parent_proc, new_proc, new_mapping);

      if (!new_file->mappings) {
        new_file->mappings = new_mapping;
      } else {
        current_new_mapping->next = new_mapping;
      }

      current_new_mapping = new_mapping;

      current_mapping = current_mapping->next;
    }

    current_file = current_file->next;
  }

  new_proc->process_state = RUNNABLE;

  new_proc->wait = nullptr;

  parent_proc->next->prev = new_proc;
  new_proc->next = parent_proc->next;
  parent_proc->next = new_proc;
  new_proc->prev = parent_proc;

  // We don't need to do any gymnastics with virtual and physical memory here
  // because we always identity page the kernel stack. But, we gotta fix any
  // saved kernel stack pointers since the addresses will be different.
  uint32_t *esp_actual = (uint32_t *)new_proc->esp;
  if (is_sse_enabled) {
    uint32_t old_esp_actual = *(uint32_t *)parent_proc->esp;
    *esp_actual = new_proc->kernel_stack_top -
                  (parent_proc->kernel_stack_top - old_esp_actual);
    esp_actual = (uint32_t *)*esp_actual;
  }
  esp_actual[7] = 0;

  advance_process_queue();

  return parent_proc->pid;
}

uint32_t clone(uint32_t flags, uint32_t stack_addr, uint32_t parent_tid_addr,
               uint32_t tls, uint32_t child_tid_addr, uint32_t reserved1) {
  return fork(0, 0, 0, 0, 0, 0);
}

} // namespace proc
