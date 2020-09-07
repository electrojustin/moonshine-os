#ifndef DRIVERS_PATA_H
#define DRIVERS_PATA_H

#include <stddef.h>
#include <stdint.h>

namespace drivers {

// IO registers
constexpr uint8_t DATA_REGISTER = 0;
constexpr uint8_t ERROR_REGISTER = 1;
constexpr uint8_t FEATURES_REGISTER = 1;
constexpr uint8_t SECTOR_COUNT_REGISTER = 2;
constexpr uint8_t LBA_LOWEST_REGISTER = 3;
constexpr uint8_t LBA_LOWER_REGISTER = 4;
constexpr uint8_t LBA_MID_REGISTER = 5;
constexpr uint8_t DRIVE_HEAD_REGISTER = 6;
constexpr uint8_t STATUS_REGISTER = 7;
constexpr uint8_t COMMAND_REGISTER = 7;

// Control registers
constexpr uint8_t ALT_STATUS_REGISTER = 0;
constexpr uint8_t DEVICE_CONTROL_REGISTER = 0;
constexpr uint8_t DRIVE_ADDRESS_REGISTER = 1;

// Commands
constexpr uint8_t IDENTIFY_COMMAND = 0xEC;
constexpr uint8_t READ_COMMAND = 0x20;
constexpr uint8_t WRITE_COMMAND = 0x30;

struct ide_device {
  // Includes:
  // Data, Error, Features
  // Sector count, Sector number, Cylinder low
  // Cylinder high, Drive/Head, Status, Command
  uint16_t io_base_port;
  // Includes:
  // Alternative status, Device control, Device select
  uint16_t control_base_port;
  char device_select; // 1 bit value

  // Basic infromation
  char is_connected;
  char model[41];

  // Drive geometry
  uint16_t num_cylinders;
  uint16_t num_heads;
  uint16_t sectors_per_track;
};

extern struct ide_device devices[4];

uint8_t read_io_register(const struct ide_device &device, uint8_t reg);

void read_io_word_block(const struct ide_device &device, uint8_t reg,
                        uint16_t *buffer, size_t size);

uint8_t read_control_register(const struct ide_device &device, uint8_t reg);

void write_io_register(const struct ide_device &device, uint8_t reg,
                       uint8_t value);

void write_io_word_block(const struct ide_device &device, uint8_t reg,
                         uint16_t *buffer, size_t size);

void write_control_register(const struct ide_device &device, uint8_t reg,
                            uint8_t value);

void set_ide_interrupt_enable(const struct ide_device &device, char is_enabled);

void send_command(const struct ide_device &device, uint8_t command);

void identify_drive(struct ide_device *device);

void read_sectors(const struct ide_device &device, uint8_t *buffer,
                  uint32_t num_sectors, uint32_t lba);

void write_sectors(const struct ide_device &device, uint8_t *buffer,
                   uint32_t num_sectors, uint32_t lba);

} // namespace drivers

#endif
