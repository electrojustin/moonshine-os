#include "filesystem/pipe.h"
#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace filesystem {

namespace {

using arch::memory::virtual_to_physical;
using arch::memory::virtual_to_virtual_memcpy;
using lib::std::kfree;
using lib::std::kmalloc;
using proc::process;
using proc::RUNNABLE;
using proc::wait_reason;
using proc::WAITING;

} // namespace

void write_to_pipe(struct process *current_process, struct pipe *write_pipe,
                   uint8_t *buf, uint32_t size) {
  uint32_t *page_dir = current_process->page_dir;

  // If someone is waiting on this information, write directly to them
  if (write_pipe->read_wait && size) {
    struct pipe_read_wait *read_wait = write_pipe->read_wait;
    uint32_t max_read_size = read_wait->len - read_wait->index;
    uint32_t read_size = size < max_read_size ? size : max_read_size;
    uint32_t *read_client_page_dir = read_wait->client->page_dir;

    virtual_to_virtual_memcpy(page_dir, read_client_page_dir, (char *)buf,
                              (char *)read_wait->buf + read_wait->index,
                              read_size);

    // Unblock reading process if it's satisfied
    read_wait->index += read_size;
    if (read_wait->len == read_wait->index) {
      read_wait->client->process_state = RUNNABLE;
      kfree(read_wait);
      current_process->wait = nullptr;
      write_pipe->read_wait = nullptr;
    }

    size -= read_size;
    buf += read_size;
  }

  // Buffer as much as we can
  while (size && (write_pipe->write_index + 1) % PIPE_MAX_SIZE !=
                     write_pipe->read_index) {
    write_pipe->buf[write_pipe->write_index] =
        *(uint8_t *)virtual_to_physical(page_dir, buf);
    buf++;
    write_pipe->write_index = (write_pipe->write_index + 1) % PIPE_MAX_SIZE;
    size--;
  }

  // If the buf is full, then block
  if (size) {
    struct pipe_write_wait *write_wait =
        (struct pipe_write_wait *)kmalloc(sizeof(struct pipe_write_wait));
    write_wait->type = PIPE_WRITE_WAIT;
    write_wait->to_write = write_pipe;
    write_wait->client = current_process;
    write_wait->buf = buf;
    write_wait->index = 0;
    write_wait->len = size;
    write_wait->next = nullptr;

    if (!write_pipe->write_wait) {
      write_pipe->write_wait = write_wait;
      current_process->wait = write_pipe->write_wait;
      current_process->process_state = WAITING;
    } else {
      // Chain this new wait to the end of the wait list.
      // Mostly used to support writev.
      struct pipe_write_wait *current_write_wait = write_pipe->write_wait;
      while (current_write_wait->next) {
        current_write_wait = current_write_wait->next;
      }
      current_write_wait->next = write_wait;
    }
  }
}

void read_from_pipe(struct process *current_process, struct pipe *read_pipe,
                    uint8_t *buf, uint32_t size) {
  uint32_t *page_dir = current_process->page_dir;

  // Read from the buf until it's empty
  while (size && read_pipe->read_index != read_pipe->write_index) {
    *(uint8_t *)virtual_to_physical(page_dir, buf) =
        read_pipe->buf[read_pipe->read_index];
    buf++;
    read_pipe->read_index = (read_pipe->read_index + 1) % PIPE_MAX_SIZE;
    size--;
  }

  // Re-buf the writing process, and possibly read from it
  while (read_pipe->write_wait) {
    struct pipe_write_wait *write_wait = read_pipe->write_wait;
    if (size) {
      uint32_t max_read_size = write_wait->len - write_wait->index;
      uint32_t read_size = size < max_read_size ? size : max_read_size;
      uint32_t *write_client_page_dir = write_wait->client->page_dir;

      virtual_to_virtual_memcpy(write_client_page_dir, page_dir, (char *)buf,
                                (char *)write_wait->buf + write_wait->index,
                                read_size);

      size -= read_size;
      buf += read_size;
      write_wait->index += read_size;
    }

    uint32_t write_index = write_wait->index;
    uint32_t write_len = write_wait->len;
    struct process *write_process = write_wait->client;
    uint8_t *write_buf = write_wait->buf;

    if (write_len == write_index) {
      read_pipe->write_wait = read_pipe->write_wait->next;
      write_process->wait = read_pipe->write_wait;
      kfree(write_wait);
      if (!write_process->wait) {
        write_process->process_state = RUNNABLE;
      }
    } else {
      // We go through this function again to handle the bufing logic
      read_pipe->write_wait = nullptr;
      write_process->wait = nullptr;
      write_process->process_state = RUNNABLE;
      write_to_pipe(write_process, read_pipe, write_buf + write_index,
                    write_len - write_index);
      if (read_pipe->write_wait) {
        // Buffer filled. Merge the write waits into the new list;
        read_pipe->write_wait->next = write_wait->next;
        kfree(write_wait);
        break;
      } else {
        read_pipe->write_wait = write_wait->next;
        write_process->wait = write_wait->next;
      }
      kfree(write_wait);
    }
  }

  // If the buf is empty and there isn't a writing process, then block
  if (size) {
    struct pipe_read_wait *read_wait =
        (struct pipe_read_wait *)kmalloc(sizeof(struct pipe_read_wait));
    read_wait->type = PIPE_READ_WAIT;
    read_wait->to_read = read_pipe;
    read_wait->client = current_process;
    read_wait->buf = buf;
    read_wait->index = 0;
    read_wait->len = size;

    read_pipe->read_wait = read_wait;

    current_process->wait = read_wait;
    current_process->process_state = WAITING;
  }
}

} // namespace filesystem
