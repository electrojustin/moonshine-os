#ifndef PROC_READ_WRITE_H
#define PROC_READ_WRITE_H

#include <stdint.h>

#include "filesystem/file.h"

namespace proc {

namespace {

using filesystem::file;

} // namespace

struct __attribute__((packed)) io_vector {
  char *buf;
  uint32_t len;
};

uint32_t read(uint32_t file_descriptor, uint32_t dest, uint32_t size,
              uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

uint32_t readlink(uint32_t path_addr, uint32_t buf_addr, uint32_t len,
                  uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

uint32_t write(uint32_t file_descriptor, uint32_t src, uint32_t size,
               uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

uint32_t writev(uint32_t file_descriptor, uint32_t io_vector_addr,
                uint32_t vector_len, uint32_t reserved1, uint32_t reserved2,
                uint32_t reserved3);

uint32_t unlink(uint32_t path_addr, uint32_t reserved1, uint32_t reserved2,
                uint32_t reserved3, uint32_t reserved4, uint32_t reserved5);

uint32_t flush(struct file *to_flush);

} // namespace proc

#endif
