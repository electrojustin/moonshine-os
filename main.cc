#include <stdbool.h>
#include <stddef.h>

#include "arch/i386/cpu/sse.h"
#include "arch/i386/interrupts/apic.h"
#include "arch/i386/interrupts/pic.h"
#include "arch/i386/memory/gdt.h"
#include "arch/i386/memory/meminfo.h"
#include "arch/i386/memory/paging.h"
#include "arch/i386/multiboot.h"
#include "arch/interrupts/control.h"
#include "arch/interrupts/interrupts.h"
#include "drivers/i386/keyboard.h"
#include "drivers/i386/pata.h"
#include "drivers/i386/pci.h"
#include "drivers/i386/pit.h"
#include "filesystem/fat32.h"
#include "filesystem/mbr.h"
#include "io/vga.h"
#include "lib/std/memory.h"
#include "lib/std/stdio.h"
#include "lib/std/string.h"
#include "proc/arch.h"
#include "proc/brk.h"
#include "proc/close.h"
#include "proc/dir.h"
#include "proc/elf_loader.h"
#include "proc/execve.h"
#include "proc/exit.h"
#include "proc/fork.h"
#include "proc/ioctl.h"
#include "proc/mmap.h"
#include "proc/open.h"
#include "proc/pid.h"
#include "proc/process.h"
#include "proc/read_write.h"
#include "proc/seek.h"
#include "proc/sleep.h"
#include "proc/stat.h"
#include "proc/syscall.h"
#include "proc/thread_area.h"
#include "proc/uid.h"
#include "proc/uname.h"

extern "C" {
void kernel_main(multiboot_info_t *multiboot_info, unsigned int magic);
}

using arch::memory::memory_info_entry;
using drivers::pci_device_list;
using filesystem::directory;
using lib::std::mem_stats;

extern "C" void userspace_test(void);

