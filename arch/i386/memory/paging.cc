#include <stddef.h>
#include <stdint.h>

#include "arch/i386/memory/paging.h"
#include "filesystem/fat32.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/process.h"

namespace arch {
namespace memory {

namespace {

using filesystem::file;
using filesystem::file_mapping;
using filesystem::read_fat32;
using filesystem::write_fat32;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::kmalloc_aligned;
using lib::std::krealloc;
using lib::std::memcpy;
using lib::std::memset;
using lib::std::panic;
using lib::std::print_error;
using lib::std::strlen;
using proc::get_currently_executing_process;
using proc::kill_current_process;
using proc::process;

constexpr uint16_t PRESENT = 0x1;
constexpr uint16_t DIRTY = 0x40;
constexpr uint16_t FILE_BACKED = 0x400;

} // namespace

uint32_t *get_page_table_entry(uint32_t *page_dir, void *virtual_addr) {
  uint32_t page_dir_index = (uint32_t)virtual_addr >> 22;
  uint32_t page_table_index = ((uint32_t)virtual_addr >> 12) & 0x3FF;
  uint32_t *page_table = (uint32_t *)(page_dir[page_dir_index] & (~(0xFFF)));

  if (page_table == nullptr) {
    return nullptr;
  }

  return page_table + page_table_index;
}

void map_memory_range(uint32_t *page_directory, uint32_t *page_table,
                      void *physical_address, void *virtual_address,
                      uint32_t size, enum permission page_permissions) {
  uint32_t *page_table_entry =
      page_table + ((uint32_t)virtual_address / PAGE_SIZE);
  map_memory_range_offset(page_directory, page_table_entry, physical_address,
                          virtual_address, size, page_permissions);
}

void unmap_memory_range(uint32_t *page_directory, void *virtual_addr,
                        size_t len) {
  void *current_addr = virtual_addr;
  while (current_addr < virtual_addr + len) {
    *get_page_table_entry(page_directory, current_addr) = 0;
  }
}

void map_memory_range_offset(uint32_t *page_directory,
                             uint32_t *page_table_entry, void *physical_address,
                             void *virtual_address, uint32_t size,
                             enum permission page_permissions) {
  if ((uint32_t)physical_address & 0xFFF || size & 0xFFF) {
    lib::std::panic((char *)"Page mappings must be 4KB aligned!");
  }

