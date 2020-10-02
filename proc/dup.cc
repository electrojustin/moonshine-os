#include "proc/dup.h"
#include "filesystem/file.h"
#include "filesystem/pipe.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/close.h"
#include "proc/process.h"

namespace proc {

namespace {

using filesystem::file;
using filesystem::file_descriptor;
using filesystem::find_file_descriptor;
using filesystem::pipe;
using lib::std::kmalloc;

} // namespace

struct file_descriptor *duplicate(struct file_descriptor *to_duplicate,
                                  uint32_t new_number) {
  struct file_descriptor *ret =
      (struct file_descriptor *)kmalloc(sizeof(struct file_descriptor));
  ret->file = to_duplicate->file;
  ret->num = new_number;
  ret->prev = nullptr;
  ret->next = nullptr;

  ret->file->num_references++;
  if (ret->file->read_write_pipe) {
    ret->file->read_write_pipe->num_references++;
  }

  return ret;
}

uint32_t dup2(uint32_t old_fd, uint32_t new_fd, uint32_t reserved1,
              uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  struct process *current_process = get_currently_executing_process();

  if (old_fd == new_fd) {
    return 0;
  }

  struct file_descriptor *old_descriptor = nullptr;

  if (old_fd == 0) {
    old_descriptor = current_process->standard_in;
  } else if (old_fd == 1) {
    old_descriptor = current_process->standard_out;
  } else if (old_fd == 2) {
    old_descriptor == current_process->standard_error;
  } else {
    old_descriptor = find_file_descriptor(old_fd, current_process->open_files);
  }

  if (!old_descriptor) {
    return -1;
  }

  struct file_descriptor *new_descriptor = duplicate(old_descriptor, new_fd);

  if (new_fd == 0) {
    if (current_process->standard_in) {
      close_file_descriptor(current_process->standard_in);
    }
    current_process->standard_in = new_descriptor;
  } else if (new_fd == 1) {
    if (current_process->standard_out) {
      close_file_descriptor(current_process->standard_out);
    }
    current_process->standard_out = new_descriptor;
  } else if (new_fd == 2) {
    if (current_process->standard_error) {
      close_file_descriptor(current_process->standard_error);
    }
    current_process->standard_error = new_descriptor;
  } else {
    struct file_descriptor *clobber_descriptor =
        find_file_descriptor(new_fd, current_process->open_files);

    if (clobber_descriptor) {
      if (clobber_descriptor->prev) {
        clobber_descriptor->prev->next = clobber_descriptor->next;
      }
      if (clobber_descriptor->next) {
        clobber_descriptor->next->prev = clobber_descriptor->prev;
      }
      if (current_process->open_files == clobber_descriptor) {
        current_process->open_files = clobber_descriptor->next;
      }
      close_file_descriptor(clobber_descriptor);
    }

    new_descriptor->prev = nullptr;
    new_descriptor->next = current_process->open_files;
    current_process->open_files = new_descriptor;
  }

  return 0;
}

} // namespace proc