void kernel_main(multiboot_info_t *multiboot_info, unsigned int magic) {
  // Disable interrupts initially.
  arch::interrupts::disable_interrupts();

  // Setup VGA and set the text color to light grey on black.
  io::vga_initialize();
  lib::std::setcolor(
      io::vga_entry_color(io::VGA_COLOR_LIGHT_GREY, io::VGA_COLOR_BLACK));

  // Setup identity paging and a flat GDT.
  arch::memory::map_memory_range(base_page_directory, base_page_table, 0, 0,
                                 (uint32_t)1024 * 1023 * 4096,
                                 arch::memory::kernel_read_write);
  arch::memory::set_page_directory(base_page_directory);
  arch::memory::enable_paging();
  arch::memory::setup_gdt();

  // Create an IDT.
  arch::interrupts::initialize_interrupts();

  // Check for SSE support
  arch::cpu::maybe_enable_sse();

  // Use the memory map provided to us by GRUB to initialize our dynamic memory
  // management.
  lib::std::printk("Mapping memory...\n");
  struct memory_info_entry *memory_table =
      (struct memory_info_entry *)(multiboot_info->mmap_addr + 4);
  uint16_t num_mmap_entries =
      multiboot_info->mmap_length / sizeof(memory_info_entry);
  for (int i = 0; i < num_mmap_entries; i++) {
    lib::std::printk(
        "Segment start: %x  Segment length: %x  Segment type: %x\n",
        memory_table[i].addr_low, memory_table[i].length_low,
        memory_table[i].type);
    if (memory_table[i].type == 1) {
      lib::std::initialize_allocator((void *)memory_table[i].addr_low,
                                     (void *)((char *)memory_table[i].addr_low +
                                              memory_table[i].length_low));
    }
  }

  // Mask all interrupts.
  arch::interrupts::pic_set_mask(0xFFFF);

  // Initialize the PS/2 keyboard with interrupt 0x21.
  drivers::init_keyboard(0x21);

  // Initialize the programmable interrupt controller (PIC) to use interrupts
  // 0x20-0x2F.
  arch::interrupts::pic_initialize(0x20);

  // Get a list of PCI devices.
  struct pci_device_list pci_list = drivers::find_pci_devices();

  drivers::identify_drive(&drivers::devices[0]);
  drivers::set_ide_interrupt_enable(drivers::devices[0], 0);

  lib::std::printk("Drive: %s\n", drivers::devices[0].model);
  lib::std::printk("Cylinders: %d Heads: %d Sectors per track: %d\n",
                   drivers::devices[0].num_cylinders,
                   drivers::devices[0].num_heads,
                   drivers::devices[0].sectors_per_track);

  filesystem::read_disk_partitions(drivers::devices[0]);
  for (int i = 0; i < 4; i++) {
    lib::std::printk("Partition %d: Start: %x  End: %x  Type: %x\n", i,
                     filesystem::disk_partitions[i].starting_lba,
                     filesystem::disk_partitions[i].ending_lba,
                     (uint32_t)filesystem::disk_partitions[i].type);
  }

  filesystem::init_fat32(drivers::devices[0], filesystem::disk_partitions[0]);

  proc::initialize_syscalls(0x198);
  proc::register_syscall(0x01, proc::exit);
  proc::register_syscall(0x02, proc::fork);
  proc::register_syscall(0x03, proc::read);
  proc::register_syscall(0x04, proc::write);
  proc::register_syscall(0x05, proc::open);
  proc::register_syscall(0x06, proc::close);
  proc::register_syscall(0x0A, proc::unlink);
  proc::register_syscall(0x0B, proc::execve);
  proc::register_syscall(0x0C, proc::chdir);
  proc::register_syscall(0x13, proc::lseek);
  proc::register_syscall(0x14, proc::getpid);
  proc::register_syscall(0x21, proc::access);
  proc::register_syscall(0x27, proc::mkdir);
  proc::register_syscall(0x2D, proc::brk);
  proc::register_syscall(0x36, proc::ioctl);
  proc::register_syscall(0x55, proc::readlink);
  proc::register_syscall(0x5A, proc::mmap);
  proc::register_syscall(0x5B, proc::munmap);
  proc::register_syscall(0x78, proc::clone);
  proc::register_syscall(0x7A, proc::new_uname);
  proc::register_syscall(0x7D, proc::mprotect);
  proc::register_syscall(0x8C, proc::llseek);
  proc::register_syscall(0x90, proc::msync);
  proc::register_syscall(0x92, proc::writev);
  proc::register_syscall(0xA2, proc::nanosleep);
  proc::register_syscall(0xB4, proc::pread64);
  proc::register_syscall(0xC0, proc::mmap);
  proc::register_syscall(0xC3, proc::stat64);
  proc::register_syscall(0xC5, proc::fstat64);
  proc::register_syscall(0xC7, proc::getuid);
  proc::register_syscall(0xC8, proc::getgid);
  proc::register_syscall(0xC9, proc::geteuid);
  proc::register_syscall(0xCA, proc::getegid);
  proc::register_syscall(0xDC, proc::getdents);
  proc::register_syscall(0xF3, proc::set_thread_area);
  proc::register_syscall(0xF4, proc::get_thread_area);
  proc::register_syscall(0xFC, proc::exit);
  proc::register_syscall(0x127, proc::openat);
  proc::register_syscall(0x180, proc::arch_prctl);
  proc::register_syscall(0x197, proc::clock_nanosleep);

  // Initialize the Programmable Interrupt Timer with interrupt 0x20.
  drivers::init_pit(0x20, 1000);

  // Load the initial process
  char **argv = (char **)lib::std::kmalloc(2 * sizeof(char *));
  argv[0] = lib::std::make_string_copy("/init.exe");
  argv[1] = nullptr;
  proc::load_elf(argv[0], 1, argv);

  // Execute processes
  proc::execute_processes();
}
