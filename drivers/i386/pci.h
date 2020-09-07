#ifndef DRIVERS_I386_PCI_H
#define DRIVERS_I386_PCI_H

#include <stddef.h>
#include <stdint.h>

namespace drivers {

// Note this looks a little different from the diagrams.
// This is because the diagrams are written backwards,
// both x86 and PCI are little endian.
struct __attribute__((__packed__)) pci_config_header {
  uint16_t vendor_id;
  uint16_t device_id;
  uint16_t command;
  uint16_t status;
  uint8_t revision_id;
  uint8_t prog;
  uint8_t subclass_code;
  uint8_t class_code;
  uint8_t cache_line_size;
  uint8_t latency_timer;
  uint8_t header_type;
  uint8_t self_test;
};

struct pci_device_list {
  size_t num_devices;
  uint8_t *busses;
  uint8_t *devices;
  uint8_t *functions;
  struct pci_config_header *config_headers;
};

uint32_t pci_read_config_register(uint8_t bus, uint8_t device, uint8_t func,
                                  uint8_t addr);

void pci_write_config_register(uint8_t bus, uint8_t device, uint8_t func,
                               uint8_t addr, uint32_t value);

void pci_read_config_header(uint8_t bus, uint8_t device, uint8_t func,
                            struct pci_config_header *config);

void pci_write_config_header(uint8_t bus, uint8_t device, uint8_t func,
                             struct pci_config_header *config);

char check_function(uint8_t bus, uint8_t device, uint8_t func);

struct pci_device_list find_pci_devices(void);

} // namespace drivers

#endif
