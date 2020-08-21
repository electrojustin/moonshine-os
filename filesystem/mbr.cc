#include "drivers/i386/pata.h"
#include "filesystem/chs.h"
#include "filesystem/mbr.h"
#include "lib/std/stdio.h"

namespace filesystem {

namespace {

using drivers::ide_device;
using lib::std::panic;

struct __attribute__((packed)) partition_table_entry {
	uint8_t boot_flag;
	uint8_t start_head;
	uint8_t start_sector; // Upper 2 bits are part of start cylinder
	uint8_t start_cylinder;
	uint8_t type;
	uint8_t end_head;
	uint8_t end_sector; // Upper 2 bits are part of end cylinder
	uint8_t end_cylinder;
	uint32_t relative_sector;
	uint32_t length;
};

}
		
struct partition disk_partitions[4];

void read_disk_partitions(struct ide_device& device) {
	if (!device.is_connected) {
		panic("Invalid drive!");
	}

	uint8_t mbr_buf[512];
	drivers::read_sectors(device, mbr_buf, 1, 0);

	if (mbr_buf[510] != 0x55 || mbr_buf[511] != 0xAA) {
		panic("Invalid MBR!");
	}

	struct partition_table_entry* partition_table = (struct partition_table_entry*)(mbr_buf + 446);
	// Parse the 4 partition table entries of the MBR
	for (int i = 0; i < 4; i++) {
		disk_partitions[i].boot_flag = partition_table[i].boot_flag;
		disk_partitions[i].type = partition_table[i].type;
		disk_partitions[i].length = partition_table[i].length;
		disk_partitions[i].starting_lba = partition_table[i].relative_sector;
		disk_partitions[i].ending_lba = partition_table[i].relative_sector + partition_table[i].length;
	}
}

} // namespace filesystem
