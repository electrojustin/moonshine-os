#ifndef ARCH_I386_MEMORY_MEMINFO_H
#define ARCH_I386_MEMORY_MEMINFO_H

#include <stdint.h>

namespace arch {
namespace memory {

struct __attribute__((__packed__)) memory_info_entry {
  uint32_t addr_low;
  uint32_t addr_high;
  uint32_t length_low;
  uint32_t length_height;
  uint32_t type;
  uint32_t reserved;
};

} // namespace memory
} // namespace arch

#endif
