#include "proc/exit.h"
#include "proc/process.h"

namespace proc {

uint32_t exit(uint32_t reserved1, uint32_t reserved2, uint32_t reserved3, uint32_t reserved4, uint32_t reserved5) {
	get_currently_executing_process()->process_state = STOPPED;
	advance_process_queue();
	execute_processes();

	return 0;
}

} // namespace proc
