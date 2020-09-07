#ifndef PROC_EXIT_H
#define PROC_EXIT_H

#include <stdint.h>

#include "proc/process.h"

namespace proc {

uint32_t exit(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3,
              uint32_t reserved4, uint32_t reserved5);

} // namespace proc

#endif
