#ifndef LIB_STD_STDIO_H
#define LIB_STD_STDIO_H

#include <stdint.h>

extern char stack_bottom;
extern char stack_top;

namespace lib {
namespace std {

namespace {
void *current_ebp = nullptr;
void *original_ebp = nullptr;
uint32_t eip;
uint32_t esp;
uint32_t ebp;
uint32_t eax;
uint32_t ebx;
uint32_t ecx;
uint32_t edx;
uint32_t esi;
uint32_t edi;
} // namespace

void setcolor(uint8_t color);

void putc(char c);

void puts(const char *s);

char getc(void);

int printk(const char *format, ...);

void print_stack_trace(void *current_ebp);

static inline void print_error(const char *message, uint32_t provided_ebp = 0) {
  // Dump registers.
  asm volatile("mov %%esp, %0\n"
               "mov %%ebp, %1\n"
               "mov %%eax, %2\n"
               "mov %%ebx, %3\n"
               "mov %%ecx, %4\n"
               "mov %%edx, %5\n"
               "mov %%esi, %6\n"
               "mov %%edi, %7\n"
               : "=m"(esp), "=m"(ebp), "=m"(eax), "=m"(ebx), "=m"(ecx),
                 "=m"(edx), "=m"(esi), "=m"(edi));
  if (provided_ebp) {
    ebp = provided_ebp;
  }
  printk("Error: %s\n", message);
  printk("esp: %x   ebp: %x\n", esp, ebp);
  printk("eax: %x   ebx; %x   ecx: %x\n", eax, ebx, ecx);
  printk("edx: %x   esi: %x   edi: %x\n", edx, esi, edi);

  // Print the stack.
  print_stack_trace((void *)ebp);
}

static inline void panic(const char *message) {
  printk("Kernel panic!\n");
  print_error(message);

  // Halt forever.
  while (1) {
    asm volatile("cli\n"
                 "hlt");
  }
  __builtin_unreachable();
}

} // namespace std
} // namespace lib

#endif
