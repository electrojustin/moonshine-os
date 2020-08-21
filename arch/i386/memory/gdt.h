#ifndef ARCH_I386_MEMORY_GDT_H
#define ARCH_I386_MEMORY_GDT_H

#include <stdint.h>

namespace arch {
namespace memory {

struct __attribute__((__packed__)) gdt_descriptor {
	uint16_t size; // Actually size in bytes - 1
	uint32_t addr;
};

struct __attribute__((__packed__)) gdt_entry {
	uint16_t limit;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t flags;
	uint8_t base_high;
};

struct __attribute__((packed)) tss {
	uint32_t reserved1;
	uint32_t esp0;
	uint16_t ss0;
	uint32_t esp1;
	uint32_t reserved2;
	uint16_t reserved3;
	uint32_t reserved4[32];
};

struct __attribute__((packed)) tls_segment {
	int32_t gdt_index;
	uint32_t segment_base; // Virtual memory address
	uint32_t limit; // Note: limit, not length, so subtract 1
	uint32_t seg_32bits:1;
	uint32_t contents:1;
	uint32_t read_exec_only:1;
	uint32_t limit_in_pages:1;
	uint32_t useable:1;
};

extern struct tss main_tss;

constexpr uint16_t GDT_TABLE_SIZE = 9;
constexpr uint16_t TLS_ENTRY_OFFSET = 6;
constexpr uint8_t CODE_SELECTOR = 0x8;
constexpr uint8_t DATA_SELECTOR = 0x10;
constexpr uint8_t USER_CODE_SELECTOR = 0x1B;
constexpr uint8_t USER_DATA_SELECTOR = 0x23;
constexpr uint8_t TSS_SELECTOR = 0x2B;

constexpr uint8_t SYSCALL_GATE_SELECTOR = 0x18;

// Initializes a flat GDT.
void setup_gdt(void);

void set_tls(struct tls_segment& tls);

void flush_tss(void);

} // namespace memory
} // namespace arch

#endif
