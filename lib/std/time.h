#ifndef LIB_STD_TIME_H
#define LIB_STD_TIME_H

#include <stdint.h>

namespace lib {
namespace std {

struct time {
	uint32_t seconds;
	uint32_t nanoseconds;
};

extern struct time system_time;

void tick(struct time& tick_len);

} // namespace std
} // namespace lib

#endif
