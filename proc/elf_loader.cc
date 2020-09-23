#include <stddef.h>
#include <stdint.h>

#include "filesystem/fat32.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/elf_loader.h"
#include "proc/process.h"

namespace proc {

namespace {

using filesystem::directory_entry;
using filesystem::read_fat32;
using filesystem::stat_fat32;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::krealloc;
using lib::std::make_string_copy;
using lib::std::memcpy;
using proc::process_memory_segment;

struct __attribute__((packed)) elf_header {
  unsigned char ident[16];
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint32_t entry;
  uint32_t segments_offset;
  uint32_t sections_offset;
  uint32_t flags;
  uint16_t header_size;
  uint16_t segment_entry_size;
  uint16_t num_segments;
  uint16_t section_entry_size;
  uint16_t num_sections;
  uint16_t string_table_index;
};

struct __attribute__((packed)) segment_header {
  uint32_t type;
  uint32_t offset;
  uint32_t virtual_address;
  uint32_t physical_address; // Ignored
  uint32_t disk_size;
  uint32_t memory_size;
  uint32_t flags;
  uint32_t alignment;
};

constexpr uint16_t MACHINE_TYPE_X86 = 3;

constexpr uint32_t LOADABLE_SEGMENT = 1;
constexpr uint32_t INTERPRETER_SEGMENT = 3;
constexpr uint32_t TLS_SEGMENT = 7;

constexpr uint32_t EXEC = 2;
constexpr uint32_t DYN = 3;

constexpr uint32_t DEFAULT_PROGRAM_VIRTUAL_OFFSET = 0x8048000;

struct segment_list {
  struct process_memory_segment *segments;
  uint32_t num_segments;
  char *linker_path;
};

uint8_t *load_elf_from_disk(char *path) {
  struct directory_entry file_info = stat_fat32(path);

  if (!file_info.name) {
    return nullptr;
  } else if (!file_info.size || file_info.is_directory) {
    kfree(file_info.name);
    return nullptr;
  }

  kfree(file_info.name);

  // Load executable into memory
  uint8_t *file_buf = (uint8_t *)kmalloc(file_info.size);
  if (!read_fat32(path, file_buf, file_info.size)) {
    kfree(file_buf);
    return nullptr;
  }

  // Check the header to make sure this is actually an ELF
  struct elf_header *header = (struct elf_header *)file_buf;
  if (header->ident[0] != 0x7F || header->ident[1] != 'E' ||
      header->ident[2] != 'L' || header->ident[3] != 'F' ||
      header->machine != MACHINE_TYPE_X86) {
    kfree(file_buf);
    return nullptr;
  }

  return file_buf;
}

struct segment_list create_process_segments(uint8_t *file_buf,
                                            uint32_t dyn_virtual_offset) {
  struct segment_list ret;

  ret.linker_path = nullptr;

  struct elf_header *header = (struct elf_header *)file_buf;

  // Find loadable segments and fill out the Process Memory Segment table
  // appropriately
  uint32_t num_segments = header->num_segments;
  struct segment_header *segment_table =
      (struct segment_header *)(file_buf + header->segments_offset);
  ret.num_segments = 0;
  for (int i = 0; i < num_segments; i++) {
    if (segment_table[i].type == LOADABLE_SEGMENT) {
      ret.num_segments++;
    }
  }

  ret.segments = (struct process_memory_segment *)kmalloc(
      ret.num_segments * sizeof(struct process_memory_segment));
  char *linker_path = nullptr;
  int process_segment_index = 0;
  for (int i = 0; i < num_segments; i++) {
    if (segment_table[i].type == LOADABLE_SEGMENT) {
      ret.segments[process_segment_index].virtual_address =
          (void *)(segment_table[i].virtual_address);
      if (header->type == DYN) {
        ret.segments[process_segment_index].virtual_address +=
            dyn_virtual_offset;
      }
      ret.segments[process_segment_index].segment_size =
          segment_table[i].memory_size;
      ret.segments[process_segment_index].flags = segment_table[i].flags;
      if (segment_table[i].disk_size) {
        ret.segments[process_segment_index].source =
            file_buf + segment_table[i].offset;
        ret.segments[process_segment_index].disk_size =
            segment_table[i].disk_size;
      }

      process_segment_index++;
    } else if (segment_table[i].type == INTERPRETER_SEGMENT) {
      ret.linker_path =
          make_string_copy((char *)file_buf + segment_table[i].offset);
    }
  }

  return ret;
}

} // namespace

char load_elf(char *path, int argc, char **argv, char *working_dir) {
  uint8_t *file_buf = load_elf_from_disk(path);
  if (!file_buf) {
    return 0;
  }

  struct segment_list segments =
      create_process_segments(file_buf, DEFAULT_PROGRAM_VIRTUAL_OFFSET);

  char need_path_cleanup = 0;
  if (segments.linker_path) {
    need_path_cleanup = 1;
    kfree(segments.segments);
    kfree(file_buf);

    path = segments.linker_path;

    // This process is dynamically linked. Load the linker into memory as well.
    file_buf = load_elf_from_disk(segments.linker_path);
    if (!file_buf) {
      return 0;
    }

    segments =
        create_process_segments(file_buf, DEFAULT_PROGRAM_VIRTUAL_OFFSET);

    argc++;
    char **old_argv = argv;
    argv = (char **)kmalloc(argc * sizeof(char **));
    argv[0] = make_string_copy(path);
    for (int i = 1; i < argc; i++) {
      argv[i] = old_argv[i - 1];
    }
    kfree(old_argv);
  }

  struct elf_header *header = (struct elf_header *)file_buf;
  void *entry = (void *)header->entry;
  if (header->type == DYN) {
    entry += DEFAULT_PROGRAM_VIRTUAL_OFFSET;
  }

  // Spawn the process
  char ret =
      spawn_new_process(path, argc, argv, segments.segments,
                        segments.num_segments, (void (*)())entry, working_dir);

  kfree(segments.segments);
  kfree(file_buf);

  if (need_path_cleanup) {
    kfree(path);
  }

  return ret;
}

} // namespace proc
