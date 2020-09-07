#include "filesystem/chs.h"
#include "drivers/i386/pata.h"

namespace filesystem {

namespace {

using drivers::ide_device;

} // namespace

uint32_t chs_to_lba(struct ide_device &device, struct chs &coords) {
  return (coords.cylinder * device.num_heads + coords.head) *
             device.sectors_per_track +
         coords.sector - 1;
}

struct chs lba_to_chs(struct ide_device &device, uint32_t lba) {
  struct chs ret;
  ret.cylinder = lba / (device.num_heads * device.sectors_per_track);
  ret.head = (lba / device.sectors_per_track) % device.num_heads;
  ret.sector = (lba % device.sectors_per_track) + 1;
  return ret;
}

} // namespace filesystem
