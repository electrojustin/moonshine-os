#include "lib/std/time.h"

namespace lib {
namespace std {

struct time system_time = {0};

void tick(struct time& tick_len) {
	system_time.seconds += tick_len.seconds;
	system_time.nanoseconds += tick_len.nanoseconds;
	if (system_time.nanoseconds > 1000000000) {
		system_time.seconds++;
		system_time.nanoseconds = system_time.nanoseconds % 1000000000;
	}
}

} // namespace std
} // namespace lib
