#ifndef PROC_STAT_H
#define PROC_STAT_H

#include <stdint.h>

namespace proc {

struct __attribute__((packed)) stat64 {
  uint64_t dev_id;
  uint64_t inode_num;
  uint32_t mode;
  uint32_t num_hardlinks;
  uint32_t uid;
  uint32_t gid;
  uint64_t special_dev_id;
  uint64_t reserved1;
  uint64_t size;
  uint32_t block_size;
  uint32_t reserved2;
  uint32_t num_blocks;
  uint32_t access_time;
  uint32_t access_time_nano;
  uint32_t modify_time;
  uint32_t modify_time_nano;
  uint32_t status_time;
  uint32_t status_time_nano;
};

uint32_t fstat64(uint32_t fd, uint32_t stat_addr, uint32_t reserved1,
                 uint32_t reserved2, uint32_t reserved3, uint32_t reserved4);

uint32_t stat64(uint32_t path_addr, uint32_t stat_addr, uint32_t reserved1,
                uint32_t reserved2, uint32_t reserved3, uint32_t reserved4);

} // namespace proc

#endif
