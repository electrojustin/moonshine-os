#ifndef PROC_SEEK_H
#define PROC_SEEK_H

#include <stdint.h>

namespace proc {

uint32_t lseek(uint32_t file_descriptor, uint32_t offset, uint32_t whence,
               uint32_t reserved1, uint32_t reserved2);

uint32_t llseek(uint32_t file_descriptor, uint32_t offset_high,
                uint32_t offset_low, uint32_t result_addr, uint32_t whence);

constexpr uint32_t SEEK_SET = 0;
constexpr uint32_t SEEK_CUR = 1;
constexpr uint32_t SEEK_END = 2;

} // namespace proc

#endif