  void *current_address = physical_address;
  uint32_t page_index = (uint32_t)virtual_address / PAGE_SIZE;
  uint32_t page_table_offset = 0;
  page_directory[page_index / 1024] =
      (uint32_t)(page_table_entry + page_table_offset) | page_permissions | 0x1;
  while ((uint32_t)current_address < (uint32_t)physical_address + size) {
    // Every 1024 pages we need a new page directory.
    if (!(page_index % 1024)) {
      page_directory[page_index / 1024] =
          (uint32_t)(page_table_entry + page_table_offset) | page_permissions |
          0x1;
    }

    // Add an entry to the page table.
    page_table_entry[page_table_offset] =
        ((uint32_t)current_address & 0xFFFFF000) | page_permissions | 0x1;

    // Go to the next page.
    current_address = (void *)((uint32_t)current_address + PAGE_SIZE);
    page_index++;
    page_table_offset++;
  }
}

void map_memory_segment(struct process *proc, uint32_t physical_address,
                        uint32_t virtual_address, size_t len,
                        enum permission page_permissions, uint16_t flags) {
  uint32_t current_address = virtual_address;
  uint32_t *page_table;
  while (current_address < virtual_address + len) {
    if (proc->page_dir[current_address >> 22]) {
      page_table = (uint32_t *)(proc->page_dir[current_address >> 22] &
                                (~(PAGE_SIZE - 1)));
    } else {
      page_table =
          (uint32_t *)kmalloc_aligned(1024 * sizeof(uint32_t), PAGE_SIZE);
      memset((char *)page_table, 1024 * sizeof(uint32_t), 0);
      proc->num_page_tables++;
      if (!proc->page_tables) {
        proc->page_tables = (uint32_t **)kmalloc(sizeof(uint32_t *));
      } else {
        proc->page_tables = (uint32_t **)krealloc(
            proc->page_tables, proc->num_page_tables * sizeof(uint32_t *));
      }

      proc->page_tables[proc->num_page_tables - 1] = page_table;

      proc->page_dir[current_address >> 22] =
          (uint32_t)page_table | page_permissions | 0x1;
    }

    if (physical_address) {
      page_table[(current_address >> 12) & 0x3FF] =
          physical_address | page_permissions | 0x1 | flags;
    } else {
      page_table[(current_address >> 12) & 0x3FF] = page_permissions | flags;
    }

    current_address += PAGE_SIZE;
    if (physical_address) {
      physical_address += PAGE_SIZE;
    }
  }
}

void *virtual_to_physical(uint32_t *page_dir, void *virtual_addr) {
  uint32_t offset = (uint32_t)virtual_addr & 0xFFF;
  uint32_t *page_table_entry = get_page_table_entry(page_dir, virtual_addr);

  if (!(*page_table_entry & PRESENT)) {
    struct process *current_process = get_currently_executing_process();
    while (current_process->page_dir != page_dir) {
      current_process = current_process->next;
      if (current_process == get_currently_executing_process()) {
        break;
      }
    }

    if (current_process->page_dir != page_dir) {
      return nullptr;
    }

    if (!swap_in_page(current_process,
                      (void *)((uint32_t)virtual_addr & (~(PAGE_SIZE - 1))))) {
      return nullptr;
    }

    page_table_entry = get_page_table_entry(page_dir, virtual_addr);
  }

  char *return_addr = (char *)(*page_table_entry & (~(0xFFF)));

  return (void *)(return_addr + offset);
}

void physical_to_virtual_memcpy(uint32_t *page_dir, char *src, char *dest,
                                size_t size) {
  char *physical_dest = (char *)virtual_to_physical(page_dir, (void *)dest);
  size_t current = 0;
  while (current < size) {
    if (!((uint32_t)(physical_dest) & (PAGE_SIZE - 1))) {
      physical_dest = (char *)virtual_to_physical(page_dir, (void *)dest);
      if (!physical_dest) {
        print_error((char *)"Segmentation Fault");
        kill_current_process();
      }
    }

    *physical_dest = *src;
    physical_dest++;
    src++;
    current++;
    dest++;
  }
}

void virtual_to_physical_memcpy(uint32_t *page_dir, char *src, char *dest,
                                size_t size) {
  char *physical_src = (char *)virtual_to_physical(page_dir, (void *)src);
  size_t current = 0;
  while (current < size) {
    if (!((uint32_t)(physical_src) & (PAGE_SIZE - 1))) {
      physical_src = (char *)virtual_to_physical(page_dir, (void *)src);
      if (!physical_src) {
        print_error((char *)"Segmentation Fault");
        kill_current_process();
      }
    }

    *dest = *physical_src;
    physical_src++;
    src++;
    current++;
    dest++;
  }
}

void virtual_to_virtual_memcpy(uint32_t *src_page_dir, uint32_t *dest_page_dir,
                               char *src, char *dest, size_t size) {
  char *physical_src = (char *)virtual_to_physical(src_page_dir, (void *)src);
  char *physical_dest =
      (char *)virtual_to_physical(dest_page_dir, (void *)dest);
  size_t current = 0;
  while (current < size) {
    if (!((uint32_t)(physical_src) & (PAGE_SIZE - 1))) {
      physical_src = (char *)virtual_to_physical(src_page_dir, (void *)src);
      if (!physical_src) {
        print_error((char *)"Segmentation Fault");
        kill_current_process();
      }
    }
    if (!((uint32_t)(physical_dest) & (PAGE_SIZE - 1))) {
      physical_dest = (char *)virtual_to_physical(dest_page_dir, (void *)dest);
      if (!physical_dest) {
        print_error((char *)"Segmentation Fault");
        kill_current_process();
      }
    }
    *physical_dest = *physical_src;
    physical_src++;
    src++;
    physical_dest++;
    dest++;
    current++;
  }
}

char *make_virtual_string_copy(uint32_t *page_dir, char *virtual_string) {
  uint32_t len = 0;
  while (1) {
    char *next_char_pointer =
        (char *)virtual_to_physical(page_dir, virtual_string + len);
    if (!next_char_pointer) {
      return nullptr;
    } else if (!*next_char_pointer) {
      break;
    }
    len++;
  }
  len++;
  char *ret = (char *)kmalloc(len);
  virtual_to_physical_memcpy(page_dir, virtual_string, ret, len);
  return ret;
}

char swap_in_page(struct process *proc, void *virtual_addr) {
  struct file_mapping *current_mapping = proc->mappings;

  while (current_mapping &&
         !(current_mapping->mapping <= virtual_addr &&
           current_mapping->mapping + current_mapping->mapping_len >
               virtual_addr)) {
    current_mapping = current_mapping->next;
  }

  if (!current_mapping) {
    return 0; // Uh oh, either segfaul or panic
  }

  struct file *current_file = current_mapping->file;

  void *actual_addr = kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);

