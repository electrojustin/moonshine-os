#ifndef PROC_THREAD_AREA_H
#define PROC_THREAD_AREA_H

#include <stdint.h>

namespace proc {

uint32_t set_thread_area(uint32_t reserved1, uint32_t reserved2,
                         uint32_t reserved3, uint32_t reserved4,
                         uint32_t reserved5);

uint32_t get_thread_area(uint32_t reserved1, uint32_t reserved2,
                         uint32_t reserved3, uint32_t reserved4,
                         uint32_t reserved5);

} // namespace proc

#endif
