#include <stddef.h>
#include <stdint.h>

#include "drivers/i386/pata.h"
#include "filesystem/fat32.h"
#include "filesystem/mbr.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"

namespace filesystem {

namespace {

using drivers::ide_device;
using drivers::read_sectors;
using lib::std::find_first;
using lib::std::kfree;
using lib::std::kmalloc;
using lib::std::krealloc;
using lib::std::make_string_copy;
using lib::std::memcpy;
using lib::std::memset;
using lib::std::panic;
using lib::std::strcat;
using lib::std::streq;
using lib::std::strlen;
using lib::std::substring;
using lib::std::trim;

struct __attribute__((packed)) directory_table_entry {
  char filename[11];
  uint8_t attributes;
  uint8_t reserved;             // We abuse this to store long filenames
  uint8_t creation_time_tenths; // Ignore this
  uint16_t creation_time;
  uint16_t creation_data;
  uint16_t last_accessed_date;
  uint16_t cluster_num_high;
  uint16_t last_modified_time;
  uint16_t last_modified_date;
  uint16_t cluster_num_low;
  uint32_t file_size;
};

struct __attribute__((packed)) long_filename_entry {
  uint8_t sequence_num;
  uint16_t chars1[5];
  uint8_t attribute;
  uint8_t type; // This doesn't seem to ever get used
  uint8_t checksum;
  uint16_t chars2[6];
  uint16_t reserved;
  uint16_t chars3[2];
};

constexpr char DELETED_DIR_ENTRY = 0xE5;

constexpr uint8_t LONG_FILE_NAME = 0x0F;
constexpr uint8_t READ_ONLY_FLAG = 0x01;
constexpr uint8_t DIRECTORY_FLAG = 0x10;

constexpr uint32_t END_OF_FILE_CLUSTER = 0x0FFFFFF8;

struct __attribute__((packed)) extended_boot_record {
  char start_code[3]; // Machine code, not data
  char oem_ident[8];
  uint16_t sector_size; // In bytes
  uint8_t cluster_size; // Measured in sectors
  uint16_t fat_pointer; // In sectors
  uint8_t num_fat;
  uint16_t num_dir_entries;
  uint16_t small_sector_count; // Probably safe to ignore for modern hardware
  uint8_t type;
  uint16_t reserved; // 16bit sectors per fat is ignored for FAT32
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t num_hidden_sectors;
  uint32_t large_sector_count;
  uint32_t sectors_per_fat;
  uint16_t flags;
  uint8_t version_minor;
  uint8_t version_major;
  uint32_t root_dir_pointer;           // In clusters
  uint16_t fsinfo_pointer;             // In sectors
  uint16_t backup_boot_sector_pointer; // In sectors
  char reserved2[12];
  uint8_t drive_num;
  uint8_t reserved3;
  uint8_t signature; // Must be 0x28 or 0x29
  uint32_t volume_serial;
  char volume_label[11];
  char system_ident_string[8];
  char boot_code[420]; // More machine code
  uint16_t bootable_partition_signature;
};

const struct ide_device *device;
struct extended_boot_record boot_record;
uint32_t filesystem_size;
uint32_t start_sector; // LBA
uint32_t sector_size;  // In bytes
uint32_t fat_start;    // LBA
uint32_t fat_size;     // In bytes
uint32_t fat_sectors;  // Size in sectors
uint32_t num_fats;
uint32_t *file_allocation_table;
uint32_t cluster_start;    // LBA
uint32_t cluster_size;     // In bytes
uint32_t cluster_sectors;  // Size in sectors
uint32_t root_dir_cluster; // In clusters

uint32_t read_clusters(uint32_t cluster, uint8_t *buf, size_t len) {
  uint8_t *tmp_buf = (uint8_t *)kmalloc(cluster_size);
  size_t bytes_read = 0;

  while (bytes_read < len) {
    read_sectors(*device, tmp_buf, cluster_sectors,
                 cluster_start + (cluster - 2) * cluster_sectors);
    bytes_read += cluster_size;

    if (bytes_read > len) {
      bytes_read -= cluster_size;
      size_t remaining_bytes = len - bytes_read;
      memcpy((char *)tmp_buf, (char *)buf, remaining_bytes);
      bytes_read += remaining_bytes;
      break;
    } else {
      memcpy((char *)tmp_buf, (char *)buf, cluster_size);
      buf += cluster_size;
    }

    if ((file_allocation_table[cluster] & 0x0FFFFFFF) >=
        (uint32_t)END_OF_FILE_CLUSTER) {
      break;
    } else {
      cluster = file_allocation_table[cluster] & 0x0FFFFFFF;
    }
  }

  kfree(tmp_buf);

  return bytes_read;
}

void write_clusters(uint32_t cluster, uint8_t *buf, size_t len) {
  uint32_t index = 0;
  uint8_t *temp_buf = (uint8_t *)kmalloc(cluster_size);
  do {
    uint32_t copy_size =
        (len - index) < cluster_size ? len - index : cluster_size;
    memcpy((char *)buf + index, (char *)temp_buf, copy_size);
    if (copy_size < cluster_size) {
      memset((char *)temp_buf + copy_size, cluster_size - copy_size, 0);
    }
    write_sectors(*device, temp_buf, cluster_sectors,
                  cluster_start + (cluster - 2) * cluster_sectors);
    cluster = file_allocation_table[cluster] & 0x0FFFFFFF;
    index += cluster_size;
  } while (index < len && cluster < END_OF_FILE_CLUSTER);
  kfree(temp_buf);
}

void parse_eight_three_filename(const char *filename, char *parsed_filename) {
  int i = 0;
  char is_dot_file = 0;
  if (filename[0] == '.' && filename[1] == 0) {
    i = 0;
    is_dot_file = 1;
  } else if (filename[0] == '.' && filename[1] == '.' && filename[2] == 0) {
    i = 1;
    is_dot_file = 1;
  }

  if (is_dot_file) {
    int j;
    for (j = 0; j <= i; j++) {
      parsed_filename[j] = '.';
    }
    for (j; j < 11; j++) {
      parsed_filename[j] = ' ';
    }
    return;
  }

  for (i = 0; i < 8; i++) {
    if (!filename[i] || filename[i] == '.') {
      break;
    }
    parsed_filename[i] = filename[i];
  }
  for (i; i < 8; i++) {
    parsed_filename[i] = ' ';
  }

  uint32_t filename_len = strlen(filename);
  if (filename_len >= 4 && filename[filename_len - 4] == '.') {
    parsed_filename[8] = filename[filename_len - 3];
    parsed_filename[9] = filename[filename_len - 2];
    parsed_filename[10] = filename[filename_len - 1];
  } else {
    parsed_filename[8] = ' ';
    parsed_filename[9] = ' ';
    parsed_filename[10] = ' ';
  }

  lib::std::to_upper(parsed_filename);
}

char *unpack_eight_three_filename(const char *eight_three_filename) {
  char *name = substring(eight_three_filename, 0, 8);
  char *ext = substring(eight_three_filename, 8, 11);
  char *tmp = trim(name);

  kfree(name);

  if (ext[2] != ' ') {
    char *tmp2;
    tmp2 = strcat(tmp, ".");
    kfree(tmp);
    tmp = strcat(tmp2, ext);
    kfree(tmp2);
  }

  kfree(ext);

  return tmp;
}

uint32_t convert_cluster(struct directory_table_entry &entry) {
  uint32_t ret = (uint32_t)entry.cluster_num_low;
  ret |= ((uint32_t)entry.cluster_num_high << 16);
  return ret;
}

struct lfn_part {
  char text[14];
  uint8_t index;
};

uint32_t parse_long_filename(char *buf, char **filename) {
  struct long_filename_entry *lfn_entries = (struct long_filename_entry *)buf;
  size_t parts_size = 1;
  size_t parts_len = 0;
  struct lfn_part *parts =
      (struct lfn_part *)kmalloc(parts_size * sizeof(struct lfn_part));
  int i = 0;
  while (lfn_entries[i].attribute == LONG_FILE_NAME) {
    if (lfn_entries[i].sequence_num == DELETED_DIR_ENTRY) {
      i++;
      continue;
    }

    parts_len++;
    if (parts_len > parts_size) {
      parts_size *= 2;
      parts = (struct lfn_part *)krealloc(parts,
                                          parts_size * sizeof(struct lfn_part));
    }

    // This is arcane...
    for (int j = 0; j < 5; j++) {
      parts[parts_len - 1].text[j] = lfn_entries[i].chars1[j];
    }
    for (int j = 0; j < 6; j++) {
      parts[parts_len - 1].text[j + 5] = lfn_entries[i].chars2[j];
    }
    for (int j = 0; j < 2; j++) {
      parts[parts_len - 1].text[j + 11] = lfn_entries[i].chars3[j];
    }
    parts[parts_len - 1].text[13] = 0;

    parts[parts_len - 1].index = lfn_entries[i].sequence_num & 0x1F;

    i++;
  }

  char *partial_filename = nullptr;
  for (int j = parts_len - 1; j >= 0; j--) {
    if (!partial_filename) {
      partial_filename = make_string_copy(parts[j].text);
    } else {
      char *tmp = partial_filename;
      partial_filename = strcat(partial_filename, parts[j].text);
      kfree(tmp);
    }
  }
  *filename = partial_filename;

  kfree(parts);

  return i; // We want to return i rather than parts_len here in case there was
            // a deleted entry
}

struct directory_table_entry find_directory_entry(
    char *path, uint32_t current_cluster = root_dir_cluster,
    char return_long_filenames = true, uint32_t *dir_table_cluster = nullptr,
    uint8_t **dir_table_buf = nullptr, size_t *dir_table_buf_size = nullptr) {
  struct directory_table_entry ret;
  ret.filename[0] = 0;
  ret.reserved = 0;
  if (!path || !path[0] || path[0] != '/') {
    return ret;
  }

  path++;
  int next_dir = find_first(path, '/', 255);
  char *current_name;
  if (next_dir < 0) {
    current_name = path;
  } else {
    current_name = substring(path, 0, next_dir);
  }

  size_t table_size = cluster_size;
  struct directory_table_entry *dir_table =
      (struct directory_table_entry *)kmalloc(table_size);
  read_clusters(current_cluster, (uint8_t *)dir_table, cluster_size);
  current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  while ((current_cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER) {
    table_size += cluster_size;
    dir_table = (struct directory_table_entry *)krealloc(dir_table, table_size);
    read_clusters(current_cluster,
                  (uint8_t *)dir_table + table_size - cluster_size,
                  cluster_size);
    current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  }

  char eight_three_filename[12] = {0};
  parse_eight_three_filename(current_name, eight_three_filename);
  char *long_filename = nullptr;

  int i = 0;
  for (i = 0; i < table_size / sizeof(struct directory_table_entry); i++) {
    if (!dir_table[i].filename[0]) {
      kfree(dir_table);
      if (next_dir >= 0) {
        kfree(current_name);
      }
      return ret; // File not found
    } else if (streq(dir_table[i].filename, eight_three_filename, 11)) {
      ret = dir_table[i];
      ret.reserved = 0;
      break;
    } else if (eight_three_filename[0] != 'U' &&
               dir_table[i].attributes == LONG_FILE_NAME) {
      uint32_t num_entries =
          parse_long_filename((char *)&dir_table[i], &long_filename);
      if (streq(long_filename, current_name, 13 * num_entries)) {
        if (return_long_filenames) {
          ret = dir_table[i + num_entries];
          ret.reserved = 1; // TODO: this is hacky, find a proper solution
          *(char **)&ret.filename = long_filename;
        } else {
          kfree(long_filename);
        }
        break;
      }
      kfree(long_filename);
      long_filename = nullptr;
      i += num_entries;
    }
  }

  if (next_dir < 0) {
    if (!dir_table_buf) {
      kfree(dir_table);
    } else {
      *dir_table_buf_size = table_size;
      *dir_table_buf = (uint8_t *)dir_table;
      *dir_table_cluster = current_cluster;
    }
    return ret;
  } else {
    kfree(dir_table);
    if (long_filename) {
      kfree(long_filename);
    }
    if (!(ret.attributes & DIRECTORY_FLAG)) {
      ret.filename[0] = 0;
      return ret;
    } else {
      kfree(current_name);
      uint32_t next_cluster = convert_cluster(ret);
      return find_directory_entry(path + next_dir, next_cluster);
    }
  }
}

constexpr uint32_t INVALID_CLUSTER = 0xFFFFFFFF;
uint32_t find_cluster(char *path, uint32_t current_cluster = root_dir_cluster) {
  if (streq(path, "/", 2)) {
    return root_dir_cluster;
  }

  struct directory_table_entry entry =
      find_directory_entry(path, current_cluster);
  if (!entry.filename[0]) {
    return INVALID_CLUSTER;
  } else {
    return convert_cluster(entry);
  }
}

void flush_fat(void) {
  uint32_t fat_lba = fat_start;
  // There are generally multiple (usually 2) file allocation tables.
  // They are duplicates of each, presumably used to detect corruption.
  for (int i = 0; i < num_fats; i++) {
    write_sectors(*device, (uint8_t *)file_allocation_table, fat_sectors,
                  fat_lba);
    fat_lba += fat_sectors;
  }
}

uint32_t alloc_cluster(size_t len, uint32_t search_start = 2) {
  for (int i = search_start; i < fat_size / sizeof(uint32_t); i++) {
    if (!file_allocation_table[i]) {
      if (len <= cluster_size) {
        file_allocation_table[i] = END_OF_FILE_CLUSTER;
      } else {
        uint32_t next_cluster =
            alloc_cluster(len - cluster_size, search_start + 1);
        if (next_cluster == INVALID_CLUSTER) {
          return INVALID_CLUSTER;
        }
        file_allocation_table[i] = next_cluster;
      }
      return i;
    }
  }
  return INVALID_CLUSTER;
}

void dealloc_clusters(uint32_t cluster) {
  do {
    uint32_t next_cluster = file_allocation_table[cluster];
    file_allocation_table[cluster] = 0;
    cluster = next_cluster;
  } while ((cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER);
  flush_fat();
}

void dealloc_clusters(uint32_t cluster, int num) {
  for (int i = 0; i < num; i++) {
    if ((cluster & 0x0FFFFFFF) >= END_OF_FILE_CLUSTER) {
      break;
    }
    uint32_t next_cluster = file_allocation_table[cluster];
    file_allocation_table[cluster] = 0;
    cluster = next_cluster;
  }
  flush_fat();
}

int split_path(char *path) {
  int len = strlen(path);

  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') {
      return i;
    }
  }

  return -1;
}

int create_long_filename_entries(char *filename, uint8_t checksum,
                                 struct long_filename_entry **output_entries) {
  int len = strlen(filename);
  int num_entries = (len + 1) / 13;
  if (13 * (len + 1) != num_entries) { // We wanna round up here
    num_entries++;
  }
  struct long_filename_entry *entries = (struct long_filename_entry *)kmalloc(
      num_entries * sizeof(struct long_filename_entry));
  *output_entries = entries;

