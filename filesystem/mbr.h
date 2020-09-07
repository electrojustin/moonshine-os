#ifndef FILESYSTEM_MBR_H
#define FILESYSTEM_MBR_H

#include <stdint.h>

#include "drivers/i386/pata.h"

namespace filesystem {

namespace {

using drivers::ide_device;

} // namespace

struct partition {
  char boot_flag;
  uint8_t type;
  uint32_t starting_lba;
  uint32_t ending_lba;
  uint32_t length; // Measured in blocks/sectors
};

extern struct partition disk_partitions[4];

void read_disk_partitions(struct ide_device &device);

} // namespace filesystem

#endif
