#ifndef PROC_CLOSE_H
#define PROC_CLOSE_H

#include <stdint.h>

#include "filesystem/file.h"

namespace proc {

namespace {

using filesystem::file;

} // namespace

void close_file(struct process *current_process, struct file *to_close);

uint32_t close(uint32_t file_descriptor, uint32_t reserved1, uint32_t reserved2,
               uint32_t reserved3, uint32_t reserved4);

} // namespace proc

#endif
