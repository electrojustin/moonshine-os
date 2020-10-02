#ifndef PROC_DUP_H
#define PROC_DUP_H

#include "filesystem/file.h"

namespace proc {

namespace {

using filesystem::file_descriptor;

} // namespace

struct file_descriptor *duplicate(struct file_descriptor *to_duplicate,
                                  uint32_t new_number);

uint32_t dup2(uint32_t old_fd, uint32_t new_fd, uint32_t reserved1,
              uint32_t reserved2, uint32_t reserved3, uint32_t reserved4);

} // namespace proc

#endif
