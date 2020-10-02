#include "proc/seek.h"
#include "filesystem/file.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace proc {

namespace {

using filesystem::file;
using filesystem::file_descriptor;
using filesystem::find_file_descriptor;
using proc::get_currently_executing_process;

} // namespace

uint32_t lseek(uint32_t file_descriptor, uint32_t offset, uint32_t whence,
               uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
  struct process *current_process = get_currently_executing_process();

  struct file_descriptor *to_seek = nullptr;

  if (file_descriptor == 0) {
    to_seek = current_process->standard_in;
  } else if (file_descriptor == 1) {
    to_seek = current_process->standard_out;
  } else if (file_descriptor == 2) {
    to_seek = current_process->standard_error;
  } else {
    to_seek =
        find_file_descriptor(file_descriptor, current_process->open_files);
  }

  if (!to_seek || to_seek->file->read_write_pipe) {
    return -1;
  }

  switch (whence) {
  case SEEK_SET:
    to_seek->file->offset = offset;
    return 0;
  case SEEK_CUR:
    to_seek->file->offset += offset;
    return 0;
  case SEEK_END:
    to_seek->file->offset = to_seek->file->size + offset;
    return 0;
  default:
    return -1;
  }
}

uint32_t llseek(uint32_t file_descriptor, uint32_t offset_high,
                uint32_t offset_low, uint32_t result_addr, uint32_t whence,
                uint32_t reserved1) {
  // 64 bit offsets are unsupported
  return lseek(file_descriptor, offset_low, whence, 0, 0, 0);
}

} // namespace proc
