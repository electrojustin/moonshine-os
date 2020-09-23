#include "proc/close.h"
#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::flush_pages;
using filesystem::file;
using filesystem::file_mapping;
using lib::std::kfree;

} // namespace

void close_file(struct process *current_process, struct file *to_close) {
  struct file_mapping *mapping = to_close->mappings;
  while (mapping) {
    struct file_mapping *next = mapping->next;
    flush_pages(current_process->page_dir, to_close, mapping);
    kfree(mapping);
    mapping = next;
  }

  kfree(to_close->path);
  if (to_close->buffer) {
    kfree(to_close->buffer);
  }

  if (!to_close->next && !to_close->prev) {
    current_process->open_files = nullptr;
  } else if (!to_close->prev) {
    current_process->open_files = to_close->next;
    to_close->next->prev = nullptr;
  } else if (!to_close->next) {
    to_close->prev->next = nullptr;
  } else {
    to_close->prev->next = to_close->next;
    to_close->next->prev = to_close->prev;
  }

  kfree(to_close);
}

uint32_t close(uint32_t file_descriptor, uint32_t reserved1, uint32_t reserved2,
               uint32_t reserved3, uint32_t reserved4, uint32_t reserved5) {
  struct process *current_process = get_currently_executing_process();

  struct file *current_file = current_process->open_files;
  while (current_file) {
    if (current_file->file_descriptor == file_descriptor) {
      if (!current_file->mappings) {
        // Mappings are supposed to persist after the file closes
        close_file(current_process, current_file);
      } else {
        current_file->can_free = 1;
      }
      return 0;
    }

    current_file = current_file->next;
  }

  return -1;
}

} // namespace proc
