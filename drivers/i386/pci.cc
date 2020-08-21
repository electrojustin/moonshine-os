#include "drivers/i386/pci.h"
#include "io/i386/io.h"
#include "lib/std/memory.h"

namespace drivers {

namespace {

using io::out32;
using io::in32;
using lib::std::kmalloc;
using lib::std::kfree;
using lib::std::krealloc;

constexpr uint16_t CONFIG_ADDRESS_PORT = 0x0CF8;
constexpr uint16_t CONFIG_DATA_PORT = 0x0CFC;

uint32_t compute_config_address(uint8_t bus, uint8_t device, uint8_t func, uint8_t addr) {
	uint32_t config_address = 0;
	config_address = (uint32_t)addr & 0xFC |
			 (uint32_t)func << 8 |
			 (uint32_t)device << 11 |
			 (uint32_t)bus << 16 |
			 0x80000000;
	return config_address;
}

} // namespace

uint32_t pci_read_config_register(uint8_t bus, uint8_t device, uint8_t func, uint8_t addr) {
	uint32_t config_address = compute_config_address(bus, device, func, addr);
	out32(CONFIG_ADDRESS_PORT, config_address);
	uint32_t value = in32(CONFIG_DATA_PORT);
	return value;
}

void pci_write_config_register(uint8_t bus, uint8_t device, uint8_t func, uint8_t addr, uint32_t value) {
	uint32_t config_address = compute_config_address(bus, device, func, addr);
	out32(CONFIG_ADDRESS_PORT, config_address);	
	out32(CONFIG_DATA_PORT, value);
}

void pci_read_config_header(uint8_t bus, uint8_t device, uint8_t func, struct pci_config_header* config) {
	uint32_t* config_buf = (uint32_t*)config;
	for (int i = 0; i < sizeof(struct pci_config_header)/sizeof(uint32_t); i++) {
		config_buf[i] = pci_read_config_register(bus, device, func, i*sizeof(uint32_t));
	}
}

void pci_write_config_header(uint8_t bus, uint8_t device, uint8_t func, struct pci_config_header* config) {
	uint32_t* config_buf = (uint32_t*)config;
	for (int i = 0; i < sizeof(struct pci_config_header)/sizeof(uint32_t); i++) {
		pci_write_config_register(bus, device, func, i*sizeof(uint32_t), config_buf[i]);
	}
}

char check_function(uint8_t bus, uint8_t device, uint8_t func) {
	uint32_t config_register = pci_read_config_register(bus, device, func, 0);
	if (config_register == 0xFFFFFFFF) {
		return 0;
	} else {
		return 1;
	}
}

struct pci_device_list find_pci_devices(void) {
	struct pci_device_list ret;
	ret.num_devices = 0;
	size_t alloc_size = 1;
	ret.busses = (uint8_t*)kmalloc(alloc_size*sizeof(uint8_t));
	ret.devices = (uint8_t*)kmalloc(alloc_size*sizeof(uint8_t));
	ret.functions = (uint8_t*)kmalloc(alloc_size*sizeof(uint8_t));
	ret.config_headers = (struct pci_config_header*)kmalloc(alloc_size*sizeof(struct pci_config_header));

	for (int bus = 0; bus < 256; bus++) {
		for (int device = 0; device < 32; device++) {
			for (int function = 0; function < 8; function++) {
				if (check_function(bus, device, function)) {
					ret.num_devices++;
					if (ret.num_devices > alloc_size) {
						alloc_size *= 2;
						ret.busses = (uint8_t*)krealloc(ret.busses, alloc_size*sizeof(uint8_t));
						ret.devices = (uint8_t*)krealloc(ret.devices, alloc_size*sizeof(uint8_t));
						ret.functions = (uint8_t*)krealloc(ret.functions, alloc_size*sizeof(uint8_t));
						ret.config_headers = (struct pci_config_header*)krealloc(ret.config_headers, alloc_size*sizeof(struct pci_config_header));
					}
					ret.busses[ret.num_devices-1] = bus;
					ret.devices[ret.num_devices-1] = device;
					ret.functions[ret.num_devices-1] = function;
					pci_read_config_header(bus, device, function, ret.config_headers + (ret.num_devices - 1));
				}
			}
		}
	}

	return ret;
}

} // namespace drivers
