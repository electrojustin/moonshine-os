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
  uint32_t prev_tss;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
};

struct __attribute__((packed)) tls_segment {
  int32_t gdt_index;
  uint32_t segment_base; // Virtual memory address
  uint32_t limit;        // Note: limit, not length, so subtract 1
  uint32_t seg_32bits : 1;
  uint32_t contents : 1;
  uint32_t read_exec_only : 1;
  uint32_t limit_in_pages : 1;
  uint32_t useable : 1;
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

void set_tls(struct tls_segment &tls);

void flush_tss(void);

} // namespace memory
} // namespace arch

#endif
