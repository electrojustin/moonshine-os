#ifndef FILESYSTEM_CHS_H
#define FILESYSTEM_CHS_H

#include <stdint.h>

#include "drivers/i386/pata.h"

namespace filesystem {

namespace {

using drivers::ide_device;

} // namespace

struct chs {
	uint16_t cylinder;
	uint8_t head;
	uint8_t sector;
};

uint32_t chs_to_lba(struct ide_device& device, struct chs& coords);

struct chs lba_to_chs(struct ide_device& device, uint32_t lba);

} // namespace filesystem

#endif
