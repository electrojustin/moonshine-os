#ifndef FILESYSTEM_PIPE_H
#define FILESYSTEM_PIPE_H

#include <stdint.h>

#include "proc/process.h"

namespace filesystem {

namespace {

using proc::process;
using proc::wait_reason;

} // namespace

constexpr uint32_t PIPE_MAX_SIZE = 4096;
constexpr uint32_t PIPE_READ_WAIT = 0x3;
constexpr uint32_t PIPE_WRITE_WAIT = 0x4;

struct pipe_read_wait;
struct pipe_write_wait;

struct pipe {
  uint8_t buf[PIPE_MAX_SIZE];
  uint32_t read_index;
  uint32_t write_index;
  struct pipe_read_wait *read_wait;
  struct pipe_write_wait *write_wait;
  uint8_t num_references;
};

struct pipe_read_wait : wait_reason {
  struct pipe *to_read;
  struct process *client;
  uint8_t *buf; // Virtual address space
  uint32_t index;
  uint32_t len;
};

struct pipe_write_wait : wait_reason {
  struct pipe *to_write;
  struct process *client;
  uint8_t *buf; // Virtual address space
  uint32_t index;
  uint32_t len;
  struct pipe_write_wait *next;
};

void write_to_pipe(struct process *current_process, struct pipe *write_pipe,
                   uint8_t *buf, uint32_t size);

void read_from_pipe(struct process *current_process, struct pipe *read_pipe,
                    uint8_t *buf, uint32_t size);

} // namespace filesystem

#endif
