#ifndef PROC_IOCTL_H
#define PROC_IOCTL_H

#include <stdint.h>

namespace proc {

uint32_t ioctl(uint32_t fd, uint32_t request, uint32_t param1, uint32_t param2,
               uint32_t param3, uint32_t param4);

} // namespace proc

#endif
