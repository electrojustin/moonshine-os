#ifndef PROC_SLEEP_H
#define PROC_SLEEP_H

#include "lib/std/time.h"

namespace proc {

namespace {

using lib::std::time;

} // namespace

constexpr uint32_t SLEEP_WAIT = 0x2;

struct sleep_wait : wait_reason {
	struct time end_time;
};

uint32_t nanosleep(uint32_t req_addr, uint32_t rem_addr, uint32_t reserved1, uint32_t reserved2, uint32_t reserved3);

} // namespace proc

#endif
