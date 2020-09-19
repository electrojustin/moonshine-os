#include "proc/mmap.h"
#include "arch/i386/memory/paging.h"
#include "filesystem/file.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "proc/process.h"

namespace proc {

namespace {
using arch::memory::flush_pages;
using arch::memory::map_memory_segment;
using arch::memory::PAGE_SIZE;
using arch::memory::unmap_memory_range;
using arch::memory::user_read_write;
using filesystem::file;
using filesystem::file_mapping;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::kmalloc_aligned;
using lib::std::krealloc;

uint32_t msync_internal(uint32_t req_addr, uint32_t len, uint32_t flags,
                        char should_delete_mapping) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;

  struct file *to_sync = current_process->open_files;
  struct file_mapping *current_mapping = nullptr;
  while (to_sync) {
    current_mapping = to_sync->mappings;
    while (current_mapping && (uint32_t)current_mapping->mapping != req_addr) {
      current_mapping = current_mapping->next;
    }

    if (current_mapping) {
      break;
    }

    to_sync = to_sync->next;
  }

  if (!to_sync) {
    return -1;
  }

  flush_pages(page_dir, to_sync, current_mapping);

  if (should_delete_mapping) {
    if (current_mapping->prev) {
      current_mapping->prev->next = current_mapping->next;
    } else {
      to_sync->mappings = current_mapping->next;
    }

    if (current_mapping->next) {
      current_mapping->next->prev = current_mapping->prev;
    }

    kfree(current_mapping);
  }

  return 0;
}
} // namespace

uint32_t mmap(uint32_t req_addr, uint32_t len, uint32_t prot, uint32_t flags,
              uint32_t file_descriptor, uint32_t offset) {
  struct process *current_process = get_currently_executing_process();
  uint32_t *page_dir = current_process->page_dir;

  if (!req_addr) {
    req_addr = (current_process->lower_brk - len) & (~(PAGE_SIZE - 1));
    current_process->lower_brk = req_addr;
  }

  if (!file_descriptor) {
    current_process->num_segments++;
    current_process->segments = (struct process_memory_segment *)krealloc(
        current_process->segments,
        current_process->num_segments * sizeof(struct process_memory_segment));
    struct process_memory_segment *new_segment =
        current_process->segments + current_process->num_segments - 1;
    size_t segment_size = len;
    segment_size = (segment_size & (PAGE_SIZE - 1))
                       ? ((segment_size / PAGE_SIZE) + 1) * PAGE_SIZE
                       : segment_size;
    new_segment->virtual_address = (void *)req_addr;
    new_segment->segment_size = segment_size;
    new_segment->alloc_size = segment_size;
    new_segment->actual_address = kmalloc_aligned(segment_size, PAGE_SIZE);
    new_segment->flags = WRITEABLE_MEMORY | READABLE_MEMORY;
    new_segment->source = nullptr;
    new_segment->disk_size = 0;
    map_memory_segment(current_process, (uint32_t)new_segment->actual_address,
                       (uint32_t)new_segment->virtual_address, segment_size,
                       user_read_write);
    return req_addr;
  } else {
    struct file *to_map = current_process->open_files;
    while (to_map && to_map->file_descriptor != file_descriptor) {
      to_map = to_map->next;
    }

    if (!to_map) {
      return -1;
    }

    map_memory_segment(current_process, 0, req_addr, len, user_read_write);

    struct file_mapping *new_mapping =
        (struct file_mapping *)kmalloc(sizeof(struct file_mapping));
    new_mapping->next = to_map->mappings;
    new_mapping->prev = nullptr;
    new_mapping->mapping = (void *)req_addr;
    new_mapping->mapping_len = len;
    new_mapping->offset = offset * PAGE_SIZE;
    to_map->mappings = new_mapping;

    return req_addr;
  }
}

uint32_t munmap(uint32_t req_addr, uint32_t len, uint32_t reserved1,
                uint32_t reserved2, uint32_t reserved3, uint32_t reserved4) {
  if (!msync_internal(req_addr, len, 0, true)) {
    struct process *current_process = get_currently_executing_process();
    uint32_t *page_dir = current_process->page_dir;

    int i = 0;
    for (i = 0; i < current_process->num_segments; i++) {
      if ((uint32_t)current_process->segments[i].virtual_address == req_addr) {
        break;
      }
    }

    if (i == current_process->num_segments) {
      return -1;
    }

    unmap_memory_range(page_dir, current_process->segments[i].virtual_address,
                       current_process->segments[i].segment_size);

    kfree(current_process->segments[i].actual_address);

    current_process->num_segments--;
    struct process_memory_segment *new_segments =
        (struct process_memory_segment *)kmalloc(
            current_process->num_segments *
            sizeof(struct process_memory_segment));
    for (int j = 0; j < current_process->num_segments; j++) {
      if (j == i) {
        current_process->segments++;
      }

      new_segments[j] = *current_process->segments;

      current_process->segments++;
    }
    kfree(current_process->segments);
    current_process->segments = new_segments;

    return 0;
  }
}

uint32_t mprotect(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
                  uint32_t reserved4, uint32_t reserved5, uint32_t reserved6) {
  return 0;
}

uint32_t msync(uint32_t req_addr, uint32_t len, uint32_t flags,
               uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
  msync_internal(req_addr, len, flags, false);
}

} // namespace proc
