#include "drivers/i386/pata.h"
#include "io/i386/io.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"

constexpr uint8_t NO_INTERRUPT_FLAG = 0x02;
constexpr uint8_t BUSY_FLAG = 0x80;
constexpr uint8_t DEVICE_SELECT_FLAG = 0x10;
constexpr uint8_t LBA_FLAG = 0x40;

namespace drivers {

namespace {

using io::in;
using io::in_block16;
using io::out;
using io::out_block16;

void block_until_drive_ready(const struct ide_device &device) {
  // Wait until the device is ready for our command.
  uint8_t status = BUSY_FLAG;
  while (status & BUSY_FLAG && status != 0xFF) {
    status = read_io_register(device, STATUS_REGISTER);
  }
}

void enter_lba_address(const struct ide_device &device, uint8_t num_sectors,
                       uint32_t lba) {
  write_io_register(device, SECTOR_COUNT_REGISTER, num_sectors);
  write_io_register(device, LBA_LOWEST_REGISTER, lba & 0xFF);
  write_io_register(device, LBA_LOWER_REGISTER, (lba >> 8) & 0xFF);
  write_io_register(device, LBA_MID_REGISTER, (lba >> 16) & 0xFF);
  uint8_t drive_reg = read_io_register(device, DRIVE_HEAD_REGISTER);
  drive_reg = (drive_reg & 0xF0) | ((lba >> 24) & 0xF) | LBA_FLAG;
  write_io_register(device, DRIVE_HEAD_REGISTER, drive_reg);
}

} // namespace

// These values are the default ports for the IDE controller and will almost
// never vary. If you suspect otherwise, you can consult the PCI config of the
// controller.
struct ide_device devices[4] = {
    {.io_base_port = 0x01F0, .control_base_port = 0x03F6, .device_select = 0},
    {.io_base_port = 0x01F0, .control_base_port = 0x03F6, .device_select = 1},
    {.io_base_port = 0x0170, .control_base_port = 0x0376, .device_select = 0},
    {.io_base_port = 0x0170, .control_base_port = 0x0376, .device_select = 1}};

uint8_t read_io_register(const struct ide_device &device, uint8_t reg) {
  uint8_t value;
  value = in(device.io_base_port + reg);
  return value;
}

void read_io_word_block(const struct ide_device &device, uint8_t reg,
                        uint16_t *buffer, size_t size) {
  in_block16(device.io_base_port + reg, buffer, size);
}

uint8_t read_control_register(const struct ide_device &device, uint8_t reg) {
  uint8_t value;
  value = in(device.control_base_port + reg);
  return value;
}

void write_io_register(const struct ide_device &device, uint8_t reg,
                       uint8_t value) {
  out(device.io_base_port + reg, value);
}

void write_io_word_block(const struct ide_device &device, uint8_t reg,
                         uint16_t *buffer, size_t size) {
  out_block16(device.io_base_port + reg, buffer, size);
}

void write_control_register(const struct ide_device &device, uint8_t reg,
                            uint8_t value) {
  out(device.control_base_port + reg, value);
}

void set_ide_interrupt_enable(const struct ide_device &device,
                              char is_enabled) {
  uint8_t reg_value = read_control_register(device, DEVICE_CONTROL_REGISTER);
  if (is_enabled) {
    reg_value = reg_value & (~NO_INTERRUPT_FLAG);
    write_control_register(device, DEVICE_CONTROL_REGISTER, reg_value);
  } else {
    reg_value = reg_value | NO_INTERRUPT_FLAG;
    write_control_register(device, DEVICE_CONTROL_REGISTER, reg_value);
  }
}

void send_command(const struct ide_device &device, uint8_t command) {
  char device_select = !!device.device_select;
  uint8_t device_reg = read_io_register(device, DRIVE_HEAD_REGISTER);
  if ((device_reg & DEVICE_SELECT_FLAG) >> 4 != device_select) {
    device_reg = ((~DEVICE_SELECT_FLAG) & device_reg) | device_select << 4;
    write_io_register(device, DRIVE_HEAD_REGISTER, device_reg);

    block_until_drive_ready(device);
  }

  write_io_register(device, COMMAND_REGISTER, command);

  block_until_drive_ready(device);
}

void identify_drive(struct ide_device *device) {
  uint8_t status = read_io_register(*device, STATUS_REGISTER);
  if (status == 0xFF) {
    device->is_connected = 0;
    return;
  }

  send_command(*device, IDENTIFY_COMMAND);

  uint16_t buffer[256];
  read_io_word_block(*device, DATA_REGISTER, buffer, 256);

  if (buffer[0]) {
    device->num_cylinders = buffer[1];
    device->num_heads = buffer[3];
    device->sectors_per_track = buffer[6];
    device->is_connected = 1;
    for (int i = 27; i < 46; i++) {
      if (buffer[i]) {
        device->model[2 * (i - 27)] = buffer[i] >> 8;
        device->model[2 * (i - 27) + 1] = buffer[i] & 0xFF;
      } else {
        device->model[2 * (i - 27)] = 0;
        break;
      }
    }
  } else {
    device->is_connected = 0;
  }
}

void read_sectors(const struct ide_device &device, uint8_t *buffer,
                  uint32_t num_sectors, uint32_t lba) {
  for (int i = 0; i < num_sectors; i++) {
    block_until_drive_ready(device);

    enter_lba_address(device, num_sectors, lba);

    send_command(device, READ_COMMAND);

    // Sectors are 512 bytes, or 256 double bytes
    read_io_word_block(device, DATA_REGISTER, (uint16_t *)buffer, 256);
    buffer += 512;
    lba++;
  }
}

void write_sectors(const struct ide_device &device, uint8_t *buffer,
                   uint32_t num_sectors, uint32_t lba) {
  for (int i = 0; i < num_sectors; i++) {
    block_until_drive_ready(device);

    enter_lba_address(device, num_sectors, lba);

    send_command(device, WRITE_COMMAND);

    // Sectors are 512 bytes, or 256 double bytes
    write_io_word_block(device, DATA_REGISTER, (uint16_t *)buffer, 256);
    buffer += 512;
    lba++;
  }
}

} // namespace drivers
