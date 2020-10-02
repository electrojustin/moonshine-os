#include "proc/close.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/file.h"
#include "filesystem/pipe.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::flush_pages;
using filesystem::file;
using filesystem::file_descriptor;
using filesystem::pipe;
using lib::std::kfree;

} // namespace

void close_file(struct file *to_close) {
  if (to_close->read_write_pipe) {
    to_close->read_write_pipe->num_references--;
    if (!to_close->read_write_pipe->num_references) {
      kfree(to_close->read_write_pipe);
    }
  } else {
    kfree(to_close->path);
    if (to_close->buffer) {
      kfree(to_close->buffer);
    }
  }

  kfree(to_close);
}

void close_file_descriptor(struct file_descriptor *to_close) {
  to_close->file->num_references--;
  if (to_close->file->num_references == 0) {
    close_file(to_close->file);
  }
  kfree(to_close);
}

uint32_t close(uint32_t file_descriptor, uint32_t reserved1, uint32_t reserved2,
               uint32_t reserved3, uint32_t reserved4, uint32_t reserved5) {
  struct process *current_process = get_currently_executing_process();

  if (file_descriptor == 0) {
    if (current_process->standard_in) {
      close_file_descriptor(current_process->standard_in);
      current_process->standard_in = nullptr;
    } else {
      return -1;
    }
  } else if (file_descriptor == 1) {
    if (current_process->standard_out) {
      close_file_descriptor(current_process->standard_out);
      current_process->standard_out = nullptr;
    } else {
      return -1;
    }
  } else if (file_descriptor == 2) {
    if (current_process->standard_error) {
      close_file_descriptor(current_process->standard_error);
      current_process->standard_error = nullptr;
    } else {
      return -1;
    }
  }

  struct file_descriptor *to_close =
      find_file_descriptor(file_descriptor, current_process->open_files);

  if (!to_close) {
    return -1;
  }

  if (to_close->prev) {
    to_close->prev->next = to_close->next;
  }
  if (to_close->next) {
    to_close->next->prev = to_close->prev;
  }
  if (current_process->open_files == to_close) {
    current_process->open_files = to_close->next;
  }
  close_file_descriptor(to_close);

  return 0;
}

} // namespace proc
