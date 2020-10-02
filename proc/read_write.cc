#include "proc/read_write.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "filesystem/file.h"
#include "filesystem/pipe.h"
#include "io/keyboard.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace proc {

namespace {

using arch::memory::make_virtual_string_copy;
using arch::memory::physical_to_virtual_memcpy;
using arch::memory::virtual_to_physical;
using arch::memory::virtual_to_physical_memcpy;
using filesystem::del_fat32;
using filesystem::file;
using filesystem::file_descriptor;
using filesystem::read_fat32;
using filesystem::read_from_pipe;
using filesystem::write_fat32;
using filesystem::write_new_fat32;
using filesystem::write_to_pipe;
using io::KEYBOARD_WAIT;
using io::keyboard_wait;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::krealloc;
using lib::std::memcpy;
using lib::std::putc;
using lib::std::streq;
using lib::std::strlen;

uint32_t write_file(struct process *current_process, struct file *to_write,
                    uint8_t *virtual_buf, uint32_t size) {
  if (to_write->read_write_pipe) {
    write_to_pipe(current_process, to_write->read_write_pipe, virtual_buf,
                  size);
    return size;
  } else {
    uint8_t *buf = (uint8_t *)kmalloc(size);
    virtual_to_physical_memcpy(current_process->page_dir, (char *)virtual_buf,
                               (char *)buf, size);

    if (!to_write->inode) {
      to_write->inode = write_new_fat32(to_write->path, 0, buf, size);
      to_write->size = size;
      to_write->offset = size;
    } else {
      size_t new_size =
          write_fat32(to_write->path, to_write->offset, buf, size);
      if (new_size > 1) {
        to_write->size = new_size;
      }
      to_write->offset += size;
    }

    kfree(buf);
  }

  return size;
}

uint32_t write_internal(struct process *current_process,
                        uint32_t file_descriptor, uint8_t *virtual_buf,
                        uint32_t size) {
  uint32_t *page_dir = current_process->page_dir;
  if (file_descriptor < 3) {
    if (file_descriptor == 2) {
      if (current_process->standard_error) {
        return write_file(current_process,
                          current_process->standard_error->file, virtual_buf,
                          size);
      }
    } else if (file_descriptor == 1) {
      if (current_process->standard_out) {
        return write_file(current_process, current_process->standard_out->file,
                          virtual_buf, size);
      }
    } else {
      return -1;
    }

    for (int i = 0; i < size; i++) {
      putc(*(char *)virtual_to_physical(page_dir, virtual_buf + i));
    }

    return size;
  } else {
    struct file_descriptor *to_write =
        find_file_descriptor(file_descriptor, current_process->open_files);

    if (!to_write) {
      return -1;
    }

    return write_file(current_process, to_write->file, virtual_buf, size);
  }
}

uint32_t read_from_keyboard(struct process *current_process, char *buf,
                            uint32_t size) {
  struct keyboard_wait *wait =
      (struct keyboard_wait *)kmalloc(sizeof(struct keyboard_wait));
  wait->type = KEYBOARD_WAIT;
  wait->buf = buf;
  wait->index = 0;
  wait->len = size;

  current_process->process_state = WAITING;
  current_process->wait = wait;

  return size;
}

uint32_t read_file(struct process *current_process, struct file *to_read,
                   uint32_t dest, uint32_t size, char use_seek,
                   uint32_t offset) {
  offset = use_seek ? to_read->offset : offset;

  uint32_t read_size = 0;
  if (to_read->read_write_pipe) {
    read_from_pipe(current_process, to_read->read_write_pipe, (uint8_t *)dest,
                   size);
    return size;
  } else {
    if (to_read->buffer) {
      read_size =
          size < (to_read->size - offset) ? size : (to_read->size - offset);
      if (read_size) {
        physical_to_virtual_memcpy(current_process->page_dir, to_read->buffer,
                                   (char *)dest, read_size);
      }
    } else if (to_read->inode) {
      uint8_t *temp_buf = (uint8_t *)kmalloc(size);
      read_size = read_fat32(to_read->inode, offset, temp_buf, size);
      if (read_size) {
        physical_to_virtual_memcpy(current_process->page_dir, (char *)temp_buf,
                                   (char *)dest, read_size);
      }
      kfree(temp_buf);
    } else {
      return -1;
    }

    if (use_seek) {
      to_read->offset += read_size;
    }

    return read_size;
  }
}

uint32_t read_internal(uint32_t file_descriptor, uint32_t dest, uint32_t size,
                       char use_seek, uint32_t offset) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;

  if (file_descriptor < 3) {
    if (file_descriptor == 0) {
      if (current_process->standard_in) {
        return read_file(current_process, current_process->standard_in->file,
                         dest, size, use_seek, offset);
      }
    } else {
      return -1;
    }

    return read_from_keyboard(current_process, (char *)dest, size);
  } else {
    struct file_descriptor *to_read =
        find_file_descriptor(file_descriptor, current_process->open_files);

    if (!to_read) {
      return -1;
    }

    return read_file(current_process, to_read->file, dest, size, use_seek,
                     offset);
  }
}

} // namespace

uint32_t read(uint32_t file_descriptor, uint32_t dest, uint32_t size,
              uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
  read_internal(file_descriptor, dest, size, 1, 0);
}

uint32_t pread64(uint32_t file_descriptor, uint32_t dest, uint32_t size,
                 uint32_t offset, uint32_t reserved1, uint32_t reserved2) {
  read_internal(file_descriptor, dest, size, 0, offset);
}

uint32_t readlink(uint32_t path_addr, uint32_t buf_addr, uint32_t len,
                  uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *path = make_virtual_string_copy(page_dir, (char *)path_addr);

  if (streq("/proc/self/exe", path, strlen("/proc/self/exe"))) {
    size_t executable_path_len = strlen(current_process->path) + 1;
    size_t copy_len = executable_path_len < len ? executable_path_len : len;
    physical_to_virtual_memcpy(page_dir, current_process->path,
                               (char *)buf_addr, copy_len);
    kfree(path);
    return copy_len;
  }

  kfree(path);
  return -1;
}

uint32_t write(uint32_t file_descriptor, uint32_t src, uint32_t size,
               uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;

  return write_internal(current_process, file_descriptor, (uint8_t *)src, size);
}

uint32_t writev(uint32_t file_descriptor, uint32_t io_vector_addr,
                uint32_t vector_len, uint32_t reserved1, uint32_t reserved2,
                uint32_t reserved3) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  struct io_vector *vector =
      (struct io_vector *)kmalloc(sizeof(struct io_vector) * vector_len);
  virtual_to_physical_memcpy(page_dir, (char *)io_vector_addr, (char *)vector,
                             sizeof(struct io_vector) * vector_len);

  uint32_t ret = 0;
  for (int i = 0; i < vector_len; i++) {
    ret += write_internal(current_process, file_descriptor,
                          (uint8_t *)vector[i].buf, vector[i].len);
  }

  return ret;
}

uint32_t unlink(uint32_t path_addr, uint32_t reserved1, uint32_t reserved2,
                uint32_t reserved3, uint32_t reserved4, uint32_t reserved5) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;
  char *path = make_virtual_string_copy(page_dir, (char *)path_addr);
  del_fat32(path);
  kfree(path);
  return 0;
}

} // namespace proc