  map_memory_segment(proc, (uint32_t)actual_addr, (uint32_t)virtual_addr,
                     PAGE_SIZE, user_read_write, FILE_BACKED);

  uint32_t offset = (uint32_t)virtual_addr - (uint32_t)current_mapping->mapping;
  size_t read_len = current_mapping->mapping_len - offset < PAGE_SIZE
                        ? current_mapping->mapping_len - offset
                        : PAGE_SIZE;
  offset += current_mapping->offset;
  uint32_t read_size =
      read_fat32(current_file->inode, offset, (uint8_t *)actual_addr, read_len);
  if (!read_size) {
    return 0;
  } else if (read_size < PAGE_SIZE) {
    memset((char *)actual_addr + read_size, PAGE_SIZE - read_size, 0);
  }

  return 1;
}

void flush_pages(uint32_t *page_dir, struct file_mapping *mapping,
                 void *start_addr, void *stop_addr) {
  struct file *backing_file = mapping->file;
  void *current_addr = mapping->mapping;
  if (start_addr) {
    current_addr = start_addr;
  }
  while (current_addr < mapping->mapping + mapping->mapping_len &&
         (!stop_addr || current_addr < stop_addr)) {
    uint32_t *page_table_entry = get_page_table_entry(page_dir, current_addr);
    if (!(*page_table_entry & PRESENT) || !(*page_table_entry & FILE_BACKED)) {
      current_addr += PAGE_SIZE;
      continue;
    }

    void *actual_addr = virtual_to_physical(page_dir, current_addr);

    if (!mapping->is_private && *page_table_entry & DIRTY) {
      // Flush dirty pages to disk
      uint32_t offset = (uint32_t)current_addr - (uint32_t)mapping->mapping;
      size_t write_len = mapping->mapping_len - offset < PAGE_SIZE
                             ? mapping->mapping_len - offset
                             : PAGE_SIZE;
      offset += mapping->offset;
      write_fat32(backing_file->path, offset, (uint8_t *)actual_addr,
                  write_len);
      *page_table_entry ^= DIRTY;
    }

    // Free the memory allocation
    kfree(actual_addr);

    *page_table_entry ^= PRESENT;

    current_addr += PAGE_SIZE;
  }
}

void copy_mapping(struct process *src_proc, struct process *dest_proc,
                  struct file_mapping *mapping) {
  void *current_addr = mapping->mapping;
  while (current_addr < mapping->mapping + mapping->mapping_len) {
    uint32_t *page_table_entry =
        get_page_table_entry(src_proc->page_dir, current_addr);

    if (*page_table_entry & PRESENT && *page_table_entry & FILE_BACKED) {
      void *actual_addr = kmalloc_aligned(PAGE_SIZE, PAGE_SIZE);
      map_memory_segment(dest_proc, (uint32_t)actual_addr,
                         (uint32_t)current_addr, PAGE_SIZE, user_read_write,
                         FILE_BACKED);
      memcpy((char *)virtual_to_physical(src_proc->page_dir, current_addr),
             (char *)actual_addr, PAGE_SIZE);
    }

    current_addr += PAGE_SIZE;
  }
}

} // namespace memory
} // namespace arch
