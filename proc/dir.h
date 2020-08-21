#ifndef PROC_DIR_H
#define PROC_DIR_H

#include <stdint.h>

namespace proc {

uint32_t chdir(uint32_t path_addr, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4);

uint32_t getdents(uint32_t file_descriptor, uint32_t output, uint32_t count, uint32_t reserved1, uint32_t reserved2);

uint32_t mkdir(uint32_t path_addr, uint32_t mode, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

} // namespace proc

#endif
