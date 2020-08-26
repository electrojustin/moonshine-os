#include "arch/i386/memory/paging.h"
#include "lib/std/memory.h"
#include "lib/std/time.h"
#include "proc/process.h"
#include "proc/sleep.h"
#include "lib/std/stdio.h"

namespace proc {

namespace {

using arch::memory::virtual_to_physical_memcpy;
using lib::std::kmalloc;
using lib::std::system_time;
using lib::std::time;

} // namespace

uint32_t nanosleep(uint32_t req_addr, uint32_t rem_addr, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3) {
	struct process* current_process = get_currently_executing_process();
	uint32_t* page_dir = current_process->page_dir;
	struct time* wait_time = (struct time*)kmalloc(sizeof(struct time));
	virtual_to_physical_memcpy(page_dir, (char*)req_addr, (char*)wait_time, sizeof(struct time));

	struct sleep_wait* wait = (struct sleep_wait*)kmalloc(sizeof (struct sleep_wait));
	wait->type = SLEEP_WAIT;
	wait->end_time = system_time;
	wait->end_time.seconds += wait_time->seconds;
	wait->end_time.nanoseconds += wait_time->nanoseconds;
	if (wait->end_time.nanoseconds > 1000000000) {
		wait->end_time.seconds++;
		wait->end_time.nanoseconds = wait->end_time.nanoseconds % 1000000000;
	}

	current_process->process_state = WAITING;
	current_process->wait = wait;

	kfree(wait_time);

	return 0;
}

uint32_t clock_nanosleep(uint32_t clock_id, uint32_t flags, uint32_t req_addr, uint32_t rem_addr, uint32_t reserved1) {
	return nanosleep(req_addr, rem_addr, 0, 0, 0);
}

} // namespace proc