  int filename_index = 0;
  for (int i = num_entries - 1; i >= 0; i--) {
    entries[i].sequence_num = num_entries - i;
    if (i == 0) {
      entries[i].sequence_num |= 0x40;
    }
    entries[i].attribute = LONG_FILE_NAME;
    entries[i].type = 0;
    entries[i].checksum = checksum;
    entries[i].reserved = 0;

    for (int j = 0; j < 5; j++) {
      if (filename_index == len) {
        entries[i].chars1[j] = 0;
        return num_entries;
      }
      entries[i].chars1[j] = filename[filename_index];
      filename_index++;
    }
    for (int j = 0; j < 6; j++) {
      if (filename_index == len) {
        entries[i].chars2[j] = 0;
        return num_entries;
      }
      entries[i].chars2[j] = filename[filename_index];
      filename_index++;
    }
    for (int j = 0; j < 2; j++) {
      if (filename_index == len) {
        entries[i].chars3[j] = 0;
        return num_entries;
      }
      entries[i].chars3[j] = filename[filename_index];
      filename_index++;
    }
  }
  return num_entries;
}

uint8_t compute_checksum(char *eight_three_filename) {
  uint8_t ret = 0;

  for (int i = 0; i < 11; i++) {
    ret = ((ret & 1) << 7) + (ret >> 1) + eight_three_filename[i];
  }
  return ret;
}

char add_directory_entry(char *dir_path, char *name, uint32_t attributes,
                         size_t len, uint32_t cluster) {
  uint32_t dir_cluster = find_cluster(dir_path);

  if (dir_cluster == INVALID_CLUSTER) {
    return 0;
  }

  char eight_three_filename[12] = {0};
  parse_eight_three_filename(name, eight_three_filename);
  uint8_t checksum = compute_checksum(eight_three_filename);
  struct long_filename_entry *long_filename_entries = nullptr;
  int num_long_filename_entries =
      create_long_filename_entries(name, checksum, &long_filename_entries);
  int num_dir_entries = 1 + num_long_filename_entries;

  size_t table_size = cluster_size;
  struct directory_table_entry *dir_table =
      (struct directory_table_entry *)kmalloc(table_size);
  uint32_t current_cluster = dir_cluster;
  read_clusters(current_cluster, (uint8_t *)dir_table, cluster_size);
  uint32_t last_cluster = current_cluster;
  current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  while ((current_cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER) {
    table_size += cluster_size;
    dir_table = (struct directory_table_entry *)krealloc(dir_table, table_size);
    read_clusters(current_cluster,
                  (uint8_t *)dir_table + table_size - cluster_size,
                  cluster_size);
    last_cluster = current_cluster;
    current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  }

  int i = 0;
  int contiguous_free_entries = 0;

  for (i = 0; i < table_size / sizeof(struct directory_table_entry); i++) {
    if (dir_table[i].filename[0] == DELETED_DIR_ENTRY) {
      contiguous_free_entries++;
    } else if (!dir_table[i].filename[0]) {
      i += num_dir_entries - contiguous_free_entries - 1;
      contiguous_free_entries = num_dir_entries;
    } else {
      contiguous_free_entries = 0;
    }

    if (contiguous_free_entries == num_dir_entries) {
      break;
    }
  }

  if (i == table_size / sizeof(struct directory_table_entry)) {
    size_t old_table_size = table_size;
    table_size +=
        contiguous_free_entries * sizeof(struct directory_table_entry);
    dir_table = (struct directory_table_entry *)krealloc(dir_table, table_size);
    uint32_t new_cluster = alloc_cluster(table_size - old_table_size);
    file_allocation_table[last_cluster] = new_cluster;
    flush_fat();
  }

  memcpy((char *)long_filename_entries,
         (char *)(dir_table + i - num_long_filename_entries),
         num_long_filename_entries * sizeof(struct long_filename_entry));
  memcpy(eight_three_filename, dir_table[i].filename,
         sizeof(dir_table[i].filename));
  dir_table[i].attributes = attributes;
  dir_table[i].file_size = len;
  dir_table[i].cluster_num_low = cluster & 0xFFFF;
  dir_table[i].cluster_num_high = cluster >> 16;
  write_clusters(dir_cluster, (uint8_t *)dir_table, table_size);
  kfree(long_filename_entries);
  kfree(dir_table);
  return 1;
}

} // namespace

void init_fat32(struct ide_device &_device, struct partition &partition) {
  device = &_device;
  start_sector = partition.starting_lba;
  read_sectors(*device, (uint8_t *)&boot_record, 1, start_sector);

  filesystem_size = boot_record.large_sector_count;
  sector_size = boot_record.sector_size;

  fat_start = start_sector + boot_record.fat_pointer;
  num_fats = boot_record.num_fat;
  fat_sectors = boot_record.sectors_per_fat;
  fat_size = fat_sectors * sector_size;
  file_allocation_table = (uint32_t *)kmalloc(fat_size);
  read_sectors(*device, (uint8_t *)file_allocation_table, fat_sectors,
               fat_start);

  cluster_sectors = boot_record.cluster_size;
  cluster_size = cluster_sectors * sector_size;
  cluster_start = fat_start + fat_sectors * num_fats;
  root_dir_cluster = boot_record.root_dir_pointer;
}

char read_fat32(char *path, uint8_t *buf, size_t len) {
  uint32_t cluster = find_cluster(path, root_dir_cluster);
  if (cluster != INVALID_CLUSTER) {
    read_clusters(cluster, buf, len);
    return 1;
  } else {
    return 0;
  }
}

uint32_t read_fat32(uint32_t inode, uint32_t offset, uint8_t *buf, size_t len) {
  uint32_t cluster = inode;
  int index;

  for (index = 0; index + cluster_size <= offset &&
                  (cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER;
       index += cluster_size) {
    cluster = file_allocation_table[cluster] & 0x0FFFFFFF;
  }

  if ((cluster & 0x0FFFFFFF) >= END_OF_FILE_CLUSTER) {
    return 0;
  }

  uint8_t *temp_buf = (uint8_t *)kmalloc(cluster_size);
  size_t read_len = len < cluster_size ? len : cluster_size;
  uint32_t temp_len = read_clusters(cluster, temp_buf, read_len);
  for (int i = offset - index; i < temp_len; i++) {
    *buf = temp_buf[i];
    buf++;
  }
  kfree(temp_buf);
  cluster = file_allocation_table[cluster] & 0x0FFFFFFF;

  if (cluster >= END_OF_FILE_CLUSTER) {
    return temp_len - (offset - index);
  }
  if (len > temp_len - (offset - index)) {
    len -= temp_len - (offset - index);

    return (temp_len - (offset - index)) + read_clusters(cluster, buf, len);
  } else {
    return len;
  }
}

struct directory_entry stat_fat32(char *path) {
  struct directory_entry ret;

  if (path && strlen(path) == 1 && path[0] == '/') {
    ret.name = make_string_copy("/");
    ret.read_only = 0;
    ret.is_directory = 1;
    ret.size = cluster_size;
    ret.inode = root_dir_cluster;
    return ret;
  }

  struct directory_table_entry entry = find_directory_entry(path);

  if (entry.filename[0] || entry.reserved) {
    if (!entry.reserved) {
      ret.name = unpack_eight_three_filename(entry.filename);
    } else {
      ret.name = *(char **)&entry.filename;
    }
    ret.read_only = !!(entry.attributes & READ_ONLY_FLAG);
    ret.is_directory = !!(entry.attributes & DIRECTORY_FLAG);
    ret.size = entry.file_size;
    ret.inode = convert_cluster(entry);
  } else {
    ret.name = nullptr;
    ret.size = 0;
  }
  return ret;
}

struct directory ls_fat32(char *path) {
  uint32_t current_cluster;
  current_cluster = find_cluster(path, root_dir_cluster);
  struct directory ret;

  if (current_cluster == INVALID_CLUSTER) {
    ret.num_children = 0;
    ret.children = nullptr;
    return ret;
  }

  size_t table_size = cluster_size;
  struct directory_table_entry *dir_table =
      (struct directory_table_entry *)kmalloc(table_size);
  read_clusters(current_cluster, (uint8_t *)dir_table, cluster_size);
  current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  while ((current_cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER) {
    table_size += cluster_size;
    dir_table = (struct directory_table_entry *)krealloc(dir_table, table_size);
    read_clusters(current_cluster,
                  (uint8_t *)dir_table + table_size - cluster_size,
                  cluster_size);
    current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  }

  uint32_t num_children = 0;
  for (int i = 0; i < table_size / sizeof(struct directory_table_entry); i++) {
    if (!dir_table[i].filename[0]) {
      break;
    }
    if (dir_table[i].attributes == LONG_FILE_NAME ||
        dir_table[i].filename[0] == DELETED_DIR_ENTRY) {
      continue;
    }
    num_children++;
  }

  ret.num_children = num_children;
  ret.children = (struct directory_entry *)kmalloc(
      num_children * sizeof(struct directory_entry));
  int child_index = 0;
  for (int i = 0; i < table_size / sizeof(struct directory_table_entry); i++) {
    if (!dir_table[i].filename[0]) {
      break;
    }
    if (dir_table[i].filename[0] == DELETED_DIR_ENTRY) {
      continue;
    } else if (dir_table[i].attributes == LONG_FILE_NAME) {
      i += parse_long_filename((char *)&dir_table[i],
                               &ret.children[child_index].name);
      child_index++;
    } else {
      ret.children[child_index].name =
          unpack_eight_three_filename(dir_table[i].filename);
      ret.children[child_index].read_only =
          !!(dir_table[i].attributes & READ_ONLY_FLAG);
      ret.children[child_index].size = dir_table[i].file_size;
      child_index++;
    }
  }

  return ret;
}

uint32_t write_new_fat32(char *path, uint8_t attributes, uint8_t *buf,
                         size_t len) {
  del_fat32(path);
  uint32_t cluster = alloc_cluster(len);
  if (cluster == INVALID_CLUSTER) {
    return 0;
  } else {
    flush_fat();
  }

  int path_split = split_path(path);
  char *dir_path;
  char *name;
  if (path_split > 0) {
    dir_path = substring(path, 0, path_split);
    name = substring(path, path_split + 1, strlen(path));
  } else {
    dir_path = make_string_copy("/");
    name = substring(path, 1, strlen(path));
  }

  if (add_directory_entry(dir_path, name, attributes, len, cluster)) {
    write_clusters(cluster, buf, len);
    kfree(dir_path);
    kfree(name);
    return cluster;
  } else {
    dealloc_clusters(cluster);
    kfree(dir_path);
    kfree(name);
    return 0;
  }
}

char update_file_length(char *path, size_t new_len) {
  uint8_t *buf = nullptr;
  size_t buf_size;
  uint32_t cluster = INVALID_CLUSTER;
  struct directory_table_entry entry = find_directory_entry(
      path, root_dir_cluster, false, &cluster, &buf, &buf_size);

  if (!buf) {
    return 0;
  }

  struct directory_table_entry *table = (struct directory_table_entry *)buf;
  int i = 0;
  while (table[i].filename[0]) {
    if (streq(entry.filename, table[i].filename, 11)) {
      table[i].file_size = new_len;
      write_clusters(cluster, buf, buf_size);
      kfree(buf);
      return 1;
    }

    i++;
  }

  return 0;
}

uint32_t write_fat32(char *path, uint32_t offset, uint8_t *buf, size_t len) {
  uint32_t cluster = find_cluster(path, root_dir_cluster);
  uint32_t last_cluster = cluster;

  if ((cluster & 0x0FFFFFFF) >= END_OF_FILE_CLUSTER) {
    return 0;
  }

  uint64_t index = 0;
  for (index; index + cluster_size <= offset &&
              (cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER;
       index += cluster_size) {
    last_cluster = cluster;
    cluster = file_allocation_table[cluster] & 0x0FFFFFFF;
  }

  for (index; index < offset + len; index += cluster_size) {
    if ((cluster & 0x0FFFFFFF) >= END_OF_FILE_CLUSTER) {
      // Pure append operation
      uint32_t new_cluster = alloc_cluster(len + offset - index);
      write_clusters(new_cluster, buf + index - offset, len + offset - index);
      file_allocation_table[last_cluster] = new_cluster;
      flush_fat();
      uint32_t new_file_len = len + offset;
      update_file_length(path, new_file_len);
      return new_file_len;
    } else if (index < offset && index + cluster_size > offset) {
      uint8_t *temp_buf = (uint8_t *)kmalloc(cluster_size);
      read_clusters(cluster, temp_buf, cluster_size);
      for (int i = offset - index; i < cluster_size; i++) {
        temp_buf[i] = buf[index - offset + i];
      }
      write_clusters(cluster, temp_buf, cluster_size);
      kfree(temp_buf);
    } else if (index < offset + len && index + cluster_size > offset + len) {
      uint8_t *temp_buf = (uint8_t *)kmalloc(cluster_size);
      read_clusters(cluster, temp_buf, cluster_size);
      for (int i = 0; i < len - (index - offset); i++) {
        temp_buf[i] = buf[index - offset + i];
      }
      write_clusters(cluster, temp_buf, cluster_size);
      kfree(temp_buf);
      return 1;
    } else {
      write_clusters(cluster, buf + index - offset, cluster_size);
    }

    last_cluster = cluster;
    cluster = file_allocation_table[cluster] & 0x0FFFFFFF;
  }

  return 1;
}

char mkdir_fat32(char *path) {
  del_fat32(path);

  int path_split = split_path(path);
  char *dir_path;
  char *name;
  if (path_split > 0) {
    dir_path = substring(path, 0, path_split);
    name = substring(path, path_split + 1, strlen(path));
  } else {
    dir_path = make_string_copy("/");
    name = substring(path, 1, strlen(path));
  }

  uint32_t parent_cluster = find_cluster(dir_path);
  if (parent_cluster == INVALID_CLUSTER) {
    kfree(dir_path);
    kfree(name);
    return 0;
  }

  uint32_t new_cluster = alloc_cluster(cluster_size);
  if (new_cluster == INVALID_CLUSTER) {
    kfree(dir_path);
    kfree(name);
    return 0;
  } else {
    flush_fat();
  }

  if (!add_directory_entry(dir_path, name, DIRECTORY_FLAG, cluster_size,
                           new_cluster)) {
    dealloc_clusters(new_cluster);
    kfree(dir_path);
    kfree(name);
    return 0;
  }

  kfree(dir_path);
  kfree(name);

  char *buf = (char *)kmalloc(cluster_size);
  memset(buf, cluster_size, 0);
  write_clusters(new_cluster, (uint8_t *)buf, cluster_size);
  kfree(buf);

  if (!add_directory_entry(path, ".", DIRECTORY_FLAG, cluster_size,
                           new_cluster)) {
    // This should never happen and will result in filesystem corruption.
    panic("Failed to allocate new directory!");
  }

  if (!add_directory_entry(path, "..", DIRECTORY_FLAG, cluster_size,
                           parent_cluster)) {
    // Again, filesystem corruption.
    panic("Failed to allocate new directory!");
  }

  return 1;
}

char del_fat32(char *path) {
  if (path[1] == 0) {
    return 0;
  }

  int path_split = split_path(path);
  char *dir_path;
  char *name;
  if (path_split > 0) {
    dir_path = substring(path, 0, path_split);
    name = substring(path, path_split + 1, strlen(path));
  } else {
    dir_path = make_string_copy("/");
    name = substring(path, 1, strlen(path));
  }

  uint32_t parent_cluster = find_cluster(dir_path);
  if (parent_cluster == INVALID_CLUSTER) {
    kfree(dir_path);
    kfree(name);
    return 0;
  }

  int dir_entry_index = -1;

  size_t table_size = cluster_size;
  struct directory_table_entry *dir_table =
      (struct directory_table_entry *)kmalloc(table_size);
  uint32_t current_cluster = parent_cluster;
  read_clusters(current_cluster, (uint8_t *)dir_table, cluster_size);
  current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  while ((current_cluster & 0x0FFFFFFF) < END_OF_FILE_CLUSTER) {
    table_size += cluster_size;
    dir_table = (struct directory_table_entry *)krealloc(dir_table, table_size);
    read_clusters(current_cluster,
                  (uint8_t *)dir_table + table_size - cluster_size,
                  cluster_size);
    current_cluster = file_allocation_table[current_cluster] & 0x0FFFFFFF;
  }

  char eight_three_filename[12] = {0};
  parse_eight_three_filename(name, eight_three_filename);
  char *long_filename = nullptr;
  for (int i = 0; i < table_size / sizeof(struct directory_table_entry); i++) {
    if (!dir_table[i].filename[0]) {
      break;
    } else if (dir_table[i].filename[0] == DELETED_DIR_ENTRY) {
      continue;
    } else if (dir_table[i].attributes == LONG_FILE_NAME) {
      uint32_t num_entries =
          parse_long_filename((char *)&dir_table[i], &long_filename);
      if (streq(name, long_filename, 13 * num_entries)) {
        kfree(long_filename);
        dir_entry_index = i + num_entries;
        break;
      }
      kfree(long_filename);
      i += num_entries;
    } else if (streq(eight_three_filename, dir_table[i].filename, 11)) {
      dir_entry_index = i;
      break;
    }
  }

  if (dir_entry_index < 0) {
    kfree(dir_path);
    kfree(name);
    kfree(dir_table);
    return 0;
  }

  uint32_t file_cluster = convert_cluster(dir_table[dir_entry_index]);
  dealloc_clusters(file_cluster);

  dir_table[dir_entry_index].filename[0] = DELETED_DIR_ENTRY;
  for (int i = dir_entry_index - 1; i >= 0; i--) {
    if (dir_table[i].attributes != LONG_FILE_NAME) {
      break;
    }
    dir_table[i].filename[0] = DELETED_DIR_ENTRY;
  }
  write_clusters(parent_cluster, (uint8_t *)dir_table, table_size);

  kfree(dir_path);
  kfree(name);
  kfree(dir_table);
  return 1;
}

} // namespace filesystem
